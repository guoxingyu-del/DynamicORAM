#include "RankOram.h"
#include <math.h>
#include <algorithm>
#include <iostream>
#include "Utils.h"
#include <numeric>

using namespace std;

RankOram::RankOram() {
    max_level = 2;
    count = 0;
    number = 0;
    bandwidth = 0;
    server.resize(max_level + 1);
    for (int i = 0; i <= max_level; i++) {
        server[i].resize(1 << i);
    }
    is_full.resize(max_level + 1, 0);
    client.resize(max_level + 1);
    sentinel.resize(max_level + 1);
    dummy_counter.resize(max_level + 1, 0);
}

array<int8_t, Block::DATA_SIZE> RankOram::Access(Operation op, const int address, array<int8_t, Block::DATA_SIZE>& newdata) {
    auto pos = Position(address);
    vector<int> Q(max_level + 1);
    for (int i = 0; i <= max_level; i++) {
        if (is_full[i]) {
            if (i == pos.first) {
                Q[i] = Permutate(i, pos.second);
            }
            else {
                Q[i] = Permutate(i, dummy_counter[i]);
                dummy_counter[i]++;
            }
        }
        else Q[i] = -1;
    }
    // send Q to the server
    bandwidth += sizeof(int) * (max_level + 1);
    
    vector<pair<int, Block>> O;
    for (int i = 0; i <= max_level; i++) {
        if (Q[i] != -1) {
            O.push_back(make_pair(i, server[i][Q[i]]));
        }
    }
    // send O to the client
    bandwidth += O.size() * (sizeof(int) + Block::GetBlockSize(true));
    
    Block* target = new Block();
    for (int i = 0; i < O.size(); i++) {
        if (O[i].first == pos.first) {
            target->data = O[i].second.data;
            target->index = O[i].second.index;
            target->is_delete = O[i].second.is_delete;
            break;
        }
    }
    array<int8_t, Block::DATA_SIZE> res = target->data;

    count = (count + 1) % (1 << (max_level - 1));
    int rebuild_level = Msb(count) + 1;

    int delete_flag = 0;
    if (op == Operation::WRITE) {
        target->data = newdata;
    }
    else if (op == Operation::INSERT) {
        target->index = address;
        target->data = newdata;
        number++;
        if (number > (1 << (max_level - 1))) {
            rebuild_level = max_level + 1;
            delete_flag = 1;
        }
    }
    else if (op == Operation::DEL) {
        target->is_delete = 1;
        number--;
        // delete last level
        if (number == (1 << (max_level - 2)) || number == 0) {
            rebuild_level = max_level - 1;
            if (rebuild_level < 2) rebuild_level = 2;
            delete_flag = 1;
        }
    }
    // target -> T0
    server[0][0] = *target;
    bandwidth += Block::GetBlockSize(true);
    // target->address -> S0
    client[0].push_back(target->index);
    is_full[0] = 1;
    Rebuild(rebuild_level, delete_flag);
    return res;
}

pair<int, int> RankOram::Position(const int x) {
    if (is_full[0] && x == client[0][0]) return make_pair(0, 0);
    for (int i = 1; i <= max_level; i++) {
        if (is_full[i]) {
            auto upper = upper_bound(sentinel[i].begin(), sentinel[i].end(), x);
            if (upper == sentinel[i].begin()) continue;
            int basis_index = upper - sentinel[i].begin() - 1;
            int basis = sentinel[i][basis_index];
            if (basis == x) return {i, basis_index * i};
            for (int j = basis_index * i + 1; j < client[i].size() && j < (basis_index + 1) * i; j++) {
                basis += client[i][j];
                if (basis == x) return {i, j};
            }
        }
    }
    // dummy position
    return make_pair(-1, -1);
}

void RankOram::Rebuild(const int rebuild_level, const int delete_flag) {
    int depose_level = rebuild_level;
    if (delete_flag) {
        depose_level = max_level;
    }
    vector<Block> eviction;
    set<int> delete_Blocks;
    for (int i = 0; i <= depose_level; i++) {
        if (is_full[i]) {
            Evict(i, eviction, delete_flag, delete_Blocks);
        }
    }

    // merge all clients
    vector<int> merge;
    vector<int> new_sentinel;
    MergeClient(depose_level, merge, delete_flag, delete_Blocks);
    CreateSentinel(rebuild_level, merge, new_sentinel);

    if (delete_flag) {
        client.resize(rebuild_level + 1);
        sentinel.resize(rebuild_level + 1);
    }
    client[rebuild_level] = merge;
    sentinel[rebuild_level] = new_sentinel;

    Shuffle(rebuild_level, eviction);    

    // reset
    if (delete_flag) {
        server.resize(rebuild_level + 1);
        is_full.resize(rebuild_level + 1);
        dummy_counter.resize(rebuild_level + 1);
        max_level = rebuild_level; count = 0;
    }
    
    for (int i = 0; i < rebuild_level; i++) {
        client[i].clear(); sentinel[i].clear();
        is_full[i] = 0; dummy_counter[i] = 0;
    }
    is_full[rebuild_level] = number == 0 ? 0 : 1;
    server[rebuild_level] = eviction;
    dummy_counter[rebuild_level] = client[rebuild_level].size();
}

void RankOram::Evict(int level, vector<Block>& eviction, const int delete_flag, set<int>& delete_Blocks) {
    for (int i = 0; i < (1 << level); i++) {
        // pick out the deleted Blocks
        if (delete_flag && server[level][i].is_delete) {
            delete_Blocks.insert(server[level][i].index);
            continue;
        }
        // check is_delete flag
        bandwidth += sizeof(int8_t);
        
        int rank = InvPermutate(level, i);
        if (rank < client[level].size()) {
            int a = Index(rank, level);
            pair<int, int> pos = Position(a);
            if (pos.first == level) {
                // a is untouched
                eviction.push_back(server[level][i]);
            }
        }
        else if (rank >= dummy_counter[level]) {
            eviction.push_back(server[level][i]);
        }
    }
}

int RankOram::Msb(int x) {
    if (x == 0) return max_level - 1;
    // using built-in function of gcc and clang
    return __builtin_ctz(x);
}

void RankOram::MergeClient(const int rebuild_level, vector<int>& merge, const int delete_flag, set<int>& delete_Blocks) {
    auto process_element = [&](int& current_sum, int& last, vector<int>& temp, 
                                vector<int>::iterator& it, vector<int>::iterator end, 
                                vector<int>& src, bool is_same) {
        if (!is_same && !(delete_flag && delete_Blocks.find(current_sum) != delete_Blocks.end())) {
            temp.push_back(current_sum - last);
            last = current_sum;
        }
        ++it;
        if (it != end) current_sum += *it;
    };

    for (int i = 0; i <= rebuild_level; i++) {
        if (!is_full[i]) continue;
        if (merge.empty()) {
            int prefix = 0, ending = 0;
            merge.reserve(client[i].size());
            for (const auto& delta : client[i]) {
                prefix += delta;
                if (!delete_flag || delete_Blocks.find(prefix) == delete_Blocks.end()) {
                    merge.push_back(prefix - (merge.empty() ? 0 : ending));
                    ending = prefix;
                }
            }
            continue;
        }

        vector<int> temp;
        temp.reserve(merge.size() + client[i].size());
        auto it1 = merge.begin(), end1 = merge.end();
        auto it2 = client[i].begin(), end2 = client[i].end();
        
        int sum1 = *it1, sum2 = *it2;
        int last = 0;
        
        while (it1 != end1 && it2 != end2) {
            if (sum1 < sum2) {
                process_element(sum1, last, temp, it1, end1, merge, false);
            } else if (sum1 > sum2) {
                process_element(sum2, last, temp, it2, end2, client[i], false);
            } else {
                process_element(sum1, last, temp, it1, end1, merge, false);
                process_element(sum2, last, temp, it2, end2, client[i], true);
            }
        }

        while (it1 != end1) process_element(sum1, last, temp, it1, end1, merge, false);
        while (it2 != end2) process_element(sum2, last, temp, it2, end2, client[i], false);

        merge.swap(temp);
    }
}

void RankOram::CreateSentinel(const int level, const vector<int>& merge, vector<int>& new_sentinel) {
    int start = 0;
    for (int i = 0; i < merge.size(); i++) {
        start += merge[i];
        if (i % level == 0) {
            new_sentinel.push_back(start);
        }
    }
}

int RankOram::Index(int rank, int level) {
    if (level == 0) return client[level][rank];
    int slice = rank / level;
    int res = sentinel[level][slice];
    for (int i = slice * level + 1; i <= rank; i++) {
        res += client[level][i];
    }
    return res;
}

int RankOram::Contains(const int x, const int level) {
    for (int i = 0; i < server[level].size(); i++) {
        if (x == server[level][i].index) {
            return i;
        }
        bandwidth += sizeof(int);
    }
    return -1;
}

int RankOram::Permutate(int level, int input) {
    if (level == 0) return input;
    int first_bit = ((1 << (level - 1)) & input) >> (level - 1);
    input = input & ((1 << (level - 1)) - 1);
    input <<= 1;
    return input | first_bit;
}

int RankOram::InvPermutate(int level, int input) {
    if (level == 0) return input;
    int last_bit = input & 0x1;
    return (last_bit << (level - 1)) | (input >> 1);
}

int RankOram::Rank(const int x, const int level) {
    auto upper = upper_bound(sentinel[level].begin(), sentinel[level].end(), x);
    int basis_index = upper - sentinel[level].begin() - 1;
    int basis = sentinel[level][basis_index];
    if (basis == x) return basis_index * level;
    for (int j = basis_index * level + 1; j < client[level].size() && j < (basis_index + 1) * level; j++) {
        basis += client[level][j];
        if (basis == x) return j;
    }
    return -1;
}

void RankOram::Shuffle(int level, vector<Block>& blocks) {
    // split by n^0.5
    int n = blocks.size();
    int sqrt_n = ceil(sqrt(n));
    // client
    vector<vector<Block>> Q(sqrt_n);
    // server
    vector<vector<Block>> T(sqrt_n);

    vector<int> random_position(sqrt_n * sqrt_n);
    iota(random_position.begin(), random_position.end(), 0);
    Utils::ShuffleArray(random_position);
    
    for (int i = 0; i < sqrt_n; i++) {
        // server
        vector<Block> I;
        for (int j = i * sqrt_n; j < (i + 1) * sqrt_n; j++) {
            if (random_position[j] < blocks.size()) {
                I.push_back(blocks[random_position[j]]);
            }
            else {
                I.push_back(Block());
            }
        }
        // clinet
        for (int j = 0; j < I.size(); j++) {
            if (I[j].index == -1) continue;
            int d = Permutate(level, Rank(I[j].index, level)) / (2 * sqrt_n);
            Q[d].push_back(I[j]);
        }
        // server
        for (int k = 0; k < 2; k++) {
            for (int j = 0; j < sqrt_n; j++) {
                if (!Q[j].empty()) {
                    T[j].push_back(Q[j][0]);
                    Q[j].erase(Q[j].begin());
                }
                else {
                    T[j].push_back(Block());
                }
            }
        }
    }

    // client
    vector<Block> res(1 << level);
    for (int i = 0; i < sqrt_n; i++) {
        T[i].insert(T[i].end(), Q[i].begin(), Q[i].end());
        auto it = stable_partition(T[i].begin(), T[i].end(), [](const Block& a) {
            return a.index != -1;
        });
        T[i].erase(it, T[i].end());
        for (int j = 0; j < T[i].size(); j++) {
            int pos = Permutate(level, Rank(T[i][j].index, level));
            res[pos] = T[i][j];
        }
    }

    blocks.swap(res);
    bandwidth += 7 * n * Block::GetBlockSize(true);
}

void RankOram::PrintOram() {
    cout << "in client:" << endl;
    for (int i = 0; i <= max_level; i++) {
        if (is_full[i]) {
            cout << "level " << i << ": " << endl;
            for (int j = 0; j < client[i].size(); j++) {
                cout << client[i][j] << ", ";
            }
            cout << endl;
            for (int j = 0; j < sentinel[i].size(); j++) {
                cout << sentinel[i][j] << ", ";
            }
            cout << endl;
        }
    }
    cout << "in server:" << endl;
    for (int i = 0; i <= max_level; i++) {
        if (is_full[i]) {
            cout << "level " << i << ": " << endl;
            for (int j = 0; j < (1 << i); j++) {
                string holder = "";
                for (int k = 0; k < Block::DATA_SIZE; k++) {
                    holder += static_cast<char>(server[i][j].data[k]);
                    holder += " ";
                }
                cout << "key: " << server[i][j].index << ", value: " << holder << ", is_delete: " << server[i][j].is_delete << endl;
            }
        }
    }
}

int RankOram::GetMaxLevel() {
    return this->max_level;
}

int RankOram::GetBandwidth() {
    return this->bandwidth;
}

void RankOram::ClearBandwidth() {
    this->bandwidth = 0;
}
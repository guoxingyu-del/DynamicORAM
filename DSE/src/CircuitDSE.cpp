#include "CircuitDSE.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <sys/time.h>
#include "Utils.h"

CircuitDSE::CircuitDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, 
                        int bucket_size, int capacity_blocks, set<pair<string, int>>& database) {
    this->storage = storage;
    this->rand_gen = rand_gen;
    this->bucket_size = bucket_size;
    this->capacity_blocks = capacity_blocks;
    this->num_blocks = database.size();
    this->num_levels = ceil(log10(capacity_blocks) / log10(2));
    this->num_buckets = pow(2, num_levels)-1;
    if (this->num_buckets*this->bucket_size < this->capacity_blocks) {
        throw new runtime_error("Not enough space for the acutal number of blocks.");
    }
    this->num_leaves = pow(2, num_levels-1);
    Bucket::ResetState();
    Bucket::SetMaxSize(bucket_size);
    this->rand_gen->SetBound(num_leaves);
    this->storage->SetCapacity(num_buckets);
    this->stash = vector<Block>(CONST_COEFFICIENT * num_levels, Block());
    
    vector<Block> tmp_blocks;

    for (auto it = database.begin(); it != database.end(); it++) {
        string keyword = it->first;
        position_map[keyword].first++;
        int leaf = this->rand_gen->GetRandomLeaf(keyword, position_map[keyword].first, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, position_map[keyword].first);
        tmp_blocks.emplace_back(Block(hashkey, it->second, leaf));
    }

    for(int i = num_buckets - 1; i >= 0; i--) {

        Bucket init_bkt = Bucket();
        int cur_count = 0;

        for (auto it = tmp_blocks.begin(); it != tmp_blocks.end(); it++) {
            int leaf_idx = it->leaf_id + (1 << (num_levels - 1)) - 1;
            while (leaf_idx >= i) {
                if (leaf_idx == i) {
                    init_bkt.AddBlock(*it);
                    cur_count++;
                    tmp_blocks.erase(it); it--;
                    break;
                }
                leaf_idx = (leaf_idx - 1) / 2;
            }
            if (cur_count == bucket_size) break;
        }

        for(int j = cur_count; j < bucket_size; j++){
            init_bkt.AddBlock(Block());
        }
        storage->WriteBucket(i, Bucket(init_bkt));
    }

    this->bandwidth = 0;

}

vector<int> CircuitDSE::Search(const string& keyword) {
    vector<int> res;
    for (int i = 1; i <= position_map[keyword].first; i++) {
        int leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, i);
        Block target = ReadPath(leaf, hashkey);
        res.push_back(target.document_id);
        int new_leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second + 1);
        AddStash(Block(hashkey, target.document_id, new_leaf));
        bandwidth += 2 * GetStashSize();
        Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
        Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
    }
    position_map[keyword].second++;
    return res;
}

void CircuitDSE::Add(const string& keyword, int document_id) {
    position_map[keyword].first++;
    int leaf = this->rand_gen->GetRandomLeaf(keyword, position_map[keyword].first, position_map[keyword].second);
    array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, position_map[keyword].first);
    AddStash(Block(hashkey, document_id, leaf));
    bandwidth += 2 * GetStashSize();
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
}

void CircuitDSE::Delete(const string& keyword, int document_id) {
    int nc = 0, nt = position_map[keyword].second + 1;
    for (int i = 1; i <= position_map[keyword].first; i++) {
        int leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, i);
        Block res = ReadPath(leaf, hashkey);
        if (res.document_id == document_id) {
            AddStash(Block());
            bandwidth += 2 * GetStashSize();
        }
        else {
            nc++;
            int new_leaf = this->rand_gen->GetRandomLeaf(keyword, nc, nt);
            array<int8_t, Block::KEY_SIZE> new_hashkey = this->rand_gen->GetHashKey(keyword, nc);
            AddStash(Block(new_hashkey, res.document_id, new_leaf));
            bandwidth += 2 * GetStashSize();
        }
        Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
        Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
    }
    position_map[keyword].first = nc;
    position_map[keyword].second = nt;
}

Block CircuitDSE::ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey) {
    Block res;

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->keyword == hashKey) {
            res = *it;
            *it = Block();
        }
    }
    bandwidth += 2 * GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(CircuitDSE::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize();
        for (Block& b: blocks) {
            if (b.keyword == hashKey) {
                res = b;
                b = Block();
            }
        }
        storage->WriteBucket(CircuitDSE::P(pos, i), Bucket(blocks));
        bandwidth += bucket_size * Block::GetBlockSize();
    }

    return res;
}

void CircuitDSE::Evict(int pos) {
    auto target = PrepareTarget(pos);
    Block hold, towrite;
    int dest = -1;
    for (int i = -1; i < num_levels; i++) {
        if (i != -1 && hold.document_id != -1 && i == dest) {
            towrite = Block(hold); hold = Block();
            dest = -1;
        }
        array<int8_t, Block::KEY_SIZE> blockHashKey = target[i + 1].second;
        // read and remove deepest block in path[i]
        if (i == -1) {
            for (auto it = stash.begin(); it < stash.end(); it++) {
                if (it->keyword == blockHashKey && it->document_id != -1) {
                    hold = *it; *it = Block();
                }
            }
            bandwidth += 2 * GetStashSize();
            if (target[i + 1].first != -1)
                dest = target[i + 1].first;
        }
        else {
            Bucket& bucket = storage->ReadBucket(CircuitDSE::P(pos, i));
            bandwidth += bucket_size * Block::GetBlockSize();
            vector<Block>& blocks = bucket.GetBlocks();
            for (auto it = blocks.begin(); it != blocks.end(); it++) {
                if (it->keyword == blockHashKey && it->document_id != -1) {
                    hold = *it; bucket.RemoveBlock(*it);
                }
            }
            if (target[i + 1].first != -1)
                dest = target[i + 1].first;
            if (towrite.document_id != -1) {
                bucket.AddBlock(towrite);
                towrite = Block();
            }
            storage->WriteBucket(CircuitDSE::P(pos, i), bucket);
            bandwidth += bucket_size * Block::GetBlockSize();
        }
    }
}

int CircuitDSE::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (this->num_levels - level - 1));
}

vector<pair<int, array<int8_t, Block::KEY_SIZE>>> CircuitDSE::PrepareDeepest(int leaf_position) {
    const int table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

    vector<pair<int, array<int8_t, Block::KEY_SIZE>>> deepest;
    int src = -2, goal = -1;
    array<int8_t, Block::KEY_SIZE> blockHashKey;
    for (auto it = stash.begin(); it != stash.end(); it++) {
        // client
        if (it->document_id == -1) {
            continue;
        }
        int tmp_leaf = it->leaf_id ^ leaf_position;
        tmp_leaf = Utils::ReverseBits(tmp_leaf, num_levels - 1);
        int num = num_levels - 1;
        if (tmp_leaf != 0) {
            unsigned int m = tmp_leaf & -tmp_leaf;
            num = min(num, table[(m * 0x077CB531U) >> 27]);
        }
        if (num > goal) {
            goal = num; src = -1;
            blockHashKey = it->keyword;
        }
    }
    bandwidth += GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        array<int8_t, Block::KEY_SIZE> tmp;
        tmp.fill(-1);
        deepest.push_back(make_pair(-2, tmp));
        if (goal >= i) {
            // src = -1 stands for coming from Stash
            deepest[i].first = src;
            deepest[i].second = blockHashKey;
        }
        vector<Block>& blocks = storage->ReadBucket(CircuitDSE::P(leaf_position, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize();
        for (Block& b : blocks) {
            // client
            if (b.document_id == -1) {
                continue;
            }
            int tmp_leaf = b.leaf_id ^ leaf_position;
            tmp_leaf = Utils::ReverseBits(tmp_leaf, num_levels - 1);
            int num = num_levels - 1;
            if (tmp_leaf != 0) {
                unsigned int m = tmp_leaf & -tmp_leaf;
                num = min(num, table[(m * 0x077CB531U) >> 27]);
            }
            if (num > goal) {
                goal = num; src = i;
                blockHashKey = b.keyword;
            }
        }
    }
    return deepest;
}

vector<pair<int, array<int8_t, Block::KEY_SIZE>>> CircuitDSE::PrepareTarget(int leaf_position) {
    vector<pair<int, array<int8_t, Block::KEY_SIZE>>> deepest = PrepareDeepest(leaf_position);
    int dest = -1, src = -2;
    array<int8_t, Block::KEY_SIZE> blockHashKey;
    vector<pair<int, array<int8_t, Block::KEY_SIZE>>> target;
    for (int level = num_levels - 1; level >= -1; level--) {
        array<int8_t, Block::KEY_SIZE> tmp;
        tmp.fill(-1);
        target.push_back(make_pair(-1, tmp));
    }
    for (int level = num_levels - 1; level >= -1; level--) {
        if (level == src) {
            target[level + 1].first = dest;
            target[level + 1].second = blockHashKey;
            dest = -1; src = -2;
        }
        if (level == -1) {
            break;
        }
        Bucket& bucket = storage->ReadBucket(CircuitDSE::P(leaf_position, level));
        bandwidth += bucket_size * Block::GetBlockSize();
        if (((dest == -1 && bucket.HasEmpty()) || target[level + 1].first != -1) && deepest[level].first != -2) {
            src = deepest[level].first; dest = level;
            blockHashKey = deepest[level].second;
        }
    }
    return target;
}

void CircuitDSE::AddStash(const Block& block) {
    bool flag = false;
    for (Block &b : stash) {
        if (!flag && b.document_id == -1) {
            b = Block(block);
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("CircuitDSE stash is overflow.");
    }
}

void CircuitDSE::DelStash(int targetIndex) {
    stash[targetIndex] = Block();
}
    
int CircuitDSE::GetNumLeaves() {
    return this->num_leaves;

}

int CircuitDSE::GetNumLevels() {
    return this->num_levels;

}

int CircuitDSE::GetNumBlocksCapacity() {
    return this->capacity_blocks;

}

int CircuitDSE::GetNumBlocks() { 
    return this->num_blocks;
}

void CircuitDSE::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int CircuitDSE::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* CircuitDSE::GetRandForOram() {
    return this->rand_gen;
}

int CircuitDSE::GetBandwidth() {
    return this->bandwidth;
}

int CircuitDSE::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize();
}

int CircuitDSE::GetStashMetaSize() {
    return (this->stash).size() * sizeof(int32_t);
}

void CircuitDSE::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* CircuitDSE::GetUntrustedStorage() {
    return this->storage;
}

int CircuitDSE::GetRealStashSize() {
    int res = 0;
    for (int i = 0; i < stash.size(); i++) {
        if (stash[i].document_id != -1) res++;
    }
    return res * Block::GetBlockSize();
}
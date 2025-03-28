#include "DynamicCircuitOPQ.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "Utils.h"
#include <algorithm>

DynamicCircuitOPQ::DynamicCircuitOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
                                            int bucket_size, int capacity_blocks) {
    this->storage = storage;
    this->rand_gen = rand_gen;
    this->bucket_size = bucket_size;

    this->num_blocks = capacity_blocks + DEFAULT_MIN_NUM_BLOCKS;
    this->num_levels = ceil(log10(this->num_blocks) / log10(2));
    this->max_level = this->num_levels - 1;
    this->full_offset = pow(2, max_level) - 1;
    this->num_leaves = this->num_blocks - pow(2, max_level);
    this->max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);
    this->num_buckets = full_offset + this->num_leaves;

    if (this->num_buckets*this->bucket_size < this->num_blocks) {
        throw new runtime_error("Not enough space for the acutal number of blocks.");
    }

    Bucket::ResetState();
    Bucket::SetMaxSize(bucket_size);
    // the leaf number should be the maximum virtual level
    this->rand_gen->SetBound(pow(2, MAX_VIRTUAL_LEVEL));
    this->storage->SetCapacity(pow(2, num_levels) - 1);
    this->stash.resize(CONST_COEFFICIENT * num_levels);
    
    for (int i = 0; i < this->num_blocks; i++){
        position_map[i] = rand_gen->GetRandomLeaf();
    }

    for(int i = 0; i < num_buckets; i++){
        Bucket init_bkt = Bucket();
        for(int j = 0; j < bucket_size; j++){
            init_bkt.AddBlock(Block());
        }
        if (i < pow(2, max_level) - 1) {
            storage->WriteBucket(i, Bucket(init_bkt));
        }
        else {
            int pos = Utils::ReverseBits(i - full_offset, max_level) + full_offset;
            storage->WriteBucket(pos, Bucket(init_bkt));
        }
    }

    this->bandwidth = 0;

}

void DynamicCircuitOPQ::Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    bandwidth += 2 * GetStashSize();
    AddStorage();
    int left_leaf = max_leaf << (MAX_VIRTUAL_LEVEL - max_level);
    int right_leaf = ((max_leaf + GetNumLevesInLevel(max_level - 1)) % GetNumLevesInLevel(max_level)) << (MAX_VIRTUAL_LEVEL - max_level);
    Evict(left_leaf);
    Evict(right_leaf);
    UpdateMax(left_leaf);
    UpdateMax(right_leaf);
}

Block DynamicCircuitOPQ::ExtractMax() {
    Block max_block = FindMax();
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadDelPath(max_block.index, data);
    int left_leaf = max_leaf << (MAX_VIRTUAL_LEVEL - max_level);
    int right_leaf = oldLeaf;
    Evict(left_leaf);
    Evict(right_leaf);
    DelStorage();
    UpdateMax(left_leaf);
    UpdateMax(right_leaf);
    return max_block;
}

int DynamicCircuitOPQ::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            *it = Block();
        }
    }
    bandwidth += 2 * GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicCircuitOPQ::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index == blockIndex) {
                data = b.data;
                b = Block();
            }
        }
        storage->WriteBucket(DynamicCircuitOPQ::P(pos, i), blocks);
        bandwidth += bucket_size * Block::GetBlockSize(false);
    }

    return pos;
}

void DynamicCircuitOPQ::Evict(int pos) {
    auto target = PrepareTarget(pos);
    Block hold, towrite;
    int dest = -1;
    for (int i = -1; i < num_levels; i++) {
        if (i != -1 && hold.index != -1 && i == dest) {
            towrite = Block(hold); hold = Block();
            dest = -1;
        }
        int blockId = target[i + 1].second;
        // read and remove deepest block in path[i]
        if (i == -1) {
            for (auto it = stash.begin(); it < stash.end(); it++) {
                if (it->index == blockId && blockId != -1) {
                    hold = *it; *it = Block();
                }
            }
            bandwidth += 2 * GetStashSize();
            if (target[i + 1].first != -1)
                dest = target[i + 1].first;
        }
        else {
            Bucket& bucket = storage->ReadBucket(DynamicCircuitOPQ::P(pos, i));
            bandwidth += bucket_size * Block::GetBlockSize(false);
            vector<Block>& blocks = bucket.GetBlocks();
            for (auto it = blocks.begin(); it != blocks.end(); it++) {
                if (it->index == blockId && blockId != -1) {
                    hold = *it; bucket.RemoveBlock(*it);
                }
            }
            if (target[i + 1].first != -1)
                dest = target[i + 1].first;
            if (towrite.index != -1) {
                bucket.AddBlock(towrite);
                towrite = Block();
            }
            storage->WriteBucket(DynamicCircuitOPQ::P(pos, i), bucket);
            bandwidth += bucket_size * Block::GetBlockSize(false);
        }
    }
}

void DynamicCircuitOPQ::UpdateMax(int leaf) {
    for (int l = num_levels-1; l >= 0; l--) {
        int p = DynamicCircuitOPQ::P(leaf, l);
        auto potential_maxs = std::vector<const Block*>();

        Bucket bucket_on_path = storage->ReadBucket(p);
        bandwidth += bucket_size * Block::GetBlockSize(false);
        const std::vector<Block>& blocks_on_path = bucket_on_path.GetBlocks();
        if (blocks_on_path.size() > 0) {
            const Block& max_block_on_path =
                *std::max_element(blocks_on_path.begin(), blocks_on_path.end());
            potential_maxs.push_back(&max_block_on_path);
        }
        if (l != num_levels-1) {
            Block child1_block = storage->ReadSubtreeMax(2*p+1);
            Block child2_block = storage->ReadSubtreeMax(2*p+2);
            bandwidth += 2 * Block::GetBlockSize(false);

            potential_maxs.push_back(&child1_block);
            potential_maxs.push_back(&child2_block);
        }

        // given a vector of pointers, get the max of what those pointers reference
        if (potential_maxs.size() > 0) {
            int max_index = 0;
            for (int i = 1; i < potential_maxs.size(); i++) {
                if (potential_maxs.at(i)->index > potential_maxs.at(max_index)->index) {
                    max_index = i;
                }
            }
            storage->WriteSubtreeMax(p, *potential_maxs.at(max_index));
            bandwidth += Block::GetBlockSize(false);
        } else {
            storage->WriteSubtreeMax(p, Block());
            bandwidth += Block::GetBlockSize(false);
        }
    }
}

Block DynamicCircuitOPQ::FindMax() {
    Block max_block = storage->ReadSubtreeMax(DynamicCircuitOPQ::P(0,0));
    bandwidth += Block::GetBlockSize(false);
    if (stash.size() > 0) {
        const Block& stash_max = *std::max_element(stash.begin(), stash.end());
        bandwidth += GetStashSize();
        // assume the key is the priority
        if (stash_max.index > max_block.index) {
            max_block = stash_max;
        }
    }
    return max_block;
}
int DynamicCircuitOPQ::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (MAX_VIRTUAL_LEVEL - level));
}

void DynamicCircuitOPQ::AddStash(const Block& block) {
    bool flag = false;
    for (Block &b : stash) {
        if (!flag && b.index == -1) {
            b = Block(block);
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("DynamicCircuitOPQ stash is overflow.");
    }
}

void DynamicCircuitOPQ::DelStash() {
    bool flag = false;
    for (Block& b : stash) {
        if (!flag && b.index == -1) {
            b = Block(stash.back());
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("DynamicCircuitOPQ stash is overflow.");
    }
    stash.pop_back();
}

void DynamicCircuitOPQ::AddStorage() { 
    num_blocks++;
    // Change maxLeaf
    if (Utils::IsPowerOfTwoPlusOne(num_blocks)) {
        num_levels++; max_level++; num_buckets++;
        num_leaves = 1;
        max_leaf = num_leaves - 1;
        full_offset = pow(2, max_level) - 1;

        storage->ChangeCapacity(pow(2, num_levels) - 1);
        for (int i = 0; i < CONST_COEFFICIENT; i++) 
            stash.emplace_back(Block());
    } 
    else {
        num_leaves++; num_buckets++;
        max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);  
    }
    Bucket init_bkt = Bucket();
    for(int j = 0; j < bucket_size; j++){
        init_bkt.AddBlock(Block());
    }
    storage->WriteBucket(max_leaf + full_offset, Bucket(init_bkt));
}

void DynamicCircuitOPQ::DelStorage() {
    vector<Block>& blocks = storage->ReadBucket(max_leaf + full_offset).GetBlocks();
    bandwidth += bucket_size * Block::GetBlockSize(false);
    int tmp_leaf = (max_leaf + full_offset - 1) >> 1;
    for (int i = num_levels - 2; i >= 0; i--) {
        Bucket& tmp_bucket = storage->ReadBucket(tmp_leaf);
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (int j = 0; j < bucket_size; j++) {
            if (tmp_bucket.AddBlock(blocks[j])) {
                blocks[j] = Block();
            }
        }
        storage->WriteBucket(tmp_leaf, tmp_bucket);
        bandwidth += bucket_size * Block::GetBlockSize(false);
        tmp_leaf = (tmp_leaf - 1) / 2;
    }
    for (Block& b: blocks) {
        AddStash(b);
        bandwidth += 2 * GetStashSize();
    }
    blocks.clear();
    
    if (Utils::IsPowerOfTwoPlusOne(num_blocks)) {
        num_levels--; max_level--; num_buckets--;
        num_leaves = pow(2, max_level);
        max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);
        full_offset = pow(2, max_level) - 1;

        storage->ChangeCapacity(pow(2, num_levels) - 1);
        for (int i = 0; i < CONST_COEFFICIENT; i++) {
            bandwidth += 2 * GetStashSize();
            DelStash();
        }
    } 
    else {
        num_leaves--; num_buckets--;
        max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);
    }

    num_blocks--;
}

vector<pair<int, int>> DynamicCircuitOPQ::PrepareDeepest(int leaf_position) {
    const int table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

    int P_max_level = P(leaf_position, max_level);
    bool is_overflow = false;
    if (Utils::ReverseBits(P_max_level - full_offset, max_level) >= num_leaves) {
        is_overflow = true;
    }

    vector<pair<int, int>> deepest;
    int src = -2, goal = -1;
    int blockId = 0;
    for (auto it = stash.begin(); it != stash.end(); it++) {
        // client
        if (it->index == -1) {
            continue;
        }
        int tmp_leaf = position_map[it->index] ^ leaf_position;
        tmp_leaf = Utils::ReverseBits(tmp_leaf, MAX_VIRTUAL_LEVEL);
        int num = num_levels - 1;
        if (tmp_leaf != 0) {
            unsigned int m = tmp_leaf & -tmp_leaf;
            num = min(num, table[(m * 0x077CB531U) >> 27]);
        }
        if (num > goal) {
            goal = num; src = -1;
            blockId = it->index;
        }
    }
    bandwidth += GetStashMetaSize();

    for (int i = 0; i < num_levels; i++) {
        deepest.push_back(make_pair(-2, -1));
        if (i == max_level && is_overflow) continue;
        if (goal >= i) {
            // src = -1 stands for coming from Stash
            deepest[i].first = src;
            deepest[i].second = blockId;
        }
        vector<int32_t> indexes = storage->ReadBucketMetaData(DynamicCircuitOPQ::P(leaf_position, i));
        bandwidth += bucket_size * sizeof(int32_t);
        for (int ind : indexes) {
            // client
            if (ind == -1) {
                continue;
            }
            int tmp_leaf = position_map[ind] ^ leaf_position;
            tmp_leaf = Utils::ReverseBits(tmp_leaf, MAX_VIRTUAL_LEVEL);
            int num = num_levels - 1;
            if (tmp_leaf != 0) {
                unsigned int m = tmp_leaf & -tmp_leaf;
                num = min(num, table[(m * 0x077CB531U) >> 27]);
            }
            if (num > goal) {
                goal = num; src = i;
                blockId = ind;
            }
        }
    }
    return deepest;
}

vector<pair<int, int>> DynamicCircuitOPQ::PrepareTarget(int leaf_position) {
    vector<pair<int, int>> deepest = PrepareDeepest(leaf_position);
    int dest = -1, src = -2;
    int blockId = 0;
    vector<pair<int, int>> target;
    for (int level = num_levels - 1; level >= -1; level--) {
        target.push_back(make_pair(-1, -1));
    }
    for (int level = num_levels - 1; level >= -1; level--) {
        if (level == src) {
            target[level + 1].first = dest;
            target[level + 1].second = blockId;
            dest = -1; src = -2;
        }
        if (level == -1) {
            break;
        }
        Bucket& bucket = storage->ReadBucket(DynamicCircuitOPQ::P(leaf_position, level));
        bandwidth += bucket_size * Block::GetBlockSize(false);
        if (((dest == -1 && bucket.HasEmpty()) || target[level + 1].first != -1) && deepest[level].first != -2) {
            src = deepest[level].first; dest = level;
            blockId = deepest[level].second;
        }
    }
    return target;
}
    
int DynamicCircuitOPQ::GetNumLeaves() {
    return this->num_leaves;

}

int DynamicCircuitOPQ::GetNumLevels() {
    return this->num_levels;

}

int DynamicCircuitOPQ::GetNumBlocksCapacity() {
    return this->num_blocks;

}

int DynamicCircuitOPQ::GetNumBlocks() { 
    return this->num_blocks - DEFAULT_MIN_NUM_BLOCKS;
}

void DynamicCircuitOPQ::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int DynamicCircuitOPQ::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* DynamicCircuitOPQ::GetRandForOram() {
    return this->rand_gen;
}

int DynamicCircuitOPQ::GetBandwidth() {
    return this->bandwidth;
}

int DynamicCircuitOPQ::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

int DynamicCircuitOPQ::GetStashMetaSize() {
    return (this->stash).size() * sizeof(int32_t);
}

void DynamicCircuitOPQ::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* DynamicCircuitOPQ::GetUntrustedStorage() {
    return this->storage;
}

int DynamicCircuitOPQ::GetNumLevesInLevel(int level) {
    return 1 << level;
}

int DynamicCircuitOPQ::GetRealStashSize() {
    int res = 0;
    for (int i = 0; i < stash.size(); i++) {
        if (stash[i].index != -1) res++;
    }
    return res * Block::GetBlockSize(false);
}
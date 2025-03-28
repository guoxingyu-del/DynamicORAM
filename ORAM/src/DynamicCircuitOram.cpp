#include "DynamicCircuitOram.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "Utils.h"

DynamicCircuitOram::DynamicCircuitOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
                                            int bucket_size, int capacity_blocks) {
    this->storage = storage;
    this->rand_gen = rand_gen;
    this->bucket_size = bucket_size;

    this->num_blocks = max(capacity_blocks, DEFAULT_MIN_NUM_BLOCKS);
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
            // initialize only existing leaf nodes
            int pos = Utils::ReverseBits(i - full_offset, max_level) + full_offset;
            storage->WriteBucket(pos, Bucket(init_bkt));
        }
    }

    this->bandwidth = 0;

}

array<int8_t, Block::DATA_SIZE> DynamicCircuitOram::Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    array<int8_t, Block::DATA_SIZE> data;
    ReadPath(blockIndex, op, newdata, data);
    Evict(rand_gen->GetRandomLeaf() % ((1 << MAX_VIRTUAL_LEVEL) / 2));
    Evict(rand_gen->GetRandomLeaf() % ((1 << MAX_VIRTUAL_LEVEL) / 2) + (1 << MAX_VIRTUAL_LEVEL) / 2);
    return data;
}

void DynamicCircuitOram::Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    bandwidth += 2 * GetStashSize();
    AddStorage();
    Evict(max_leaf << (MAX_VIRTUAL_LEVEL - max_level));
    Evict(((max_leaf + GetNumLevesInLevel(max_level - 1)) % GetNumLevesInLevel(max_level)) << (MAX_VIRTUAL_LEVEL - max_level));
}

array<int8_t, Block::DATA_SIZE> DynamicCircuitOram::Delete(int blockIndex) {
    array<int8_t, Block::DATA_SIZE> data;
    ReadDelPath(blockIndex, data);
    Evict(rand_gen->GetRandomLeaf() % ((1 << MAX_VIRTUAL_LEVEL) / 2));
    Evict(rand_gen->GetRandomLeaf() % ((1 << MAX_VIRTUAL_LEVEL) / 2) + (1 << MAX_VIRTUAL_LEVEL) / 2);
    DelStorage();
    return data;
}

int DynamicCircuitOram::ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];
    position_map[blockIndex] = rand_gen->GetRandomLeaf();

    Block targetBlock;
    for (Block& b : stash) {
        if (b.index == blockIndex) {
            targetBlock = Block(b);
            b = Block();
        }
    }
    bandwidth += 2 * GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicCircuitOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index == blockIndex) {
                targetBlock = Block(b);
                b = Block();
            }
        }
        storage->WriteBucket(DynamicCircuitOram::P(pos, i), Bucket(blocks));
        bandwidth += bucket_size * Block::GetBlockSize(false);
    }

    data = targetBlock.data;
    if (op == WRITE) {
        targetBlock.data = newdata;
    }
    targetBlock.leaf_id = position_map[blockIndex];

    AddStash(targetBlock);
    bandwidth += 2 * GetStashSize();
    return pos;
}

int DynamicCircuitOram::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            *it = Block();
        }
    }
    bandwidth += 2 * GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicCircuitOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index == blockIndex) {
                data = b.data;
                b = Block();
            }
        }
        storage->WriteBucket(DynamicCircuitOram::P(pos, i), blocks);
        bandwidth += bucket_size * Block::GetBlockSize(false);
    }

    return pos;
}

void DynamicCircuitOram::Evict(int pos) {
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
            Bucket& bucket = storage->ReadBucket(DynamicCircuitOram::P(pos, i));
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
            storage->WriteBucket(DynamicCircuitOram::P(pos, i), bucket);
            bandwidth += bucket_size * Block::GetBlockSize(false);
        }
    }
}

int DynamicCircuitOram::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (MAX_VIRTUAL_LEVEL - level));
}

void DynamicCircuitOram::AddStash(const Block& block) {
    bool flag = false;
    for (Block &b : stash) {
        if (!flag && b.index == -1) {
            b = Block(block);
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("DynamicCircuitOram stash is overflow.");
    }
}

void DynamicCircuitOram::DelStash() {
    bool flag = false;
    for (Block& b : stash) {
        if (!flag && b.index == -1) {
            b = Block(stash.back());
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("DynamicCircuitOram stash is overflow.");
    }
    stash.pop_back();
}

void DynamicCircuitOram::AddStorage() { 
    num_blocks++;
    // Change maxLeaf
    if (Utils::IsPowerOfTwoPlusOne(num_blocks)) {
        num_levels++; max_level++; num_buckets++;
        num_leaves = 1;
        max_leaf = num_leaves - 1;
        full_offset = pow(2, max_level) - 1;

        // add a level down
        storage->ChangeCapacity(pow(2, num_levels) - 1);
        // stash expands CONST_COEFFICIENT
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

void DynamicCircuitOram::DelStorage() {
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

vector<pair<int, int>> DynamicCircuitOram::PrepareDeepest(int leaf_position) {
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
        vector<int32_t> indexes = storage->ReadBucketMetaData(DynamicCircuitOram::P(leaf_position, i));
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

vector<pair<int, int>> DynamicCircuitOram::PrepareTarget(int leaf_position) {
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
        Bucket& bucket = storage->ReadBucket(DynamicCircuitOram::P(leaf_position, level));
        bandwidth += bucket_size * Block::GetBlockSize(false);
        if (((dest == -1 && bucket.HasEmpty()) || target[level + 1].first != -1) && deepest[level].first != -2) {
            src = deepest[level].first; dest = level;
            blockId = deepest[level].second;
        }
    }
    return target;
}
    
int DynamicCircuitOram::GetNumLeaves() {
    return this->num_leaves;

}

int DynamicCircuitOram::GetNumLevels() {
    return this->num_levels;

}

int DynamicCircuitOram::GetNumBlocksCapacity() {
    return this->num_blocks;

}

int DynamicCircuitOram::GetNumBlocks() { 
    return this->num_blocks - DEFAULT_MIN_NUM_BLOCKS;
}

void DynamicCircuitOram::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int DynamicCircuitOram::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* DynamicCircuitOram::GetRandForOram() {
    return this->rand_gen;
}

int DynamicCircuitOram::GetBandwidth() {
    return this->bandwidth;
}

int DynamicCircuitOram::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

int DynamicCircuitOram::GetStashMetaSize() {
    return (this->stash).size() * sizeof(int32_t);
}

void DynamicCircuitOram::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* DynamicCircuitOram::GetUntrustedStorage() {
    return this->storage;
}

int DynamicCircuitOram::GetNumLevesInLevel(int level) {
    return 1 << level;
}

int DynamicCircuitOram::GetRealStashSize() {
    int res = 0;
    for (int i = 0; i < stash.size(); i++) {
        if (stash[i].index != -1) res++;
    }
    return res * Block::GetBlockSize(false);
}
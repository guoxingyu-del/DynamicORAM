#include "CircuitOram.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <sys/time.h>
#include "Utils.h"

CircuitOram::CircuitOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks) {
    this->storage = storage;
    this->rand_gen = rand_gen;
    this->bucket_size = bucket_size;
    this->capacity_blocks = capacity_blocks;
    this->num_blocks = 0;
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
    
    for (int i = 0; i < this->capacity_blocks; i++) {
        position_map[i] = rand_gen->GetRandomLeaf();
    }

    for(int i = 0; i < num_buckets; i++){

        Bucket init_bkt = Bucket();
        for(int j = 0; j < bucket_size; j++){
            init_bkt.AddBlock(Block());
        }
        storage->WriteBucket(i, Bucket(init_bkt));
    }

    this->bandwidth = 0;

}

array<int8_t, Block::DATA_SIZE> CircuitOram::Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    array<int8_t, Block::DATA_SIZE> data;
    ReadPath(blockIndex, op, newdata, data);
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
    return data;
}

void CircuitOram::Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    bandwidth += 2 * GetStashSize();
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
}

array<int8_t, Block::DATA_SIZE> CircuitOram::Delete(int blockIndex) {
    array<int8_t, Block::DATA_SIZE> data;
    ReadDelPath(blockIndex, data);
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2));
    Evict(rand_gen->GetRandomLeaf() % (GetNumLeaves() / 2) + GetNumLeaves() / 2);
    return data;
}

int CircuitOram::ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, 
                            array<int8_t, Block::DATA_SIZE>& data) {
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
        vector<Block>& blocks = storage->ReadBucket(CircuitOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index == blockIndex) {
                targetBlock = Block(b);
                b = Block();
            }
        }
        storage->WriteBucket(CircuitOram::P(pos, i), Bucket(blocks));
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

int CircuitOram::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            *it = Block();
        }
    }
    bandwidth += 2 * GetStashSize();

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(CircuitOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index == blockIndex) {
                data = b.data;
                b = Block();
            }
        }
        storage->WriteBucket(CircuitOram::P(pos, i), blocks);
        bandwidth += bucket_size * Block::GetBlockSize(false);
    }

    return pos;
}

void CircuitOram::Evict(int pos) {
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
            Bucket& bucket = storage->ReadBucket(CircuitOram::P(pos, i));
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
            storage->WriteBucket(CircuitOram::P(pos, i), bucket);
            bandwidth += bucket_size * Block::GetBlockSize(false);
        }
    }
}

int CircuitOram::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (this->num_levels - level - 1));
}

vector<pair<int, int>> CircuitOram::PrepareDeepest(int leaf_position) {
    const int table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

    vector<pair<int, int>> deepest;
    int src = -2, goal = -1;
    int blockId = 0;
    for (auto it = stash.begin(); it != stash.end(); it++) {
        // client
        if (it->index == -1) {
            continue;
        }
        int tmp_leaf = position_map[it->index] ^ leaf_position;
        tmp_leaf = Utils::ReverseBits(tmp_leaf, num_levels - 1);
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
        if (goal >= i) {
            // src = -1 stands for coming from Stash
            deepest[i].first = src;
            deepest[i].second = blockId;
        }
        vector<int32_t> indexes = storage->ReadBucketMetaData(CircuitOram::P(leaf_position, i));
        bandwidth += bucket_size * sizeof(int32_t);
        for (int32_t ind : indexes) {
            // client
            if (ind == -1) {
                continue;
            }
            int tmp_leaf = position_map[ind] ^ leaf_position;
            tmp_leaf = Utils::ReverseBits(tmp_leaf, num_levels - 1);
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

vector<pair<int, int>> CircuitOram::PrepareTarget(int leaf_position) {
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
        Bucket& bucket = storage->ReadBucket(CircuitOram::P(leaf_position, level));
        bandwidth += bucket_size * Block::GetBlockSize(false);
        if (((dest == -1 && bucket.HasEmpty()) || target[level + 1].first != -1) && deepest[level].first != -2) {
            src = deepest[level].first; dest = level;
            blockId = deepest[level].second;
        }
    }
    return target;
}

void CircuitOram::AddStash(const Block& block) {
    bool flag = false;
    for (Block &b : stash) {
        if (!flag && b.index == -1) {
            b = Block(block);
            flag = true;
        }
    }
    if (!flag) {
        throw new runtime_error("CircuitOram stash is overflow.");
    }
}

void CircuitOram::DelStash(int targetIndex) {
    stash[targetIndex] = Block();
}
    
int CircuitOram::GetNumLeaves() {
    return this->num_leaves;

}

int CircuitOram::GetNumLevels() {
    return this->num_levels;

}

int CircuitOram::GetNumBlocksCapacity() {
    return this->capacity_blocks;

}

int CircuitOram::GetNumBlocks() { 
    return this->num_blocks;
}

void CircuitOram::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int CircuitOram::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* CircuitOram::GetRandForOram() {
    return this->rand_gen;
}

int CircuitOram::GetBandwidth() {
    return this->bandwidth;
}

int CircuitOram::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

int CircuitOram::GetStashMetaSize() {
    return (this->stash).size() * sizeof(int32_t);
}

void CircuitOram::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* CircuitOram::GetUntrustedStorage() {
    return this->storage;
}

int CircuitOram::GetRealStashSize() {
    int res = 0;
    for (int i = 0; i < stash.size(); i++) {
        if (stash[i].index != -1) res++;
    }
    return res * Block::GetBlockSize(false);
}
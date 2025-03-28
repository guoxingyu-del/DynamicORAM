#include "DynamicPathOram.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "Utils.h"

DynamicPathOram::DynamicPathOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
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
    this->stash = vector<Block>();
    
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

array<int8_t, Block::DATA_SIZE> DynamicPathOram::Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadPath(blockIndex, op, newdata, data);
    Evict(oldLeaf);
    return data;
}

void DynamicPathOram::Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    AddStorage();
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
}

array<int8_t, Block::DATA_SIZE> DynamicPathOram::Delete(int blockIndex) {
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadDelPath(blockIndex, data);
    Evict(oldLeaf);
    DelStorage();
    return data;
}

int DynamicPathOram::ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];
    position_map[blockIndex] = rand_gen->GetRandomLeaf();

    Block targetBlock;
    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            targetBlock = Block(*it);
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                if (b.index == blockIndex) {
                    targetBlock = Block(b);
                }
                else {
                    stash.emplace_back(Block(b));
                }
            }
        }
    }

    data = targetBlock.data;
    if (op == WRITE) {
        targetBlock.data = newdata;
    }
    targetBlock.leaf_id = position_map[blockIndex];

    AddStash(targetBlock);
    return pos;
}

int DynamicPathOram::ReadTmPath() {
    int pos = max_leaf << (MAX_VIRTUAL_LEVEL - max_level);
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

int DynamicPathOram::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                if (b.index == blockIndex) {
                    data = b.data;
                }
                else {
                    stash.emplace_back(Block(b));
                }
            }
        }
    }

    return pos;
}

void DynamicPathOram::Evict(int pos) {
    int P_max_level = P(pos, max_level);
    bool is_overflow = false;
    if (Utils::ReverseBits(P_max_level - full_offset, max_level) >= num_leaves) {
        is_overflow = true;
    }

    for (int l = num_levels - 1; l >= 0; l--) {
        if (is_overflow) {
            is_overflow = false;
            continue;
        }

        Bucket bucket = Bucket();
        int Pxl = P(pos, l);
        int counter = 0;

        for (auto it = stash.begin(); it != stash.end(); it++) {
            if (counter >= bucket_size) {
                break;
            }
            if (Pxl == P(position_map[it->index], l) && !is_overflow) {
                bucket.AddBlock(*it);
                stash.erase(it); it--;
                counter++;
            }
        }

        while(counter < bucket_size) {
            bucket.AddBlock(Block());
            counter++;
        }

        storage->WriteBucket(Pxl, bucket);
        bandwidth += bucket_size * Block::GetBlockSize(false);
    }
}

int DynamicPathOram::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (MAX_VIRTUAL_LEVEL - level));
}

void DynamicPathOram::AddStorage() { 
    num_blocks++;
    // Change maxLeaf
    if (Utils::IsPowerOfTwoPlusOne(num_blocks)) {
        num_levels++; max_level++; num_buckets++;
        num_leaves = 1;
        max_leaf = num_leaves - 1;
        full_offset = pow(2, max_level) - 1;

        storage->ChangeCapacity(pow(2, num_levels) - 1);
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

void DynamicPathOram::DelStorage() {
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
    }
    blocks.clear();
    
    if (Utils::IsPowerOfTwoPlusOne(num_blocks)) {
        num_levels--; max_level--; num_buckets--;
        num_leaves = pow(2, max_level);
        max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);
        full_offset = pow(2, max_level) - 1;

        storage->ChangeCapacity(pow(2, num_levels) - 1);
    } 
    else {
        num_leaves--; num_buckets--;
        max_leaf = Utils::ReverseBits(num_leaves - 1, max_level);
    }

    num_blocks--;
}

void DynamicPathOram::AddStash(const Block& block) {
    if (block.index != -1) {
        this->stash.push_back(block);
    }
}
    
int DynamicPathOram::GetNumLeaves() {
    return this->num_leaves;
}

int DynamicPathOram::GetNumLevels() {
    return this->num_levels;
}

int DynamicPathOram::GetNumBlocksCapacity() {
    return this->num_blocks;
}

int DynamicPathOram::GetNumBlocks() { 
    return this->num_blocks - DEFAULT_MIN_NUM_BLOCKS;
}

void DynamicPathOram::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int DynamicPathOram::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* DynamicPathOram::GetRandForOram() {
    return this->rand_gen;
}

int DynamicPathOram::GetBandwidth() {
    return this->bandwidth;
}

int DynamicPathOram::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

void DynamicPathOram::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* DynamicPathOram::GetUntrustedStorage() {
    return this->storage;
}
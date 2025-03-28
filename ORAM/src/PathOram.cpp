#include "PathOram.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <sys/time.h>

PathOram::PathOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
                                            int bucket_size, int capacity_blocks) {
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
    this->stash = vector<Block>();
    
    for (int i = 0; i < this->capacity_blocks; i++){
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

array<int8_t, Block::DATA_SIZE> PathOram::Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadPath(blockIndex, op, newdata, data);
    Evict(oldLeaf);
    return data;
}

void PathOram::Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
}

array<int8_t, Block::DATA_SIZE> PathOram::Delete(int blockIndex) {
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadDelPath(blockIndex, data);
    Evict(oldLeaf);
    return data;
}

int PathOram::ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];
    position_map[blockIndex] = rand_gen->GetRandomLeaf();

    Block tarGetBlock;
    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            tarGetBlock = Block(*it);
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                if (b.index == blockIndex) {
                    tarGetBlock = Block(b);
                }
                else {
                    stash.emplace_back(Block(b));
                }
            }
        }
    }

    data = tarGetBlock.data;
    if (op == WRITE) {
        tarGetBlock.data = newdata;
    }
    tarGetBlock.leaf_id = position_map[blockIndex];

    AddStash(tarGetBlock);
    return pos;
}

int PathOram::ReadTmPath() {
    int pos = rand_gen->GetRandomLeaf();
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathOram::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

int PathOram::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathOram::P(pos, i)).GetBlocks();
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

void PathOram::Evict(int pos) {
    for (int l = num_levels - 1; l >= 0; l--) {
        Bucket bucket = Bucket();
        int Pxl = P(pos, l);

        int counter = 0;
        for (auto it = stash.begin(); it != stash.end(); it++) {
            if (counter >= bucket_size) {
                break;
            }
            if (Pxl == P(position_map[it->index], l)) {
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

int PathOram::P(int leaf, int level) {
    /*
    * This function should be deterministic. 
    * INPUT: leaf in range 0 to num_leaves - 1, level in range 0 to num_levels - 1. 
    * OUTPUT: Returns the location in the storage of the bucket which is at the input level and leaf.
    */
    return (1<<level) - 1 + (leaf >> (this->num_levels - level - 1));
}

void PathOram::AddStash(const Block& block) {
    if (block.index != -1) {
        this->stash.push_back(block);
    }
}
    
int PathOram::GetNumLeaves() {
    return this->num_leaves;
}

int PathOram::GetNumLevels() {
    return this->num_levels;
}

int PathOram::GetNumBlocksCapacity() {
    return this->capacity_blocks;
}

int PathOram::GetNumBlocks() { 
    return this->num_blocks;
}

void PathOram::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int PathOram::GetNumBuckets() {
    return this->num_buckets;
}

RandForOramInterface* PathOram::GetRandForOram() {
    return this->rand_gen;
}

int PathOram::GetBandwidth() {
    return this->bandwidth;
}

int PathOram::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

void PathOram::ClearBandwidth() {
    this->bandwidth = 0;
}
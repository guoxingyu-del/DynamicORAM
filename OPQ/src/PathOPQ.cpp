#include "PathOPQ.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <sys/time.h>

PathOPQ::PathOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
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

void PathOPQ::Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
    UpdateMax(oldLeaf);
}

Block PathOPQ::ExtractMax() {
    Block max_block = FindMax();
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadDelPath(max_block.index, data);
    Evict(oldLeaf);
    UpdateMax(oldLeaf);
    return max_block;
}

int PathOPQ::ReadTmPath() {
    int pos = rand_gen->GetRandomLeaf();
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathOPQ::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

int PathOPQ::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathOPQ::P(pos, i)).GetBlocks();
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

void PathOPQ::Evict(int pos) {
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

void PathOPQ::UpdateMax(int leaf) {
    for (int l = num_levels-1; l >= 0; l--) {
        int p = PathOPQ::P(leaf, l);
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

Block PathOPQ::FindMax() {
    Block max_block = storage->ReadSubtreeMax(PathOPQ::P(0,0));
    bandwidth += Block::GetBlockSize(false);
    if (stash.size() > 0) {
        const Block& stash_max = *std::max_element(stash.begin(), stash.end());
        // assume the key is the priority
        if (stash_max.index > max_block.index) {
            max_block = stash_max;
        }
    }
    return max_block;
}

int PathOPQ::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (this->num_levels - level - 1));
}

void PathOPQ::AddStash(const Block& block) {
    if (block.index != -1)
        this->stash.push_back(block);
}
    
int PathOPQ::GetNumLeaves() {
    return this->num_leaves;
}

int PathOPQ::GetNumLevels() {
    return this->num_levels;
}

int PathOPQ::GetNumBlocksCapacity() {
    return this->capacity_blocks;
}

int PathOPQ::GetNumBlocks() { 
    return this->num_blocks;
}

void PathOPQ::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int PathOPQ::GetNumBuckets() {
    return this->num_buckets;
}

RandForOramInterface* PathOPQ::GetRandForOram() {
    return this->rand_gen;
}

int PathOPQ::GetBandwidth() {
    return this->bandwidth;
}

int PathOPQ::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

void PathOPQ::ClearBandwidth() {
    this->bandwidth = 0;
}
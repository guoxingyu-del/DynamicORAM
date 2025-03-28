#include "DynamicPathOPQ.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "Utils.h"
#include <algorithm>

DynamicPathOPQ::DynamicPathOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
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
            int pos = Utils::ReverseBits(i - full_offset, max_level) + full_offset;
            storage->WriteBucket(pos, Bucket(init_bkt));
        }
    }

    this->bandwidth = 0;

}

void DynamicPathOPQ::Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    position_map[blockIndex] = rand_gen->GetRandomLeaf();
    AddStash(Block(position_map[blockIndex], blockIndex, newdata));
    AddStorage();
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
    UpdateMax(oldLeaf);
}

Block DynamicPathOPQ::ExtractMax() {
    Block max_block = FindMax();
    array<int8_t, Block::DATA_SIZE> data;
    int oldLeaf = ReadDelPath(max_block.index, data);
    Evict(oldLeaf);
    DelStorage();
    UpdateMax(oldLeaf);
    return max_block;
}

int DynamicPathOPQ::ReadTmPath() {
    int pos = max_leaf << (MAX_VIRTUAL_LEVEL - max_level);
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathOPQ::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize(false);
        for (Block& b: blocks) {
            if (b.index != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

int DynamicPathOPQ::ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data) {
    int pos = position_map[blockIndex];

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->index == blockIndex) {
            data = it->data;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathOPQ::P(pos, i)).GetBlocks();
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

void DynamicPathOPQ::Evict(int pos) {
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

void DynamicPathOPQ::UpdateMax(int leaf) {
    for (int l = num_levels-1; l >= 0; l--) {
        int p = DynamicPathOPQ::P(leaf, l);
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

Block DynamicPathOPQ::FindMax() {
    Block max_block = storage->ReadSubtreeMax(DynamicPathOPQ::P(0,0));
    bandwidth += Block::GetBlockSize(false);
    if (stash.size() > 0) {
        const Block& stash_max = *std::max_element(stash.begin(), stash.end());
        // assume the key is the priotrity
        if (stash_max.index > max_block.index) {
            max_block = stash_max;
        }
    }
    return max_block;
}

int DynamicPathOPQ::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (MAX_VIRTUAL_LEVEL - level));
}

void DynamicPathOPQ::AddStorage() { 
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

void DynamicPathOPQ::DelStorage() {
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

void DynamicPathOPQ::AddStash(const Block& block) {
    if (block.index != -1)
        this->stash.push_back(block);
}
    
int DynamicPathOPQ::GetNumLeaves() {
    return this->num_leaves;
}

int DynamicPathOPQ::GetNumLevels() {
    return this->num_levels;
}

int DynamicPathOPQ::GetNumBlocksCapacity() {
    return this->num_blocks;
}

int DynamicPathOPQ::GetNumBlocks() { 
    return this->num_blocks - DEFAULT_MIN_NUM_BLOCKS;
}

void DynamicPathOPQ::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int DynamicPathOPQ::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* DynamicPathOPQ::GetRandForOram() {
    return this->rand_gen;
}

int DynamicPathOPQ::GetBandwidth() {
    return this->bandwidth;
}

int DynamicPathOPQ::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize(false);
}

void DynamicPathOPQ::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* DynamicPathOPQ::GetUntrustedStorage() {
    return this->storage;
}
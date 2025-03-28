#include "DynamicPathDSE.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "Utils.h"

DynamicPathDSE::DynamicPathDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
                                int bucket_size, int capacity_blocks, set<pair<string, int>>& database) {
    this->storage = storage;
    this->rand_gen = rand_gen;
    this->bucket_size = bucket_size;

    this->num_blocks = max(capacity_blocks, DynamicPathDSE::DEFAULT_MIN_NUM_BLOCKS);
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
    
    vector<Block> tmp_blocks;

    for (auto it = database.begin(); it != database.end(); it++) {
        string keyword = it->first;
        position_map[keyword].first++;
        int leaf = this->rand_gen->GetRandomLeaf(keyword, position_map[keyword].first, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, position_map[keyword].first);
        tmp_blocks.emplace_back(Block(hashkey, it->second, leaf));
    }

    for(int i = num_buckets - 1; i >= 0; i--) {
        int pos = i;

        if (i >= pow(2, max_level) - 1) {
            pos = Utils::ReverseBits(i - full_offset, max_level) + full_offset;
        }

        Bucket init_bkt = Bucket();
        int cur_count = 0;

        for (auto it = tmp_blocks.begin(); it != tmp_blocks.end(); it++) {
            int leaf_idx = P(it->leaf_id, max_level);
            while (leaf_idx >= pos) {
                if (leaf_idx == pos) {
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
        storage->WriteBucket(pos, Bucket(init_bkt));
    }

    this->bandwidth = 0;

}

vector<int> DynamicPathDSE::Search(const string& keyword) {
    vector<int> res;
    for (int i = 1; i <= position_map[keyword].first; i++) {
        int leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, i);
        Block target = ReadPath(leaf, hashkey);
        res.push_back(target.document_id);
        int new_leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second + 1);
        AddStash(Block(hashkey, target.document_id, new_leaf));
        Evict(leaf);
    }
    position_map[keyword].second++;
    return res;
}

void DynamicPathDSE::Add(const string& keyword, int document_id) {
    position_map[keyword].first++;
    int leaf = this->rand_gen->GetRandomLeaf(keyword, position_map[keyword].first, position_map[keyword].second);
    array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, position_map[keyword].first);
    AddStash(Block(hashkey, document_id, leaf));
    AddStorage();
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
}

void DynamicPathDSE::Delete(const string& keyword, int document_id) {
    int nc = 0, nt = position_map[keyword].second + 1;
    for (int i = 1; i <= position_map[keyword].first; i++) {
        int leaf = this->rand_gen->GetRandomLeaf(keyword, i, position_map[keyword].second);
        array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, i);
        Block res = ReadPath(leaf, hashkey);
        if (res.document_id == document_id) {
            AddStash(Block());
        }
        else {
            nc++;
            int new_leaf = this->rand_gen->GetRandomLeaf(keyword, nc, nt);
            array<int8_t, Block::KEY_SIZE> new_hashkey = this->rand_gen->GetHashKey(keyword, nc);
            AddStash(Block(new_hashkey, res.document_id, new_leaf));
        }
        Evict(leaf);
    }
    position_map[keyword].first = nc;
    position_map[keyword].second = nt;
    DelStorage();
}

Block DynamicPathDSE::ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey) {
    Block res;

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->keyword == hashKey) {
            res = *it;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathDSE::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize();
        for (Block& b: blocks) {
            if (b.document_id != -1) {
                if (b.keyword == hashKey) {
                    res = b;
                }
                else {
                    stash.emplace_back(Block(b));
                }
            }
        }
    }

    return res;
}

int DynamicPathDSE::ReadTmPath() {
    int pos = max_leaf << (MAX_VIRTUAL_LEVEL - max_level);
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(DynamicPathDSE::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize();
        for (Block& b: blocks) {
            if (b.document_id != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

void DynamicPathDSE::Evict(int pos) {
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
            if (Pxl == P(it->leaf_id, l) && !is_overflow) {
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
        bandwidth += bucket_size * Block::GetBlockSize();
    }
}

int DynamicPathDSE::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (MAX_VIRTUAL_LEVEL - level));
}

void DynamicPathDSE::AddStorage() { 
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

void DynamicPathDSE::DelStorage() {
    vector<Block>& blocks = storage->ReadBucket(max_leaf + full_offset).GetBlocks();
    bandwidth += bucket_size * Block::GetBlockSize();
    int tmp_leaf = (max_leaf + full_offset - 1) >> 1;
    for (int i = num_levels - 2; i >= 0; i--) {
        Bucket& tmp_bucket = storage->ReadBucket(tmp_leaf);
        bandwidth += bucket_size * Block::GetBlockSize();
        for (int j = 0; j < bucket_size; j++) {
            if (tmp_bucket.AddBlock(blocks[j])) {
                blocks[j] = Block();
            }
        }
        storage->WriteBucket(tmp_leaf, tmp_bucket);
        bandwidth += bucket_size * Block::GetBlockSize();
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

void DynamicPathDSE::AddStash(const Block& block) {
    if (block.document_id != -1) {
        this->stash.push_back(block);
    }
}
    
int DynamicPathDSE::GetNumLeaves() {
    return this->num_leaves;
}

int DynamicPathDSE::GetNumLevels() {
    return this->num_levels;
}

int DynamicPathDSE::GetNumBlocksCapacity() {
    return this->num_blocks;
}

int DynamicPathDSE::GetNumBlocks() { 
    return this->num_blocks - DEFAULT_MIN_NUM_BLOCKS;
}

void DynamicPathDSE::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int DynamicPathDSE::GetNumBuckets() {
    return this->num_buckets;

}

RandForOramInterface* DynamicPathDSE::GetRandForOram() {
    return this->rand_gen;
}

int DynamicPathDSE::GetBandwidth() {
    return this->bandwidth;
}

int DynamicPathDSE::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize();
}


void DynamicPathDSE::ClearBandwidth() {
    this->bandwidth = 0;
}

UntrustedStorageInterface* DynamicPathDSE::GetUntrustedStorage() {
    return this->storage;
}
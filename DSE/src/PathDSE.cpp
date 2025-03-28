#include "PathDSE.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <sys/time.h>

PathDSE::PathDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen,
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

vector<int> PathDSE::Search(const string& keyword) {
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

void PathDSE::Add(const string& keyword, int document_id) {
    position_map[keyword].first++;
    int leaf = this->rand_gen->GetRandomLeaf(keyword, position_map[keyword].first, position_map[keyword].second);
    array<int8_t, Block::KEY_SIZE> hashkey = this->rand_gen->GetHashKey(keyword, position_map[keyword].first);
    AddStash(Block(hashkey, document_id, leaf));
    int oldLeaf = ReadTmPath();
    Evict(oldLeaf);
}

void PathDSE::Delete(const string& keyword, int document_id) {
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
}

Block PathDSE::ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey) {
    Block res;

    for (auto it = stash.begin(); it != stash.end(); it++) {
        if (it->keyword == hashKey) {
            res = *it;
            stash.erase(it);
            break;
        }
    }

    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathDSE::P(pos, i)).GetBlocks();
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

int PathDSE::ReadTmPath() {
    int pos = rand_gen->GetRandomLeaf();
    for (int i = 0; i < num_levels; i++) {
        vector<Block>& blocks = storage->ReadBucket(PathDSE::P(pos, i)).GetBlocks();
        bandwidth += bucket_size * Block::GetBlockSize();
        for (Block& b: blocks) {
            if (b.document_id != -1) {
                stash.emplace_back(Block(b));
            }
        }
    }
    return pos;
}

void PathDSE::Evict(int pos) {
    for (int l = num_levels - 1; l >= 0; l--) {
        Bucket bucket = Bucket();
        int Pxl = P(pos, l);

        int counter = 0;
        for (auto it = stash.begin(); it != stash.end(); it++) {
            if (counter >= bucket_size) {
                break;
            }
            if (Pxl == P(it->leaf_id, l)) {
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

int PathDSE::P(int leaf, int level) {
    return (1<<level) - 1 + (leaf >> (this->num_levels - level - 1));
}

void PathDSE::AddStash(const Block& block) {
    if (block.document_id != -1) {
        this->stash.push_back(block);
    }
}
    
int PathDSE::GetNumLeaves() {
    return this->num_leaves;
}

int PathDSE::GetNumLevels() {
    return this->num_levels;
}

int PathDSE::GetNumBlocksCapacity() {
    return this->capacity_blocks;
}

int PathDSE::GetNumBlocks() { 
    return this->num_blocks;
}

void PathDSE::SetNumBlocks(int num_blocks) { 
    this->num_blocks = num_blocks;
}

int PathDSE::GetNumBuckets() {
    return this->num_buckets;
}

RandForOramInterface* PathDSE::GetRandForOram() {
    return this->rand_gen;
}

int PathDSE::GetBandwidth() {
    return this->bandwidth;
}

int PathDSE::GetStashSize() {
    return (this->stash).size() * Block::GetBlockSize();
}

void PathDSE::ClearBandwidth() {
    this->bandwidth = 0;
}
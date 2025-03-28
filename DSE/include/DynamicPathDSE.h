#ifndef DYNAMIC_PATH_DSE_H
#define DYNAMIC_PATH_DSE_H

#include "DSEInterface.h"
#include "UntrustedStorageInterface.h"
#include <set>

class DynamicPathDSE : public DSEInterface {
public:
    const static int DEFAULT_MIN_NUM_BLOCKS = 2;
    const static int MAX_VIRTUAL_LEVEL = 30;

    DynamicPathDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks, set<pair<string, int>>& database);
    vector<int> Search(const string& keyword);
    void Add(const string& keyword, int document_id);
    void Delete(const string& keyword, int document_id);
    
    int GetStashSize();
    int GetNumLeaves();
    int GetNumLevels();
    int GetNumBlocksCapacity();
    int GetNumBlocks();
    void SetNumBlocks(int num_blocks);
    int GetNumBuckets();
    RandForOramInterface* GetRandForOram();
    int GetBandwidth();
    void ClearBandwidth();
    UntrustedStorageInterface* GetUntrustedStorage();

private:
    UntrustedStorageInterface* storage;
    RandForOramInterface* rand_gen; 

    int bucket_size;
    int num_levels;
    int max_level;
    int num_leaves;
    int max_leaf;
    int num_blocks;
    int num_buckets;
    // number of nodes in the full binary tree part
    int full_offset;
    // evaluation
    int bandwidth;

    // keyword -> (count, sdTimes)
    unordered_map<string, pair<int,int>> position_map;
    vector<Block> stash;
    void AddStorage();
    void DelStorage();
    Block ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey);
    int ReadTmPath();
    void Evict(int pos);
    int P(int leaf, int level);
    void AddStash(const Block& block);
};


#endif
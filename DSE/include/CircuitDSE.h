#ifndef CIRCUIT_DSE_H
#define CIRCUIT_DSE_H

#include "DSEInterface.h"
#include "UntrustedStorageInterface.h"
#include <set>

class CircuitDSE : public DSEInterface {
public:
    const static int CONST_COEFFICIENT = 1;

    CircuitDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks, set<pair<string, int>>& database);
    vector<int> Search(const string& keyword);
    void Add(const string& keyword, int document_id);
    void Delete(const string& keyword, int document_id);
    
    int GetStashSize();
    int GetStashMetaSize();
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
    int GetRealStashSize();

private:
    UntrustedStorageInterface* storage;
    RandForOramInterface* rand_gen; 

    int bucket_size;
    int num_levels;
    int num_leaves;
    int num_blocks;
    int num_buckets;
    int capacity_blocks;

    int bandwidth;

    // keyword -> (count, sdTimes)
    unordered_map<string, pair<int,int>> position_map;
    vector<Block> stash;    // in server, can not change the size

    vector<pair<int, array<int8_t, Block::KEY_SIZE>>> PrepareDeepest(int leaf_position);
    vector<pair<int, array<int8_t, Block::KEY_SIZE>>> PrepareTarget(int leaf_position);
    Block ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey);
    void Evict(int pos);
    int P(int leaf, int level);
    void AddStash(const Block& block);
    void DelStash(int targetIndex);
};

#endif
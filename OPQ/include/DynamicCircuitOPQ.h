#ifndef DYNAMIC_CIRCUIT_OPQ_H
#define DYNAMIC_CIRCUIT_OPQ_H

#include "OPQInterface.h"
#include "UntrustedStorageInterface.h"
#include "RandForOramInterface.h"
#include "OperationType.h"

class DynamicCircuitOPQ : public OPQInterface {
public:
    const static int DEFAULT_MIN_NUM_BLOCKS = 2;
    const static int MAX_VIRTUAL_LEVEL = 30;
    const static int CONST_COEFFICIENT = 1;

    DynamicCircuitOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
    void Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    Block ExtractMax();
    
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
    int GetNumLevesInLevel(int level);
    int GetRealStashSize();

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

    int bandwidth;

    unordered_map<int, int> position_map; //array
    vector<Block> stash;

    void AddStorage();
    void DelStorage();
    vector<pair<int, int>> PrepareDeepest(int leaf_position);
    vector<pair<int, int>> PrepareTarget(int leaf_position);
    
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    void Evict(int pos);
    void UpdateMax(int leaf);
    Block FindMax();
    int P(int leaf, int level);
    void AddStash(const Block& block);
    void DelStash();
};


#endif
#ifndef CIRCUIT_OPQ_H
#define CIRCUIT_OPQ_H

#include "OPQInterface.h"
#include "UntrustedStorageInterface.h"
#include "RandForOramInterface.h"
#include "OperationType.h"

class CircuitOPQ : public OPQInterface {
public:
    const static int CONST_COEFFICIENT = 1;

    CircuitOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
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

    unordered_map<int, int> position_map; //array
    vector<Block> stash;    // in server, can not change the size

    vector<pair<int, int>> PrepareDeepest(int leaf_position);
    vector<pair<int, int>> PrepareTarget(int leaf_position);
    void Evict(int pos);
    void UpdateMax(int leaf);
    Block FindMax();
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    int P(int leaf, int level);
    void AddStash(const Block& block);
    void DelStash(int targetIndex);
};

#endif
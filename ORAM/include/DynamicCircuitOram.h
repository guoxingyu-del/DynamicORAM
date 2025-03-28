#ifndef DYNAMIC_CIRCUITORAM_H
#define DYNAMIC_CIRCUITORAM_H

#include "OramInterface.h"
#include "UntrustedStorageInterface.h"

class DynamicCircuitOram : public OramInterface {
public:
    const static int DEFAULT_MIN_NUM_BLOCKS = 2;
    const static int MAX_VIRTUAL_LEVEL = 30;
    const static int CONST_COEFFICIENT = 1;

    DynamicCircuitOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
    array<int8_t, Block::DATA_SIZE> Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    void Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    array<int8_t, Block::DATA_SIZE> Delete(int blockIndex);
    
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
    
    int ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data);
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    void Evict(int pos);
    int P(int leaf, int level);
    void AddStash(const Block& block);
    void DelStash();
};


#endif
#ifndef CIRCUITORAM_H
#define CIRCUITORAM_H

#include "OramInterface.h"
#include "UntrustedStorageInterface.h"

class CircuitOram : public OramInterface {
public:
    const static int CONST_COEFFICIENT = 1;

    CircuitOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
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
    int ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data);
    void Evict(int pos);
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    int P(int leaf, int level);
    void AddStash(const Block& block);
    void DelStash(int targetIndex);
};

#endif
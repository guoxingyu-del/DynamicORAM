#ifndef PATHORAM_H
#define PATHORAM_H
#include "OramInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include <cmath>

class PathOram : public OramInterface {
public:
    PathOram(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
    array<int8_t, Block::DATA_SIZE> Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    void Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    array<int8_t, Block::DATA_SIZE> Delete(int blockIndex);

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

private:
    UntrustedStorageInterface* storage;
    RandForOramInterface* rand_gen;

    int bucket_size;
    int num_levels;
    int num_leaves;
    int num_blocks;
    int capacity_blocks;
    int num_buckets;

    // evaluation
    int bandwidth;

    unordered_map<int, int> position_map; //array
    vector<Block> stash;

    int ReadPath(int blockIndex, Operation op, array<int8_t, Block::DATA_SIZE>& newdata, array<int8_t, Block::DATA_SIZE>& data);
    int ReadTmPath();
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    void Evict(int pos);
    int P(int leaf, int level);
    void AddStash(const Block& block);
};


#endif 

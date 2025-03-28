#ifndef PATH_OPQ_H
#define PATH_OPQ_H
#include "OPQInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include <cmath>

class PathOPQ : public OPQInterface {
public:
    PathOPQ(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks);
    void Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    Block ExtractMax();

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

    int ReadTmPath();
    int ReadDelPath(int blockIndex, array<int8_t, Block::DATA_SIZE>& data);
    void Evict(int pos);
    void UpdateMax(int leaf);
    Block FindMax();
    int P(int leaf, int level);
    void AddStash(const Block& block);
};


#endif 

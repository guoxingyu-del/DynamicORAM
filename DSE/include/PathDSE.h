#ifndef PATH_DSE_H
#define PATH_DSE_H
#include "DSEInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include <cmath>
#include <set>

class PathDSE : public DSEInterface {
public:
    PathDSE(UntrustedStorageInterface* storage, RandForOramInterface* rand_gen, int bucket_size, int capacity_blocks, set<pair<string, int>>& database);
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

    // keyword -> (count, sdTimes)
    unordered_map<string, pair<int,int>> position_map;
    vector<Block> stash;

    int ReadTmPath();
    Block ReadPath(int pos, array<int8_t, Block::KEY_SIZE>& hashKey);
    void Evict(int pos);
    int P(int leaf, int level);
    void AddStash(const Block& block);
};


#endif 

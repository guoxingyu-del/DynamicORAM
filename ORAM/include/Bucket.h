#ifndef PORAM_BUCKET_H
#define PORAM_BUCKET_H


#include "Block.h"
#include <vector>
#include <stdexcept>

class Bucket {

public:
    Bucket();
    Bucket(Bucket *other);
    Bucket(vector<Block>& blocks);
    Block GetBlockByIndex(int index);
    bool AddBlock(const Block& new_blk);
    bool RemoveBlock(const Block& rm_blk);
    vector<Block>& GetBlocks();
    static void SetMaxSize(int maximumSize);
    static void ResetState();
    static int GetMaxSize();
    void PrintBlocks();
    bool HasEmpty();
    
private:
    static bool is_init; //should be initially false
    static int max_size; //should be initially -1
    vector<Block> blocks;
};


#endif //PORAM_BUCKET_H

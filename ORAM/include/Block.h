#ifndef PORAM_BLOCK_H
#define PORAM_BLOCK_H

#include <algorithm>
#include <unordered_map>
using namespace std;

class Block {
public:
    static const int DATA_SIZE = 64;
    int32_t leaf_id;
    int32_t index;
    array<int8_t, DATA_SIZE> data;
    // RankOram needs it
    int8_t is_delete;

    Block();
    Block(int32_t leaf_id, int32_t index, array<int8_t, DATA_SIZE>& data);
    static int GetBlockSize(bool contains_delete);
    void PrintBlock();
    virtual ~Block();
};

#endif //PORAM_BLOCK_H

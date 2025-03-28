#ifndef BLOCK_H
#define BLOCK_H

#include <algorithm>
#include <unordered_map>
using namespace std;

class Block {
public:
    static const int KEY_SIZE = 8;
    // (keyword, document_id, leaf_id)
    array<int8_t, KEY_SIZE> keyword; // keyword = G(w, i)
    int32_t document_id;
    int32_t leaf_id;

    Block();
    Block(array<int8_t, KEY_SIZE> keyword, int32_t document_id, int32_t leaf_id);

    static int GetBlockSize();
    void PrintBlock();
    virtual ~Block();
};

#endif //BLOCK_H

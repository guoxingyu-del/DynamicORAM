#ifndef INSECURE_PQ
#define INSECURE_PQ

#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>
#include "Block.h"

class InsecurePQ {
public:
    InsecurePQ();
    void Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata);
    Block ExtractMax();

    int GetBandwidth();
    void ClearBandwidth();

private:
    std::vector<Block> heap;
    int bandwidth;
    void Swim(int index);
    void Sink(int index);
    bool Empty() const;
};

#endif
#ifndef OPQ_INTERFACE_H
#define OPQ_INTERFACE_H

#include "Block.h"

class OPQInterface {
public:
    virtual void Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) = 0;
    virtual Block ExtractMax() = 0;
};


#endif
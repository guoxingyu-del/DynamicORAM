#ifndef PORAM_ORAMINTERFACE_H
#define PORAM_ORAMINTERFACE_H

#include "Block.h"
#include <vector>
#include "OperationType.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"

class OramInterface {
public:
    virtual array<int8_t, Block::DATA_SIZE> Access(Operation op, int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) = 0;
    virtual void Add(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) = 0;
    virtual array<int8_t, Block::DATA_SIZE> Delete(int blockIndex) = 0;
};


#endif
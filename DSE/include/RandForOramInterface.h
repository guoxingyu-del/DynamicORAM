#ifndef RAND_FOR_ORAM_INTERFACE_H
#define RAND_FOR_ORAM_INTERFACE_H
#include <string>
#include <array>
#include "Block.h"

using namespace std;

class RandForOramInterface {
public:
    virtual int GetRandomLeaf() = 0;

    virtual int GetRandomLeaf(const std::string& str, int seed1, int seed2) = 0;

	virtual array<int8_t, Block::KEY_SIZE> GetHashKey(const std::string& str, int seed) = 0;

    virtual int GetRandomNumber(int N) = 0;

    virtual void SetBound(int num_leaves) = 0;
};


#endif 

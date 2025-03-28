#ifndef RANDOM_FOR_ORAM_H
#define RANDOM_FOR_ORAM_H
#include <cmath>
#include <stdlib.h>
#include <vector>
#include <random>
#include "RandForOramInterface.h"
#include "duthomhas/csprng.hpp"
#include "Block.h"

using namespace std;

class RandomForOram : public RandForOramInterface {
public:
    static bool is_initialized;
    static int bound;

    RandomForOram();

    int GetRandomLeaf();
    int GetRandomLeaf(const std::string& str, int seed1, int seed2);
    array<int8_t, Block::KEY_SIZE> GetHashKey(const std::string& str, int seed);
    int GetRandomNumber(int N);

    void SetBound(int totalNumOfLeaves);
    void ResetState();

private:
    duthomhas::csprng rng;
};

#endif 

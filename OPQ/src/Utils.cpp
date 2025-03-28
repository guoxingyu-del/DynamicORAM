#include "Utils.h"
#include "Block.h"
#include <random>
#include <algorithm>

using namespace std;

void Utils::ShuffleArray(vector<int>& arr) {
    std::random_device rd;
    std::mt19937 rng(rd());

    shuffle(arr.begin(), arr.end(), rng);
}

int* Utils::SampleData(int i) {
    int* newArray = new int[Block::DATA_SIZE];
	for (int j = 0; j < Block::DATA_SIZE; ++j) {
		newArray[j] = i;
	}
	return newArray;
}

bool Utils::IsPowerOfTwoPlusOne(int x) {
    if (x < 1) {
        return false;
    }
    return x == 1 || x == 2 || ((x - 1) & (x - 2)) == 0;
}

int Utils::ReverseBits(int num, int maxBits) {
    unsigned x = num & ((1U << maxBits) - 1);
    
    x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
    x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
    x = ((x >> 4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
    x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
    x = (x >> 16) | (x << 16);
    
    return x >> (32 - maxBits);
}
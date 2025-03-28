#ifndef UTILS_H
#define UTILS_H

#include <vector>

class Utils {
public:
    static void ShuffleArray(std::vector<int>& arr);
    static int* SampleData(int i);
    static bool IsPowerOfTwoPlusOne(int x);
    static int ReverseBits(int num, int maxBits);
};

#endif
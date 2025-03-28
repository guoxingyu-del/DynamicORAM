#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

class Utils {
public:
    static void ShuffleArray(std::vector<int>& arr);
    static int* SampleData(int i);
    static bool IsPowerOfTwoPlusOne(int x);
    static int ReverseBits(int num, int maxBits);
    static std::vector<std::vector<std::string>> ReadPythonStyleArray(const std::string& filepath);
};

#endif
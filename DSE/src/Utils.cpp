#include "Utils.h"
#include "Block.h"
#include <random>
#include <algorithm>
#include <fstream>

using namespace std;

void Utils::ShuffleArray(vector<int>& arr) {
    std::random_device rd;
    std::mt19937 rng(rd());

    shuffle(arr.begin(), arr.end(), rng);
}

int* Utils::SampleData(int i) {
    int* newArray = new int[Block::KEY_SIZE];
	for (int j = 0; j < Block::KEY_SIZE; ++j) {
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

vector<vector<string>> Utils::ReadPythonStyleArray(const string& filepath) {
    vector<vector<string>> result;
    std::ifstream file(filepath);
    string line;

    getline(file, line);

    line.erase(remove(line.begin(), line.end(), ' '), line.end());
    if (line.front() == '[') line = line.substr(1, line.size() - 2);

    vector<string> sublists;
    size_t start = 0;
    while (true) {
        size_t end = line.find("],[", start);
        if (end == string::npos) {
            sublists.push_back(line.substr(start));
            break;
        }
        sublists.push_back(line.substr(start, end - start));
        start = end + 3;
    }

    for (const string& sl : sublists) {
        vector<string> row;
        string elements = sl;
        if (elements.front() == '[') elements = elements.substr(1);
        if (elements.back() == ']') elements = elements.substr(0, elements.size() - 1);

        size_t pos = 0;
        while (true) {
            size_t comma_pos = elements.find(',', pos);
            string elem = elements.substr(pos, comma_pos - pos);
            if (elem.empty()) break;

            if (elem.front() == '\'') elem = elem.substr(1);
            if (elem.back() == '\'') elem = elem.substr(0, elem.size() - 1);
            
            row.push_back(elem);
            if (comma_pos == string::npos) break;
            pos = comma_pos + 1;
        }
        result.push_back(row);
    }

    return result;
}
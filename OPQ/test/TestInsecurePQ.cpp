#include <catch2/catch.hpp>
#include "InsecurePQ.h"
#include "Utils.h"

TEST_CASE("Test Insecure PQ") {
    vector<string> correct;
    for (int i = 0; i < 26; i++) {
        string tmp = "";
        char c = 'a' + i;
        for (int j = 0; j < Block::DATA_SIZE; j++) {
            tmp += c;
        }
        correct.push_back(tmp);
    }

	InsecurePQ* pq = new InsecurePQ();

    // Starting insert into opq
    int numBlocks = pow(2, 20) + 1;
    vector<int> randoms(numBlocks);
    iota(randoms.begin(), randoms.end(), 0);
    Utils::ShuffleArray(randoms);

    array<int8_t, Block::DATA_SIZE> data;
    for (int i = 0; i < randoms.size(); i++) {
        data.fill('a' + randoms[i] % 26);
        pq->Insert(randoms[i], data);
    }

    // Starting extract from opq
    for (int i = 0; i < randoms.size(); i++) {
        Block block = pq->ExtractMax();
        string holder = "";
        for (int j = 0; j < Block::DATA_SIZE; j++) {
            holder += static_cast<char>(block.data[j]);
        }
        REQUIRE(holder == correct[(numBlocks - 1 - i) % 26]);
    }
}
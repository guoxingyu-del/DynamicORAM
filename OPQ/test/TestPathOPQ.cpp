#include <catch2/catch.hpp>
#include "ServerStorage.h"
#include "RandomForOram.h"
#include "PathOPQ.h"
#include "Utils.h"

TEST_CASE("Test Path OPQ") {
    vector<string> correct;
    for (int i = 0; i < 26; i++) {
        string tmp = "";
        char c = 'a' + i;
        for (int j = 0; j < Block::DATA_SIZE; j++) {
            tmp += c;
        }
        correct.push_back(tmp);
    }

    int bucketSize = 4;
	int numBlocks = pow(2, 20) + 1;
	Bucket::SetMaxSize(bucketSize);	

	UntrustedStorageInterface* storage = new ServerStorage();
	RandForOramInterface* random = new RandomForOram();

	PathOPQ* opq = new PathOPQ(storage, random, bucketSize, numBlocks);

    // Starting insert into opq
    vector<int> randoms(numBlocks);
    iota(randoms.begin(), randoms.end(), 0);
    Utils::ShuffleArray(randoms);

    array<int8_t, Block::DATA_SIZE> data;
    for (int i = 0; i < randoms.size(); i++) {
        data.fill('a' + randoms[i] % 26);
        opq->Insert(randoms[i], data);
    }

    // Starting extract from opq
    for (int i = 0; i < randoms.size(); i++) {
        Block block = opq->ExtractMax();
        string holder = "";
        for (int j = 0; j < Block::DATA_SIZE; j++) {
            holder += static_cast<char>(block.data[j]);
        }
        REQUIRE(holder == correct[(numBlocks - 1 - i) % 26]);
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
}
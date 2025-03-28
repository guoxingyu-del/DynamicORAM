#include <catch2/catch.hpp>
#include "ServerStorage.h"
#include "RandomForOram.h"
#include "CircuitOram.h"
#include "Utils.h"

TEST_CASE("Test Circuit Oram") {
    int bucketSize = 4;
	int numBlocks = (1 << 12);
	Bucket::SetMaxSize(bucketSize);

	UntrustedStorageInterface* storage = new ServerStorage();
	
	RandForOramInterface* random = new RandomForOram();
	
	CircuitOram* oram = new CircuitOram(storage, random, bucketSize, numBlocks);

	// Warming up the stash by inserting blocks
	int bound = numBlocks;
	for(int i = 0; i < bound; i++){
        array<int8_t, Block::DATA_SIZE> data;
        data.fill('a' + i % 26);
        oram->Add(i, data);
	}

	// Starting reading
	for(int i = 0; i < bound; i++){
        array<int8_t, Block::DATA_SIZE> data;
		array<int8_t, Block::DATA_SIZE> accessed = oram->Access(Operation::READ, i, data);
		string holder = "";
		for (int j = 0; j < Block::DATA_SIZE; ++j) {
			int8_t temp = accessed[j];
			holder += static_cast<char>(temp);
		}

        string correct = "";
        for (int j = 0; j < Block::DATA_SIZE; ++j) {
			int8_t temp = 'a' + i % 26;
			correct += static_cast<char>(temp);
		}
        REQUIRE(holder == correct);
	}

    // Strating Writing
    for(int i = 0; i < bound; i++){
        array<int8_t, Block::DATA_SIZE> data;
        data.fill('a' + (i + 2) % 26);
        oram->Access(Operation::WRITE, i, data);
	}

    // Starting Reading After Writing
    for(int i = 0; i < bound; i++){
        array<int8_t, Block::DATA_SIZE> data;
		array<int8_t, Block::DATA_SIZE> accessed = oram->Access(Operation::READ, i, data);
		string holder = "";
		for (int j = 0; j < Block::DATA_SIZE; ++j) {
			int8_t temp = accessed[j];
			holder += static_cast<char>(temp);
		}

        string correct = "";
        for (int j = 0; j < Block::DATA_SIZE; ++j) {
			int8_t temp = 'a' + (i + 2) % 26;
			correct += static_cast<char>(temp);
		}
        REQUIRE(holder == correct);
	}

    // Starting Delete
    for(int i = 0; i < bound; i++){
		array<int8_t, Block::DATA_SIZE> accessed = oram->Delete(i);

        string holder = "";
        for (int j = 0; j < Block::DATA_SIZE; ++j) {
            int8_t temp = accessed[j];
            holder += static_cast<char>(temp);
        }
        string correct = "";
        for (int j = 0; j < Block::DATA_SIZE; ++j) {
            int8_t temp = 'a' + (i + 2) % 26;
            correct += static_cast<char>(temp);
        }
        REQUIRE(holder == correct);
	}

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
}
#include <catch2/catch.hpp>
#include "RankOram.h"
#include "Utils.h"

TEST_CASE("Test Rank Oram") {
	int numBlocks = (1 << 12);
	
	RankOram* oram = new RankOram();

	// Warming up the stash by inserting blocks
	int bound = numBlocks;
	for(int i = 0; i < bound; i++){
        array<int8_t, Block::DATA_SIZE> data;
        data.fill('a' + i % 26);
        oram->Access(Operation::INSERT, i, data);
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
    array<int8_t, Block::DATA_SIZE> data;
    for(int i = 0; i < bound; i++){
		array<int8_t, Block::DATA_SIZE> accessed = oram->Access(Operation::DEL, i, data);

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
}
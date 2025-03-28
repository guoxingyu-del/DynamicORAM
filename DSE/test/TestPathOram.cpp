#include <catch2/catch.hpp>
#include "ServerStorage.h"
#include "RandomForOram.h"
#include "PathDSE.h"
#include "Utils.h"
#include <iostream>
#include <sys/time.h>
#include <set>

TEST_CASE("Test Path dse") {
    int bucketSize = 4;
	Bucket::SetMaxSize(bucketSize);

	UntrustedStorageInterface* storage = new ServerStorage();
	
	RandForOramInterface* random = new RandomForOram();

    vector<vector<string>> data = Utils::ReadPythonStyleArray("test/inspec_doc.txt");
    set<pair<string, int>> database;
    unordered_map<string, set<int>> maps;

    for (int i = 0; i < data.size(); i++) {
        for (int j = 0; j < data[i].size(); j++) {
            database.insert(make_pair(data[i][j], i));
            // maps[data[i][j]].insert(i);
        }
    }

    int numBlocks = database.size();
	set<pair<string, int>> empty_database;
	PathDSE* dse = new PathDSE(storage, random, bucketSize, numBlocks, empty_database);

    int i = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        dse->Add(it->first, it->second);
        maps[it->first].insert(it->second);
        if (i % 100000 == 0) {
            for (auto it = maps.begin(); it != maps.end(); it++) {
                string keyword = it->first;
                set<int> correct = it->second;
                vector<int> accessed = dse->Search(keyword);
                REQUIRE(accessed.size() == correct.size());
                for (int j = 0; j < accessed.size(); j++) {
                    REQUIRE(correct.count(accessed[j]) == 1);
                }
            }
        }
        i++;
    }

    for (auto it = maps.begin(); it != maps.end(); it++) {
        string keyword = it->first;
        set<int> correct = it->second;
        vector<int> accessed = dse->Search(keyword);
        REQUIRE(accessed.size() == correct.size());
        for (int j = 0; j < accessed.size(); j++) {
            REQUIRE(correct.count(accessed[j]) == 1);
        }
    }

    i = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        string keyword = it->first;
        int del_target = it->second;
        maps[keyword].erase(del_target);
        dse->Delete(keyword, del_target);

        if (i % 100000 == 0) {
            for (auto it = maps.begin(); it != maps.end(); it++) {
                string keyword = it->first;
                set<int> correct = it->second;
                vector<int> accessed = dse->Search(keyword);
                REQUIRE(accessed.size() == correct.size());
                for (int j = 0; j < accessed.size(); j++) {
                    REQUIRE(correct.count(accessed[j]) == 1);
                }
            }
        }
        i++;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
}
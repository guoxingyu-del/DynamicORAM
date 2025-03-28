#include "PerformanceTester.h"
#include "Block.h"
#include "DSEInterface.h"
#include "PathDSE.h"
#include "ServerStorage.h"
#include "RandomForOram.h"
#include <fstream>
#include <sys/time.h>
#include <iostream>
#include "CircuitDSE.h"
#include "DynamicCircuitDSE.h"
#include "DynamicPathDSE.h"
#include <numeric>
#include "Utils.h"

// change test file name here
const string PerformanceTester::FIELNAME = "lucene";

PerformanceTester::PerformanceTester() {
    string path = "test/" + FIELNAME + "_doc.txt";
    data = Utils::ReadPythonStyleArray(path);
    for (int i = 0; i < data.size(); i++) {
        for (int j = 0; j < data[i].size(); j++) {
            database.insert(make_pair(data[i][j], i));
            // maps[data[i][j]].insert(i);
        }
    }
}

void PerformanceTester::runCorrectnessTest() {
    PathOramTest();
    CircuitOramTest();
    DynamicPathOramTest();
    DynamicCircuitOramTest();
}

// ---------------------------------------------------Path Oram Test-----------------------------------------------------------------------

void PerformanceTester::PathOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
	Bucket::SetMaxSize(bucketSize);

    ofstream add_output("results/PathDSEAdd" + FIELNAME + ".txt");
    ofstream del_output("results/PathDSEDel" + FIELNAME + ".txt");
    ofstream access_output("results/PathDSEAccess"  + FIELNAME + ".txt");

	UntrustedStorageInterface* storage = new ServerStorage();
	RandForOramInterface* random = new RandomForOram();

    vector<string> keys;
    int numBlocks = database.size();
	set<pair<string, int>> two_database;

    int i = 0;
    for (auto it = database.begin(); i < 2; it++) {
        two_database.insert(*it);
        i++;
    }
	PathDSE* dse = new PathDSE(storage, random, bucketSize, numBlocks, two_database);

    int j = 0, index = 4;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }
        keys.push_back(it->first);
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Add(it->first, it->second);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;

        i++;
        if (i % 10000 == 0) cout << "Path DSE Adding..." << i << endl;
        if (i == (1 << index)) {
            cout << "Path DSE Searching... index = " << index << endl;
            for (int k = 0; k < (1 << index); k++) {
                int tmp = dse->GetRandForOram()->GetRandomNumber(keys.size());

                dse->ClearBandwidth();
                gettimeofday(&start, NULL);
                dse->Search(keys[tmp]);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;
            }
            index += 4;
        }
    }

    i = 0; j = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }

        string keyword = it->first;
        int del_target = it->second;
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Delete(keyword, del_target);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;
        i++;
        if (i % 10000 == 0) cout << "Path DSE Deleting..." << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
    delete storage;
    delete random;
    add_output.close();
    del_output.close();
    access_output.close();
}

// ---------------------------------------------------Circuit Oram Test-----------------------------------------------------------------------

void PerformanceTester::CircuitOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
	Bucket::SetMaxSize(bucketSize);

    ofstream add_output("results/CircuitDSEAdd" + FIELNAME + ".txt");
    ofstream del_output("results/CircuitDSEDel" + FIELNAME + ".txt");
    ofstream access_output("results/CircuitDSEAccess"  + FIELNAME + ".txt");

	UntrustedStorageInterface* storage = new ServerStorage();
	RandForOramInterface* random = new RandomForOram();

    vector<string> keys;
    int numBlocks = database.size();
	set<pair<string, int>> two_database;

    int i = 0;
    for (auto it = database.begin(); i < 2; it++) {
        two_database.insert(*it);
        i++;
    }
	CircuitDSE* dse = new CircuitDSE(storage, random, bucketSize, numBlocks, two_database);

    int j = 0, index = 4;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }
        keys.push_back(it->first);
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Add(it->first, it->second);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;

        i++;
        if (i % 10000 == 0) cout << "Circuit DSE Adding..." << i << endl;
        if (i == (1 << index)) {
            cout << "Circuit DSE Searching... index = " << index << endl;
            for (int k = 0; k < (1 << index); k++) {
                int tmp = dse->GetRandForOram()->GetRandomNumber(keys.size());

                dse->ClearBandwidth();
                gettimeofday(&start, NULL);
                dse->Search(keys[tmp]);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;
            }
            index += 4;
        }
    }

    i = 0; j = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }

        string keyword = it->first;
        int del_target = it->second;
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Delete(keyword, del_target);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;
        i++;
        if (i % 10000 == 0) cout << "Circuit DSE Deleting..." << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
    delete storage;
    delete random;
    add_output.close();
    del_output.close();
    access_output.close();
}

// ---------------------------------------------------Dynamic Path Oram Test-----------------------------------------------------------------------

void PerformanceTester::DynamicPathOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
	Bucket::SetMaxSize(bucketSize);

    ofstream add_output("results/DynamicPathDSEAdd" + FIELNAME + ".txt");
    ofstream del_output("results/DynamicPathDSEDel" + FIELNAME + ".txt");
    ofstream access_output("results/DynamicPathDSEAccess"  + FIELNAME + ".txt");

	UntrustedStorageInterface* storage = new ServerStorage();
	RandForOramInterface* random = new RandomForOram();

    vector<string> keys;
    int numBlocks = database.size();
	set<pair<string, int>> two_database;

    int i = 0;
    for (auto it = database.begin(); i < 2; it++) {
        two_database.insert(*it);
        i++;
    }
	DynamicPathDSE* dse = new DynamicPathDSE(storage, random, bucketSize, 2, two_database);

    int j = 0, index = 4;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }
        keys.push_back(it->first);
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Add(it->first, it->second);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;

        i++;
        if (i % 10000 == 0) cout << "Dynamic Path DSE Adding..." << i << endl;
        if (i == (1 << index)) {
            cout << "Dynamic Path DSE Searching... index = " << index << endl;
            for (int k = 0; k < (1 << index); k++) {
                int tmp = dse->GetRandForOram()->GetRandomNumber(keys.size());

                dse->ClearBandwidth();
                gettimeofday(&start, NULL);
                dse->Search(keys[tmp]);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;
            }
            index += 4;
        }
    }

    i = 0; j = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }

        string keyword = it->first;
        int del_target = it->second;
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Delete(keyword, del_target);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetStashSize() << endl;
        i++;
        if (i % 10000 == 0) cout << "Dynamic Path DSE Deleting..." << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
    delete storage;
    delete random;
    add_output.close();
    del_output.close();
    access_output.close();
}

// ---------------------------------------------------Dynamic Circuit Oram Test-----------------------------------------------------------------------

void PerformanceTester::DynamicCircuitOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
	Bucket::SetMaxSize(bucketSize);

    ofstream add_output("results/DynamicCircuitDSEAdd" + FIELNAME + ".txt");
    ofstream del_output("results/DynamicCircuitDSEDel" + FIELNAME + ".txt");
    ofstream access_output("results/DynamicCircuitDSEAccess"  + FIELNAME + ".txt");

	UntrustedStorageInterface* storage = new ServerStorage();
	RandForOramInterface* random = new RandomForOram();

    vector<string> keys;
    int numBlocks = database.size();
	set<pair<string, int>> two_database;

    int i = 0;
    for (auto it = database.begin(); i < 2; it++) {
        two_database.insert(*it);
        i++;
    }
	DynamicCircuitDSE* dse = new DynamicCircuitDSE(storage, random, bucketSize, 2, two_database);

    int j = 0, index = 4;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }
        keys.push_back(it->first);
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Add(it->first, it->second);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;

        i++;
        if (i % 10000 == 0) cout << "Dynamic Circuit DSE Adding..." << i << endl;
        if (i == (1 << index)) {
            cout << "Dynamic Circuit DSE Searching... index = " << index << endl;
            for (int k = 0; k < (1 << index); k++) {
                int tmp = dse->GetRandForOram()->GetRandomNumber(keys.size());

                dse->ClearBandwidth();
                gettimeofday(&start, NULL);
                dse->Search(keys[tmp]);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;
            }
            index += 4;
        }
    }

    i = 0; j = 0;
    for (auto it = database.begin(); it != database.end(); it++) {
        if (j < 2) {
            j++;
            continue;
        }

        string keyword = it->first;
        int del_target = it->second;
        dse->ClearBandwidth();
        gettimeofday(&start, NULL);
        dse->Delete(keyword, del_target);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output << dse->GetBandwidth() << " " << timeuse << " " << dse->GetRealStashSize() << endl;
        i++;
        if (i % 10000 == 0) cout << "Dynamic Circuit DSE Deleting..." << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1;
    delete storage;
    delete random;
    add_output.close();
    del_output.close();
    access_output.close();
}
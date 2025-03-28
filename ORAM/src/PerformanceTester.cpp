#include "PerformanceTester.h"
#include "Block.h"
#include "OramInterface.h"
#include "PathOram.h"
#include "ServerStorage.h"
#include "RandomForOram.h"
#include <fstream>
#include <sys/time.h>
#include <iostream>
#include "CircuitOram.h"
#include "DynamicCircuitOram.h"
#include "DynamicPathOram.h"
#include "RankOram.h"
#include <numeric>

PerformanceTester::PerformanceTester() {
}

void PathOramTest();
void CircuitOramTest();
void DynamicPathOramTest();
void DynamicCircuitOramTest();
void RankOramTest();


void PerformanceTester::runCorrectnessTest() {
    PathOramTest();
    CircuitOramTest();
    DynamicPathOramTest();
    DynamicCircuitOramTest();
    RankOramTest();
}

// ---------------------------------------------------Path Oram Test-----------------------------------------------------------------------

void PathOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage1 = new ServerStorage();
    RandForOramInterface* random1 = new RandomForOram();
    ofstream add_output1("results/PathOramAdd.txt");
    ofstream del_output1("results/PathOramDel.txt");
    ofstream access_output1("results/PathOramAccess.txt");
	
	PathOram* oram = new PathOram(storage1, random1, bucketSize, numBlocks);
    array<int8_t, Block::DATA_SIZE> data;
    data.fill(0); oram->Add(0, data); 
    data.fill(1); oram->Add(1, data); 

    for (int i = 0; i < 100000; i++) oram->Access(Operation::READ, 1, data);

    int index = 4;
    // the unified initial point contains 2 blocks
    for (int i = 2; i < numBlocks; i++) {
        // add
        oram->ClearBandwidth();
        data.fill('a' + i % 26);
        gettimeofday(&start, NULL);
        oram->Add(i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output1 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Path Oram Adding... i = " << i << endl;

        if (i == (1 << index) - 1) {
            cout << "Path Oram Accessing... index = " << index << endl;
            for (int j = 0; j < (1 << index); j++) {
                oram->ClearBandwidth();
                gettimeofday(&start, NULL);
                oram->Access(Operation::READ, j, data);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output1 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;
            }
            index += 4;
        }
    }

    for (int i = 2; i < numBlocks; i++) {
        // del
        oram->ClearBandwidth();
        gettimeofday(&start, NULL);
        oram->Delete(i);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output1 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Path Oram Deleting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage1;
    delete random1;
    add_output1.close();
    del_output1.close();
    access_output1.close();
}

// ---------------------------------------------------Circuit Oram Test-----------------------------------------------------------------------

void CircuitOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage2 = new ServerStorage();
    RandForOramInterface* random2 = new RandomForOram();
    ofstream add_output2("results/CircuitOramAdd.txt");
    ofstream del_output2("results/CircuitOramDel.txt");
    ofstream access_output2("results/CircuitOramAccess.txt");

    CircuitOram* oram = new CircuitOram(storage2, random2, bucketSize, numBlocks);
    array<int8_t, Block::DATA_SIZE> data;
    data.fill(0); oram->Add(0, data); 
    data.fill(1); oram->Add(1, data); 

    for (int i = 0; i < 100000; i++) oram->Access(Operation::READ, 1, data);

    int index = 4;
    for (int i = 2; i < numBlocks; i++) {
        // add
        oram->ClearBandwidth();
        data.fill('a' + i % 26);
        gettimeofday(&start, NULL);
        oram->Add(i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output2 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Circuit Oram Adding... i = " << i << endl;

        if (i == (1 << index) - 1) {
            cout << "Circuit Oram Accessing... index = " << index << endl;
            for (int j = 0; j < (1 << index); j++) {
                oram->ClearBandwidth();
                gettimeofday(&start, NULL);
                oram->Access(Operation::READ, j, data);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output2 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;
            }
            index += 4;
        }
    }

    for (int i = 2; i < numBlocks; i++) {
        // del
        oram->ClearBandwidth();
        gettimeofday(&start, NULL);
        oram->Delete(i);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output2 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Circuit Oram Deleting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage2;
    delete random2;
    add_output2.close();
    del_output2.close();
    access_output2.close();
}

// ---------------------------------------------------Dynamic Path Oram Test-----------------------------------------------------------------------

void DynamicPathOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage3 = new ServerStorage();
    RandForOramInterface* random3 = new RandomForOram();
    ofstream add_output3("results/DynamicPathOramAdd.txt");
    ofstream del_output3("results/DynamicPathOramDel.txt");
    ofstream access_output3("results/DynamicPathOramAccess.txt");

    DynamicPathOram* oram = new DynamicPathOram(storage3, random3, bucketSize, 0);
    array<int8_t, Block::DATA_SIZE> data;

    for (int i = 0; i < 100000; i++) oram->Access(Operation::READ, 1, data);

    int index = 4;

    for (int i = 2; i < numBlocks; i++) {
        // add
        oram->ClearBandwidth();
        data.fill('a' + i % 26);
        gettimeofday(&start, NULL);
        oram->Add(i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;

        add_output3 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Path Oram Adding... i = " << i << endl;

        if (i == (1 << index) - 1) {
            cout << "Dynamic Path Oram Accessing... index = " << index << endl;
            for (int j = 0; j < (1 << index); j++) {
                oram->ClearBandwidth();
                gettimeofday(&start, NULL);
                oram->Access(Operation::READ, j, data);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output3 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;
            }
            index += 4;
        }
    }

    for (int i = 2; i < numBlocks; i++) {
        // del
        oram->ClearBandwidth();
        gettimeofday(&start, NULL);
        oram->Delete(i);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output3 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Path Oram Deleting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage3;
    delete random3;
    add_output3.close();
    del_output3.close();
    access_output3.close();
}

// ---------------------------------------------------Dynamic Circuit Oram Test-----------------------------------------------------------------------

void DynamicCircuitOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage4 = new ServerStorage();
    RandForOramInterface* random4 = new RandomForOram();
    ofstream add_output4("results/DynamicCircuitOramAdd.txt");
    ofstream del_output4("results/DynamicCircuitOramDel.txt");
    ofstream access_output4("results/DynamicCircuitOramAccess.txt");

    DynamicCircuitOram* oram = new DynamicCircuitOram(storage4, random4, bucketSize, 0);
    array<int8_t, Block::DATA_SIZE> data;

    for (int i = 0; i < 100000; i++) oram->Access(Operation::READ, 1, data);

    int index = 4;
    for (int i = 2; i < numBlocks; i++) {
        // add
        oram->ClearBandwidth();
        data.fill('a' + i % 26);
        gettimeofday(&start, NULL);
        oram->Add(i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;

        add_output4 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Circuit Oram Adding... i = " << i << endl;

        // access
        if (i == (1 << index) - 1) {
            cout << "Dynamic Circuit Oram Accessing... index = " << index << endl;
            for (int j = 0; j < (1 << index); j++) {
                oram->ClearBandwidth();
                gettimeofday(&start, NULL);
                oram->Access(Operation::READ, j, data);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output4 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;
            }
            index += 4;
        }
    }

    for (int i = 2; i < numBlocks; i++) {
        // del
        oram->ClearBandwidth();
        gettimeofday(&start, NULL);
        oram->Delete(i);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output4 << oram->GetBandwidth() << " " << timeuse << " " << oram->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Circuit Oram Deleting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage4;
    delete random4;
    add_output4.close();
    del_output4.close();
    access_output4.close();
}

// ---------------------------------------------------Rank Oram Test-----------------------------------------------------------------------

void RankOramTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    ofstream add_output5("results/RankOramAdd.txt");
    ofstream del_output5("results/RankOramDel.txt");
    ofstream access_output5("results/RankOramAccess.txt");

    RankOram* rank_oram = new RankOram();
    array<int8_t, Block::DATA_SIZE> data;
    data.fill(0); rank_oram->Access(Operation::INSERT, 0, data); 
    data.fill(1); rank_oram->Access(Operation::INSERT, 1, data);

    for (int i = 0; i < 100000; i++) rank_oram->Access(Operation::READ, 1, data);

    int index = 4;
    for (int i = 2; i < numBlocks; i++) {
        // add
        rank_oram->ClearBandwidth();
        data.fill('a' + i % 26);
        gettimeofday(&start, NULL);
        rank_oram->Access(Operation::INSERT, i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        add_output5 << rank_oram->GetBandwidth() << " " << timeuse << endl;

        if (i % 10000 == 0) cout << "Rank Oram Adding... i = " << i << endl;

        if (i == (1 << index) - 1) {
            cout << "Rank Oram Accessing... index = " << index << endl;
            for (int j = 0; j < (1 << index); j++) {
                rank_oram->ClearBandwidth();
                gettimeofday(&start, NULL);
                rank_oram->Access(Operation::READ, j, data);
                gettimeofday(&end, NULL);
                timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                access_output5 << rank_oram->GetBandwidth() << " " << timeuse << endl;
            }
            index += 4;
        }
    }

    for (int i = 2; i < numBlocks; i++) {
        // del
        rank_oram->ClearBandwidth();
        gettimeofday(&start, NULL);
        rank_oram->Access(Operation::DEL, i, data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        del_output5 << rank_oram->GetBandwidth() << " " << timeuse << endl;

        if (i % 10000 == 0) cout << "Rank Oram Deleting... i = " << i << endl;
    }

    delete rank_oram;
    add_output5.close();
    del_output5.close();
    access_output5.close();
}
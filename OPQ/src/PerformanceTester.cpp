#include "PerformanceTester.h"
#include "Block.h"
#include "OPQInterface.h"
#include "PathOPQ.h"
#include "ServerStorage.h"
#include "RandomForOram.h"
#include <fstream>
#include <sys/time.h>
#include <iostream>
#include "CircuitOPQ.h"
#include "DynamicCircuitOPQ.h"
#include "DynamicPathOPQ.h"
#include <numeric>
#include "Utils.h"
#include "InsecurePQ.h"

PerformanceTester::PerformanceTester() {
}

void PathOPQTest();
void CircuitOPQTest();
void DynamicPathOPQTest();
void DynamicCircuitOPQTest();
void InsecurePQTest();

void PerformanceTester::runCorrectnessTest() {
    PathOPQTest();
    CircuitOPQTest();
    DynamicPathOPQTest();
    DynamicCircuitOPQTest();
    InsecurePQTest();
}

// ---------------------------------------------------Path Oram Test-----------------------------------------------------------------------

void PathOPQTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage = new ServerStorage();
    RandForOramInterface* random = new RandomForOram();
    ofstream insert_output("results/PathOPQInsert.txt");
    ofstream extract_output("results/PathOPQExtract.txt");
	
	PathOPQ* opq = new PathOPQ(storage, random, bucketSize, numBlocks);

    array<int8_t, Block::DATA_SIZE> data;
    data.fill(0); opq->Insert(0, data);
    data.fill(1); opq->Insert(1, data);

    // warming
    for (int i = 2; i < 10000; i++) opq->Insert(i, data);
    for (int i = 2; i < 10000; i++) opq->ExtractMax();

    vector<int> randoms(numBlocks - 2);
    iota(randoms.begin(), randoms.end(), 2);
    Utils::ShuffleArray(randoms);

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        data.fill('a' + randoms[i] % 26);
        gettimeofday(&start, NULL);
        opq->Insert(randoms[i], data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        insert_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Path OPQ Inserting... i = " << i << endl;
    }

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        gettimeofday(&start, NULL);
        opq->ExtractMax();
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        extract_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Path OPQ Extracting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage;
    delete random;
    insert_output.close();
    extract_output.close();
}

// ---------------------------------------------------Circuit Oram Test-----------------------------------------------------------------------

void CircuitOPQTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage = new ServerStorage();
    RandForOramInterface* random = new RandomForOram();
    ofstream insert_output("results/CircuitOPQInsert.txt");
    ofstream extract_output("results/CircuitOPQExtract.txt");
	
	CircuitOPQ* opq = new CircuitOPQ(storage, random, bucketSize, numBlocks);

    array<int8_t, Block::DATA_SIZE> data;
    data.fill(0); opq->Insert(0, data);
    data.fill(1); opq->Insert(1, data);

    for (int i = 2; i < 10000; i++) opq->Insert(i, data);
    for (int i = 2; i < 10000; i++) opq->ExtractMax();

    vector<int> randoms(numBlocks - 2);
    iota(randoms.begin(), randoms.end(), 2);
    Utils::ShuffleArray(randoms);

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        data.fill('a' + randoms[i] % 26);
        gettimeofday(&start, NULL);
        opq->Insert(randoms[i], data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        insert_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Circuit OPQ Inserting... i = " << i << endl;
    }

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        gettimeofday(&start, NULL);
        opq->ExtractMax();
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        extract_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Circuit OPQ Extracting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage;
    delete random;
    insert_output.close();
    extract_output.close();
}

// ---------------------------------------------------Dynamic Path Oram Test-----------------------------------------------------------------------

void DynamicPathOPQTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage = new ServerStorage();
    RandForOramInterface* random = new RandomForOram();
    ofstream insert_output("results/DynamicPathOPQInsert.txt");
    ofstream extract_output("results/DynamicPathOPQExtract.txt");
	
	DynamicPathOPQ* opq = new DynamicPathOPQ(storage, random, bucketSize, 0);

    array<int8_t, Block::DATA_SIZE> data;
    for (int i = 2; i < 10000; i++) opq->Insert(i, data);
    for (int i = 2; i < 10000; i++) opq->ExtractMax();

    vector<int> randoms(numBlocks - 2);
    iota(randoms.begin(), randoms.end(), 2);
    Utils::ShuffleArray(randoms);

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        data.fill('a' + randoms[i] % 26);
        gettimeofday(&start, NULL);
        opq->Insert(randoms[i], data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        insert_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Path OPQ Inserting... i = " << i << endl;
    }

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        gettimeofday(&start, NULL);
        opq->ExtractMax();
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        extract_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Path OPQ Extracting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage;
    delete random;
    insert_output.close();
    extract_output.close();
}

// ---------------------------------------------------Dynamic Circuit Oram Test-----------------------------------------------------------------------

void DynamicCircuitOPQTest() {
    struct timeval start, end;
    long timeuse = 0;
    int bucketSize = 4;
    int numBlocks = (1 << 20) + 1;

    Bucket::SetMaxSize(bucketSize);	
    UntrustedStorageInterface* storage = new ServerStorage();
    RandForOramInterface* random = new RandomForOram();
    ofstream insert_output("results/DynamicCircuitOPQInsert.txt");
    ofstream extract_output("results/DynamicCircuitOPQExtract.txt");
	
	DynamicCircuitOPQ* opq = new DynamicCircuitOPQ(storage, random, bucketSize, 0);

    array<int8_t, Block::DATA_SIZE> data;
    for (int i = 2; i < 10000; i++) opq->Insert(i, data);
    for (int i = 2; i < 10000; i++) opq->ExtractMax();

    vector<int> randoms(numBlocks - 2);
    iota(randoms.begin(), randoms.end(), 2);
    Utils::ShuffleArray(randoms);

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        data.fill('a' + randoms[i] % 26);
        gettimeofday(&start, NULL);
        opq->Insert(randoms[i], data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        insert_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Circuit OPQ Inserting... i = " << i << endl;
    }

    for (int i = 0; i < randoms.size(); i++) {
        opq->ClearBandwidth();
        gettimeofday(&start, NULL);
        opq->ExtractMax();
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        extract_output << opq->GetBandwidth() << " " << timeuse << " " << opq->GetRealStashSize() << endl;

        if (i % 10000 == 0) cout << "Dynamic Circuit OPQ Extracting... i = " << i << endl;
    }

    Bucket::ResetState();
    ServerStorage::is_initialized = false;
    ServerStorage::is_capacity_set = false;
    RandomForOram::is_initialized = false;
    RandomForOram::bound = -1; 
    delete storage;
    delete random;
    insert_output.close();
    extract_output.close();
}

void InsecurePQTest() {
    struct timeval start, end;
    long timeuse = 0;
    int numBlocks = (1 << 20) + 1;

    ofstream insert_output("results/InsecurePQInsert.txt");
    ofstream extract_output("results/InsecurePQExtract.txt");
	
	InsecurePQ* pq = new InsecurePQ();
    array<int8_t, Block::DATA_SIZE> data;

    vector<int> randoms(numBlocks - 2);
    iota(randoms.begin(), randoms.end(), 2);
    Utils::ShuffleArray(randoms);

    data.fill(0); pq->Insert(0, data);
    data.fill(1); pq->Insert(1, data);
    for (int i = 2; i < 100000; i++) pq->Insert(i, data);
    for (int i = 2; i < 100000; i++) pq->ExtractMax();

    for (int i = 0; i < randoms.size(); i++) {
        pq->ClearBandwidth();
        data.fill('a' + randoms[i] % 26);
        gettimeofday(&start, NULL);
        pq->Insert(randoms[i], data);
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        insert_output << pq->GetBandwidth() << " " << timeuse << endl;

        if (i % 10000 == 0) cout << "Insecure PQ Inserting... i = " << i << endl;
    }

    for (int i = 0; i < randoms.size(); i++) {
        pq->ClearBandwidth();
        gettimeofday(&start, NULL);
        pq->ExtractMax();
        gettimeofday(&end, NULL);
        timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
        extract_output << pq->GetBandwidth() << " " << timeuse << endl;

        if (i % 10000 == 0) cout << "Insecure PQ Extracting... i = " << i << endl;
    }

    insert_output.close();
    extract_output.close();
}
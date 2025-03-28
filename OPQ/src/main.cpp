#include <iostream>
#include <cmath>
#include "Bucket.h"
#include "Block.h"
#include "RandomForOram.h"
#include "PathOPQ.h"
#include "OPQInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include "ServerStorage.h"
#include "PerformanceTester.h"

using namespace std;

int main() {
    // Performance Test
    PerformanceTester* tester = new PerformanceTester();
    tester->runCorrectnessTest();
    delete tester;
}

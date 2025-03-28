#include <iostream>
#include <cmath>
#include "Bucket.h"
#include "Block.h"
#include "RandomForOram.h"
#include "PathDSE.h"
#include "DSEInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include "ServerStorage.h"
#include "PerformanceTester.h"

using namespace std;

int main() {
    cout << "Performance Test" << endl;
    PerformanceTester* tester = new PerformanceTester();
    tester->runCorrectnessTest();

    delete tester;
}

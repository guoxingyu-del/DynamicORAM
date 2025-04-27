## Illustration
Implementation for dynamic Path ORAM and dynamic Circuit ORAM.

## Directories
### ORAM
ORAM implentations.

### OPQ
Oblivious Priority Queue based on ORAM.

### DSE
Dynamic Searchable Encryption based on ORAM.


## Requirements
C++17 is required. Known to compile with gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04).


## Files

All .cpp files are in the `*/src/` and all .h files are in the `*/include/` directory, including any interfaces. `*/test/` contains various tests using the `catch2` C++ testing framework. 

`*/src/Block, */src/Bucket`: contain implementations for the Block and Bucket data structures as described in https://arxiv.org/pdf/1202.5150.

`*/include/UntrustedStorageInterface`: defines the interface that the ORAM implemenation uses to communicate with the untrusted cloud storage provider. A possible implementation is defined in `ServerStorage`.

`*/include/RandForOramInterface`: the ORAM implementations get random leaf ids from here. A possible implementation is defined in `RandomForOram`. Can be overridden e.g. for testing purposes.

`*/include/OperationType`: Enumeration type, representing various operations.

`ORAM/include/OramInterface`: The interface with which to use ORAM implementation. `PathOram`, `DynamicPathOram`, `CircuitOram` and `DynamicCircuitOram` are four implementations.

`ORAM/src/PathOram`: Path ORAM implementation.

`ORAM/src/DynamicPathOram`: Our dynamic version of Path ORAM.

`ORAM/src/CircuitOram`: Circuit ORAM implementation.

`ORAM/src/DynamicCircuitOram`: Our dynamic version of Circuit ORAM.

`ORAM/src/RankOram`: Rank ORAM implementation.

`OPQ/include/OPQInterface`: The interface with which to use OPQ implementation. `PathOPQ`, `DynamicPathOPQ`, `CircuitOPQ` and `DynamicCircuitOPQ` are four implementations.

`OPQ/src/PathOPQ`: Oblivious Priority Queue implementation based on Path ORAM.

`OPQ/src/DynamicPathOPQ`: OPQ implementation based on our dynamic version of Path ORAM.

`OPQ/src/CircuitOPQ`: OPQ implementation based on Circuit ORAM.

`OPQ/src/DynamicCircuitOPQ`: OPQ implementation based on our dynamic version of Circuit ORAM.

`OPQ/src/InsecurePQ`: Non-oblivious priority queue implementation.

`DSE/include/DSEInterface`: The interface with which to use DSE implementation. `PathDSE`, `DynamicPathDSE`, `CircuitDSE` and `DynamicCircuitDSE` are four implementations.

`DSE/src/PathDSE`: Dynamic Searchable Encryption implementation based on Path ORAM.

`DSE/src/DynamicPathDSE`: DSE implementation based on our dynamic version of Path ORAM.

`DSE/src/CircuitDSE`: DSE implementation based on Circuit ORAM.

`DSE/src/DynamicCircuitDSE`: DSE implementation based on our dynamic version of Circuit ORAM.

`*/src/PerformanceTester`: Performance test for five ORAM shemes in Add, Delete and Access operations.

`*/src/Utils`: Some general functions.

`*/src/main`: Can be used to run performance test.

`*/src/csprng.cpp` and `include/duthomhas/`: This file and directory contain code from the [CSPRNG](https://github.com/Duthomhas/CSPRNG) library as per the suggestion in the library's github.

`*/pictures/draw.py`: Drawing with matplotlib.

`*/results/`: Results of performance testing, maybe you need to create it before testing.

## Compiling and Running 
To compile the implementation, enter the corresponding folder, run `make all` in the ORAM directory. To run the code in `main.cpp`, you may now call `./PORAMRun`.

To compile and run tests, enter the corresponding folder, call `make test` to make the tests and `./PORAMTest` to run all the tests.


## Adding Tests
Tests running using catch2 are located in the `test/` respository. To add new tests, add a new `.cpp` file to the `test/` directory. You must include `catch.h` in any new test files. The `include/` directory is already linked by the make file, to include any files from the implementation in your tests, include their headers. See any of the current test files or the catch2 [documentation](https://github.com/catchorg/Catch2) to get a template for test cases.

Running `make test` as before will also compile any new test cases.

## Randomness
Note that the instantiation of RandForOramInterface in RandomForOram uses the [CSPRNG](https://github.com/Duthomhas/CSPRNG) code by Michael Thomas Greer which provides cryptographically secure PRNGs for both Linux and Windows. The file also includes an implementation using the standard C++ implementation of the Mersenne Twister Engine (MT) to instantiate the random number generator. Finally, it also contains an implementation of the LCG random number generator, instantiated using the same parameters as used in the standard Java implementation. The MT and LCG implementations could be useful when comparing outputs of this implementation (or code that uses it) with the outputs of another implementation using the same PRNGs with the same seeds. 

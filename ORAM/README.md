## Requirements
C++17 is required. Known to compile with gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04).


## Files

All .cpp files are in the `src/` and all .h files are in the `include/` directory, including any interfaces. `test/` contains various tests using the `catch2` C++ testing framework. 

`Block, Bucket`: contain implementations for the Block and Bucket data structures as described in https://arxiv.org/pdf/1202.5150

`include/UntrustedStorageInterface`: defines the interface that the ORAM implemenation uses to communicate with the untrusted cloud storage provider. A possible implementation is defined in `ServerStorage`.

`include/RandForOramInterface`: the ORAM implementations get random leaf ids from here. A possible implementation is defined in `RandomForOram`. Can be overridden e.g. for testing purposes.

`include/OramInterface`: The interface with which to use ORAM implementation. `PathOram`, `DynamicPathOram`, `CircuitOram` and `DynamicCircuitOram` are four implementations.

`include/OperationType`: Enumeration type, representing various operations.

`PathOram`: Path ORAM implementation.

`DynamicPathOram`: Our dynamic version of Path ORAM.

`CircuitOram`: Circuit ORAM implementation.

`DynamicCircuitOram`: Our dynamic version of Circuit ORAM.

`RankOram`: Rank ORAM implementation.

`PerformanceTester`: Performance test for five ORAM shemes in Add, Delete and Access operations.

`Utils`: Some general functions.

`main`: Can be used to run performance test.

`csprng.cpp` and `include/duthomhas/`: This file and directory contain code from the [CSPRNG](https://github.com/Duthomhas/CSPRNG) library as per the suggestion in the library's github.

`pictures/draw.py`: Drawing with matplotlib.

`results/`: Results of performance testing.

## Compiling and Running 
To compile the implementation, run `make all` in the ORAM directory. To run the code in `main.cpp`, you may now call `./PORAMRun`.

To compile and run tests, call `make test` to make the tests and `./PORAMTest` to run all the tests.


## Adding Tests
Tests running using catch2 are located in the `test/` respository. To add new tests, add a new `.cpp` file to the `test/` directory. You must include `catch.h` in any new test files. The `include/` directory is already linked by the make file, to include any files from the implementation in your tests, include their headers. See any of the current test files or the catch2 [documentation](https://github.com/catchorg/Catch2) to get a template for test cases.

Running `make test` as before will also compile any new test cases.

## Randomness
Note that the instantiation of RandForOramInterface in RandomForOram uses the [CSPRNG](https://github.com/Duthomhas/CSPRNG) code by Michael Thomas Greer which provides cryptographically secure PRNGs for both Linux and Windows. The file also includes an implementation using the standard C++ implementation of the Mersenne Twister Engine (MT) to instantiate the random number generator. Finally, it also contains an implementation of the LCG random number generator, instantiated using the same parameters as used in the standard Java implementation. The MT and LCG implementations could be useful when comparing outputs of this implementation (or code that uses it) with the outputs of another implementation using the same PRNGs with the same seeds. 

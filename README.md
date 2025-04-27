## Introduction
This is a project for the paper "Dynamic Oblivious RAM for Oblivious Data Structure and Dynamic Searchable Encryption". Note that we also compare the results with RankORAM(https://dl.acm.org/doi/pdf/10.1145/3634737.3656290).


## Requirements
C++17 is required. Known to compile with gcc version 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04).


## Compiling and Running 
```bash
cd DIR  # replace DIR with `ORAM`, `OPQ` or `DSE`, the folder name you want to enter in.
make clean
mkdir -p results
```
- If you want to run correctness test, input `make test`, and run `./PORAMTest`.
- If you want to run performance test, input `make all`, and run `./PORAMRun`.


## Exception
If the program terminates abnormally, it is very likely that the stash of the dynamic circuit-related solution has overflowed, and you need to manually adjust the stash size. In the `*/include/DynamicCirucit*.h` file, find the `CONST_COEFFICIENT` variable and increase it appropriately.


## Adding Tests
Tests running using catch2 are located in the `*/test/` respository. To add new tests, add a new `.cpp` file to the `*/test/` directory. You must include `catch.h` in any new test files. The `*/include/` directory is already linked by the make file, to include any files from the implementation in your tests, include their headers. See any of the current test files or the catch2 [documentation](https://github.com/catchorg/Catch2) to get a template for test cases.

Running `make test` as before will also compile any new test cases.

## Randomness
Note that the instantiation of RandForOramInterface in RandomForOram uses the [CSPRNG](https://github.com/Duthomhas/CSPRNG) code by Michael Thomas Greer which provides cryptographically secure PRNGs for both Linux and Windows. The file also includes an implementation using the standard C++ implementation of the Mersenne Twister Engine (MT) to instantiate the random number generator. Finally, it also contains an implementation of the LCG random number generator, instantiated using the same parameters as used in the standard Java implementation. The MT and LCG implementations could be useful when comparing outputs of this implementation (or code that uses it) with the outputs of another implementation using the same PRNGs with the same seeds. 

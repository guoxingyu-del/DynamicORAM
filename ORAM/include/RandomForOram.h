#ifndef RANDOM_FOR_ORAM_H
#define RANDOM_FOR_ORAM_H
#include <cmath>
#include <stdlib.h>
#include <vector>
#include <random>
#include "RandForOramInterface.h"
#include "duthomhas/csprng.hpp"

using namespace std;

class RandomForOram : public RandForOramInterface {
	public:
		static bool is_initialized;
		static int bound;
		vector<int> rand_history;
		RandomForOram();
		void RandomForOramMT();
		void RandomForOramLCG();
		int GetRandomLeaf();
		int GetRandomLeafMT();
		int GetRandomLeafLCG();
		void SetBound(int totalNumOfLeaves);
		void ResetState();
		void ClearHistory();
		vector<int> GetHistory();
		linear_congruential_engine<unsigned long, 25214903917, 11, 281474976710656> rnd_generator;
		std::mt19937 mt_generator; 
		long seed = 0L;
		duthomhas::csprng rng;
};

#endif 

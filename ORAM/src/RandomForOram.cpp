#include "RandomForOram.h"
#include <iostream>

bool RandomForOram::is_initialized = false;
int RandomForOram::bound = -1; 

RandomForOram::RandomForOram() {

	if (this->is_initialized) {
		throw new runtime_error("ONLY ONE RANDOM INSTANCE CAN BE USED AT A TIME");
	}
	this->rng(long());
	this->rand_history = vector<int>();
	is_initialized  = true;

}


int RandomForOram::GetRandomLeaf() {
	
	long next = this->rng();
	int randVal = next % this->bound;
	if (randVal < 0) {
		randVal += this->bound;
	}

	return randVal;
}

void RandomForOram::RandomForOramMT() {

	if (this->is_initialized) {
		throw new runtime_error("ONLY ONE RANDOM INSTANCE CAN BE USED AT A TIME");
	}

	std::random_device seed1;
	std::mt19937 mt_generator(seed1()); 

	this->rand_history = vector<int>();
	is_initialized  = true;

}


int RandomForOram::GetRandomLeafMT() {
	long next = mt_generator();
	int randVal = next % this->bound;
	this->rand_history.push_back(randVal);
	return randVal;
}


void RandomForOram::RandomForOramLCG() {

	if (this->is_initialized) {
		throw new runtime_error("ONLY ONE RANDOM INSTANCE CAN BE USED AT A TIME");
	}
	linear_congruential_engine<unsigned long, 0x5DEECE66DL, 11, 281474976710656> rnd_generator;
	rnd_generator.seed(0L);
	this->seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
	this->rand_history = vector<int>();
	is_initialized  = true;

}


int RandomForOram::GetRandomLeafLCG() {
	long next = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);//rnd_generator();
	this->seed = next;
	int bits = next >> (48 - 31);
	int randVal = 0;
	if ((this->bound & -this->bound) == this->bound) {  // i.e., bound is a power of 2
    	randVal = (int)((this->bound * (long)bits) >> 31);
    	return randVal;
    }
	int val = bits % this->bound;
	while (bits - val + (bound-1) < 0) {
		next = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);//rnd_generator();
		this->seed = next;
		bits = next >> (48 - 31);
		val = bits % this->bound;
	}
	randVal = val;
	this->rand_history.push_back(randVal);
	return randVal;
}
void RandomForOram::SetBound(int totalNumOfLeaves) {
	this->bound = totalNumOfLeaves;
}

void RandomForOram::ResetState() {
	this->is_initialized = false;
}
void RandomForOram::ClearHistory() {
	this->rand_history = vector<int>();
}
vector<int> RandomForOram::GetHistory() {
	return this->rand_history;
}


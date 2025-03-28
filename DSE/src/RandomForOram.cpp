#include "RandomForOram.h"
#include <iostream>
#include <climits>
#include <openssl/hmac.h>

bool RandomForOram::is_initialized = false;
int RandomForOram::bound = -1; 

RandomForOram::RandomForOram() {

	if (this->is_initialized) {
		throw new runtime_error("ONLY ONE RANDOM INSTANCE CAN BE USED AT A TIME");
	}
    this->rng(long());
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

int RandomForOram::GetRandomLeaf(const std::string& str, int a, int b) {
    // Step 1: Fixed secret key for HMAC (can be random or constant)
    const std::string key = "my_secret_key";  // Change this key as needed

    // Step 2: Combine inputs into a message buffer
    std::string message = str;
    message.append(reinterpret_cast<const char*>(&a), sizeof(a));
    message.append(reinterpret_cast<const char*>(&b), sizeof(b));

    // Step 3: HMAC-SHA256
    unsigned char hmac_result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(),
        key.data(), key.size(),
        reinterpret_cast<const unsigned char*>(message.data()), message.size(),
        hmac_result, &len);

    // Step 4: Take first 8 bytes -> uint64_t
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | hmac_result[i];
    }

    // Step 5: Modulo N
    return static_cast<int>(value % bound);
}

inline void bit_mix(uint32_t& a, uint32_t& b, uint32_t salt) {
    a = (a ^ b) * 0xcc9e2d51 + salt;
    b = (b ^ a) * 0x1b873593;
    a = (a ^ (a >> 16)) * 0x85ebca6b;
    b = (b ^ (b >> 13)) * 0xc2b2ae35;
}

array<int8_t, Block::KEY_SIZE> RandomForOram::GetHashKey(const std::string& str, int seed) {
    uint32_t state[4] = {
        0x6c078965 ^ static_cast<uint32_t>(seed),
        0x9908b0df ^ static_cast<uint32_t>(str.length()),
        0x9d2c5680,
        0xefc60000
    };

    size_t i = 0;
    for (char c : str) {
        state[i % 4] = (state[i % 4] << 7) | (state[i % 4] >> 25);
        state[i % 4] ^= static_cast<uint8_t>(c) * 0x85ebca77;
        bit_mix(state[(i+1)%4], state[i%4], static_cast<uint32_t>(i));
        ++i;
    }

    for (int j = 0; j < 8; ++j) {
        uint32_t temp = state[j % 4] ^ (state[(j+1)%4] << 11);
        state[(j+2)%4] ^= temp >> 8;
        state[j%4] = temp * 0x5bd1e995 + 0xffff0000;
    }

    std::array<int8_t, 8> result;
    for (int k = 0; k < 8; ++k) {
        uint32_t val = state[k % 4] >> ((k/2)*8);
        result[k] = static_cast<int8_t>(val & 0xFF);
    }

    return result;
}

int RandomForOram::GetRandomNumber(int N) {
    long next = this->rng();
	int randVal = next % N;
	if (randVal < 0) {
		randVal += N;
	}
	return randVal;
}

void RandomForOram::SetBound(int totalNumOfLeaves) {
	this->bound = totalNumOfLeaves;
}

void RandomForOram::ResetState() {
	this->is_initialized = false;
}


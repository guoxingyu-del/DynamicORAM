#include "Block.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

Block::Block() { //dummy block
    this->data.fill(-1);
    this->leaf_id = -1;
    this->index = -1;
}

Block::Block(int32_t leaf_id, int32_t index, array<int8_t, DATA_SIZE>& data) : leaf_id(leaf_id), index(index) {
    for (int i = 0; i < DATA_SIZE; i++){
        this->data[i] = data[i];
    }
}

int Block::GetBlockSize(bool contains_delete) {
    int sum = DATA_SIZE + sizeof(int32_t);
    return contains_delete ? sum : sum + sizeof(int32_t);
}

Block::~Block()
{
    //dtor
}

void Block::PrintBlock() {
	string data_holder = "";
	for (int i = 0; i < DATA_SIZE; i++) {
		data_holder += to_string(this->data[i]);
		data_holder += " ";
	}
	cout << "index: " << to_string(this->index) << " leaf id: " << to_string(this->leaf_id) << " data: " << data_holder << endl;
}

bool operator<(const Block& b1, const Block& b2) {
    return b1.index < b2.index;
}

bool operator==(const Block& b1, const Block& b2) {
    return b1.index < b2.index;
}

bool operator!=(const Block& b1, const Block& b2) {
    return !(b1 == b2);
}
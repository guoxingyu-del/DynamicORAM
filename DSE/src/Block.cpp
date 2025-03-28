#include "Block.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

Block::Block() {
    this->keyword.fill(-1);
    this->document_id = -1;
    this->leaf_id = -1;
}

Block::Block(array<int8_t, KEY_SIZE> keyword, int32_t document_id, int32_t leaf_id) : keyword(keyword), document_id(document_id), leaf_id(leaf_id) {
}

int Block::GetBlockSize() {
    return KEY_SIZE + 2 * sizeof(int32_t);
}

Block::~Block()
{
    //dtor
}

void Block::PrintBlock() {
	string key_holder = "";
	for (int i = 0; i < KEY_SIZE; i++) {
		key_holder += to_string(this->keyword[i]);
		key_holder += " ";
	}
	cout << "keyword: "<< key_holder << ", document id: " << to_string(this->document_id) << ", leaf id: " << to_string(this->leaf_id) << endl;
}
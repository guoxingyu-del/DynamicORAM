#include "Bucket.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

bool Bucket::is_init = false;
int Bucket::max_size = -1;

Bucket::Bucket(){
    if(!is_init){
        throw new runtime_error("Please set bucket size before creating a bucket");
    }
    blocks = vector<Block>();
}

Bucket::Bucket(Bucket *other){
    if(other == NULL){
        throw new runtime_error("the other bucket is not malloced.");
    }
    blocks = vector<Block>(max_size);
    for(int i = 0; i < max_size; i++){
        blocks[i] = Block(other->blocks[i]);
    }
}

Bucket::Bucket(vector<Block>& blocks) {
    this->blocks = vector<Block>(max_size);
    for(int i = 0; i < blocks.size(); i++){
        this->blocks[i] = Block(blocks[i]);
    }
    for (int i = blocks.size(); i < max_size; i++) {
        this->blocks[i] = Block();
    }
}

//Get block object with matching index
Block Bucket::GetBlockByIndex(int index) {
    Block *copy_block = NULL;
    for(Block b: blocks){
        if(b.index == index){
            copy_block = new Block(b);
            break;
        }
    }
    return *copy_block;
}

bool Bucket::AddBlock(const Block& new_blk){
    if(blocks.size() < max_size){
        blocks.emplace_back(Block(new_blk));
        return true;
    }
    else {
        for (int i = 0; i < max_size; i++) {
            if (blocks[i].index == -1) {
                blocks[i] = Block(new_blk);
                return true;
            }
        }
    }
    return false;
}

bool Bucket::RemoveBlock(const Block& rm_blk){
    bool removed = false;
    for(int i = 0; i < blocks.size(); i++){
        if(blocks[i].index == rm_blk.index){
            blocks[i] = Block();
            removed = true;
            break;
        }
    }
    return removed;
}

// Return a shallow copy.
vector<Block>& Bucket::GetBlocks(){
    return this->blocks;
}

void Bucket::SetMaxSize(int maximumSize){
    if(is_init == true){
        throw new runtime_error("Max Bucket Size was already set");
    }
    max_size = maximumSize;
    is_init = true;
}

int Bucket::GetMaxSize() {
    return max_size;
}

void Bucket::ResetState(){
    is_init = false;
}

void Bucket::PrintBlocks() {
    for (Block b: blocks) {
        b.PrintBlock();
    }
}

bool Bucket::HasEmpty() {
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].index == -1) return true;
    }
    return blocks.size() < max_size;
}
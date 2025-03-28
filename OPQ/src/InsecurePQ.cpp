#include "InsecurePQ.h"

InsecurePQ::InsecurePQ() : bandwidth(0) {}

void InsecurePQ::Insert(int blockIndex, array<int8_t, Block::DATA_SIZE>& newdata) {
    heap.push_back(Block(-1, blockIndex, newdata));
    bandwidth += Block::GetBlockSize(true);
    Swim(heap.size() - 1);
}

Block InsecurePQ::ExtractMax() {
    assert(!Empty());
    // server
    Block target = heap[0];
    bandwidth += Block::GetBlockSize(true);
    heap[0] = heap.back();
    heap.pop_back();
    if (!Empty()) Sink(0);
    return target;
}

void InsecurePQ::Swim(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        bandwidth += Block::GetBlockSize(true);
        if (heap[index] < heap[parent]) {           
            bandwidth += 2 * Block::GetBlockSize(true);
            break;
        }
        std::swap(heap[index], heap[parent]);
        index = parent;
        bandwidth += Block::GetBlockSize(true);
    }
}

void InsecurePQ::Sink(int index) {
    int size = heap.size();
    // read heap[0]
    bandwidth += Block::GetBlockSize(true);
    while (2 * index + 1 < size) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = left;
        // read 2 child
        bandwidth += 2 * Block::GetBlockSize(true);

        if (right < size && heap[left] < heap[right]) {
            largest = right;
        }
        // write small one
        bandwidth += Block::GetBlockSize(true);
        
        if (heap[largest] < heap[index]) {
            bandwidth += 2 * Block::GetBlockSize(true);
            break;
        }
        std::swap(heap[index], heap[largest]);
        index = largest;
        // write small one
        bandwidth += Block::GetBlockSize(true);
    }
}

bool InsecurePQ::Empty() const {
    return heap.empty();
}

int InsecurePQ::GetBandwidth() {
    return this->bandwidth;
}

void InsecurePQ::ClearBandwidth() {
    this->bandwidth = 0;
}
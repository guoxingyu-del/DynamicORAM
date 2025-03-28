#ifndef OPQ_RAND_FOR_ORAM_INTERFACE_H
#define OPQ_RAND_FOR_ORAM_INTERFACE_H

using namespace std;

class RandForOramInterface {
public:
    virtual int GetRandomLeaf() = 0;

    virtual void SetBound(int num_leaves) = 0;
};


#endif 

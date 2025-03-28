#ifndef DSE_INTERFACE_H
#define DSE_INTERFACE_H

#include "Block.h"
#include <vector>
#include "OperationType.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"

class DSEInterface {
public:
    virtual vector<int> Search(const string& keyword) = 0;
    virtual void Add(const string& keyword, int document_id) = 0;
    virtual void Delete(const string& keyword, int document_id) = 0;
};


#endif
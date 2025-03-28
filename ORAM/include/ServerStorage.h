#ifndef SERVER_STORAGE_H
#define SERVER_STORAGE_H
#include "OramInterface.h"
#include "RandForOramInterface.h"
#include "UntrustedStorageInterface.h"
#include <cmath>

class ServerStorage : public UntrustedStorageInterface {
public:
    static bool is_initialized;
    static bool is_capacity_set;
    
    ServerStorage();
    void SetCapacity(int totalNumOfBuckets);
    void ChangeCapacity(int totalNumOfBuckets);
    Bucket& ReadBucket(int position);
    vector<int32_t> ReadBucketMetaData(int position);
    void WriteBucket(int position, const Bucket& bucket_to_write);
    
private:
    int capacity;
    std::vector<Bucket> buckets;
};


#endif

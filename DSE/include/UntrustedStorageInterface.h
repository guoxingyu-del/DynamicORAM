#ifndef PORAM_UNTRUSTEDSTORAGEINTERFACE_H
#define PORAM_UNTRUSTEDSTORAGEINTERFACE_H
#include "Bucket.h"

class UntrustedStorageInterface {
public:
    virtual void SetCapacity(int totalNumOfBuckets) = 0;

    virtual void ChangeCapacity(int totalNumOfBuckets) = 0;

    virtual Bucket& ReadBucket(int position) = 0;

    virtual void WriteBucket(int position, const Bucket& bucket_to_write) = 0;

    virtual vector<int32_t> ReadBucketMetaData(int position) = 0;
};


#endif

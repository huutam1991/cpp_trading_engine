#ifndef STORAGE_SOURCE_COINBASE_REALNET_H
#define STORAGE_SOURCE_COINBASE_REALNET_H

#include <storage_source/coinbase/storage_source_coinbase.h>

class StorageSourceCoinbaseRealnet : public StorageSourceCoinbase
{
public:
    StorageSourceCoinbaseRealnet(){}
    StorageSourceCoinbaseRealnet(const std::string& user_id) : StorageSourceCoinbase(user_id) {}

protected:
    virtual void init_endpoint();
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const { return SourceType::COINBASE_REALNET; };
};

#endif //STORAGE_SOURCE_COINBASE_REALNET_H
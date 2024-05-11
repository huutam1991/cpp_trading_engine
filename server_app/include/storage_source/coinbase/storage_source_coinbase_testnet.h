#ifndef STORAGE_SOURCE_COINBASE_TESTNET_H
#define STORAGE_SOURCE_COINBASE_TESTNET_H

#include <storage_source/coinbase/storage_source_coinbase.h>

class StorageSourceCoinbaseTestnet : public StorageSourceCoinbase
{
public:
    StorageSourceCoinbaseTestnet(){}
    StorageSourceCoinbaseTestnet(const std::string& user_id) : StorageSourceCoinbase(user_id) {}

protected:
    virtual void init_endpoint();
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const { return SourceType::COINBASE_TESTNET; };
};

#endif //STORAGE_SOURCE_COINBASE_TESTNET_H
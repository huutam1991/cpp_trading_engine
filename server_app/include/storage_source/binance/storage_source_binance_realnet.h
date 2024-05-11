#ifndef STORAGE_SOURCE_BINANCE_REALNET_H
#define STORAGE_SOURCE_BINANCE_REALNET_H

#include <storage_source/binance/storage_source_binance.h>

class StorageSourceBinanceRealnet : public StorageSourceBinance
{
public:
    StorageSourceBinanceRealnet(){}
    StorageSourceBinanceRealnet(const std::string& user_id) : StorageSourceBinance(user_id) {}

protected:
    virtual void init_endpoint();
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const { return SourceType::BINANCE_SPOT; };

};

#endif //STORAGE_SOURCE_BINANCE_REALNET_H
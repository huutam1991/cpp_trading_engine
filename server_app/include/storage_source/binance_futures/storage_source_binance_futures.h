#ifndef STORAGE_SOURCE_BINANCE_FUTURES_H
#define STORAGE_SOURCE_BINANCE_FUTURES_H

#include <storage_source/storage_source.h>

class StorageSourceBinanceFutures : public StorageSource
{
public:
    StorageSourceBinanceFutures(){}
    StorageSourceBinanceFutures(const std::string& user_id) : StorageSource(user_id){}

    virtual std::string init_info();
    virtual std::string verify_valid_source();

    //virtual void init_endpoint() = 0;
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const;
    
    const std::string& get_api_key() const;
    const std::string& get_api_secret() const;

protected:
    std::string m_api_key;
    std::string m_api_secret;

private:
    std::string init_api_key();
    std::string verify_api_key();
    //void get_user_data_stream();
};

#endif //STORAGE_SOURCE_BINANCE_FUTURES_H
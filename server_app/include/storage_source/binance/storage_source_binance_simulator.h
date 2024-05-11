#ifndef STORAGE_SOURCE_BINANCE_SIMULATOR_H
#define STORAGE_SOURCE_BINANCE_SIMULATOR_H

#include <storage_source/binance/storage_source_binance.h>

class StorageSourceBinanceSimulator : public StorageSourceBinance
{
public:
    StorageSourceBinanceSimulator(){}
    StorageSourceBinanceSimulator(const std::string& user_id) : StorageSourceBinance(user_id) {}

protected:
    virtual void init_endpoint();
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const { return SourceType::BINANCE_SIMULATOR; };

private:
    void init_simulate_api_key_and_api_secret();

};

#endif //STORAGE_SOURCE_BINANCE_SIMULATOR_H
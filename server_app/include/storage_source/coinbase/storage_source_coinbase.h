#ifndef STORAGE_SOURCE_COINBASE_H
#define STORAGE_SOURCE_COINBASE_H

#include <storage_source/storage_source.h>

class StorageSourceCoinbase : public StorageSource
{
public:
    StorageSourceCoinbase(){}
    StorageSourceCoinbase(const std::string& user_id) : StorageSource(user_id){}

protected:
    std::string m_api_key;
    std::string m_api_secret;
    std::string m_passphrase;

public:
    virtual std::string init_info();
    virtual std::string verify_valid_source();
    virtual void init_endpoint() = 0;
    virtual const char* get_db_name() const = 0;

    const std::string& get_api_key() const;
    const std::string& get_api_secret() const;
    const std::string& get_passphrase() const;

private:
    std::string init_api_key();
    std::string verify_api_key();
    void get_user_data_stream();

};

#endif //STORAGE_SOURCE_COINBASE_H
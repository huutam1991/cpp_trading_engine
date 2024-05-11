#ifndef STORAGE_SOURCE_H
#define STORAGE_SOURCE_H

#include <string>
#include <memory>
#include <unordered_map>

#include <mongo_db/mongo_db.h>
#include <app_constants.h>
#include <json/json.h>

class StorageSource
{
public:
    StorageSource();
    StorageSource(const std::string& user_id);

    static std::shared_ptr<StorageSource> generate_storage_souce_by_type(SourceType type);
    static SourceType get_source_type_by_name(const std::string& source_name);
    static bool check_valid_source_name(const std::string& source_name);
    static Json get_available_source();
    
    virtual std::string init_info();
    virtual std::string verify_valid_source();
    
    virtual const char* get_db_name() const;
    virtual SourceType get_source_type() const;

    const Json get_source_info() const;
    const std::string get_id() const;
    const std::string& get_user_id() const;
    void set_user_id(const std::string& user_id);

    static std::map<std::string, SourceType>    m_source_map;

protected:
    std::string m_user_id;
};

#endif //STORAGE_SOURCE_H

#define INIT_STORAGE_SOURCE_SUCCESS "init_storage_source_success"
#define CANNOT_FIND_API_KEY "Cannot find API-key"
#define API_KEY__USED_BY_ANOTHER_USER "API-key used by another user"
#define VERIFY_API_KEY_AND_API_SECRET_SUCCESS "verify_api_key_success"
#ifndef USER_H
#define USER_H

#include <memory>
#include <string>

#include <storage_source/storage_source.h>

class User
{
public:
    User(const std::string& user_id);

    const std::string& get_user_id();
    const std::string& get_init_storage_source_result();
    const std::string get_storage_source_db_name();

    SourceType get_last_source_type();
    std::string set_active_storage_source(SourceType type);
    std::shared_ptr<StorageSource> get_active_storage_source();
    
private:
    void update_activate_storage_source_name_to_db();
    
    std::string m_user_id;
    std::string m_init_source_result;
    size_t m_profit_ws_schedule_task_id = 0;

    SourceType  m_last_source_type = SourceType::NONE;
    std::shared_ptr<StorageSource> m_storage_source = nullptr; 
};

#endif //USER_H
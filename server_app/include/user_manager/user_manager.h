#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include <util_macros.h>
#include <user_manager/user.h>

class UserManager
{
    Singleton(UserManager);

public:
    void init();
    std::shared_ptr<User> get_user_by_id(const std::string& user_id);

private:
    std::unordered_map<std::string, std::shared_ptr<User>> m_user_list;
    long m_back_testing_callback_id = -1;

    void register_back_testing_callback();
};

#endif //USER_MANAGER_H
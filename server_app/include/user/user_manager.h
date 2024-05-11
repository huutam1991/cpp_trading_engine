#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include <util_macros.h>
#include <app_constants.h>
#include <user/user.h>

class UserManager
{
    Singleton(UserManager);

public:
    void init();
    bool register_new_user(const std::string& username, const std::string& password);
    std::shared_ptr<User> get_user_by_id(const std::string& user_id);

private:
    std::unordered_map<std::string, std::shared_ptr<User>> m_user_list;

};

#endif //USER_MANAGER_H
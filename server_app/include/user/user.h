#ifndef USER_H
#define USER_H

#include <memory>
#include <string>

class User
{
public:
    User(const std::string& user_id);
    const std::string& get_user_id();

private:
    std::string m_user_id;
};

#endif //USER_H
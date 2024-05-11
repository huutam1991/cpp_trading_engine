
#include <mongo_db/mongo_db.h>
#include <timer.h>
#include <user/user.h>

User::User(const std::string& user_id) : m_user_id(user_id)
{}

const std::string& User::get_user_id()
{
    return m_user_id;
}

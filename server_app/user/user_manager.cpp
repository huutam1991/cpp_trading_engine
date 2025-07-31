#include <openssl/sha.h>
#include <iomanip>

#include <user/user_manager.h>
#include <mongo_db/mongo_db.h>

void UserManager::init()
{
    JsonNew user_activate_source_list = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "user")
        .find_many({});
}

bool UserManager::register_new_user(const std::string& username, const std::string& password)
{
    JsonNew user_account = MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "user")
        .find_one("username", username);

    // Username already exist
    if (user_account != nullptr)
    {
        return false;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned char upassword[password.size()];
    std::copy(password.begin(), password.end(), upassword);

    SHA256(upassword, password.size(), hash);

    std::stringstream ss;

    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
    }
    std::string hashed_password = ss.str();

    user_account["username"] = username;
    user_account["password"] = hashed_password;

    MongoDB::instance()
        .set_db_and_collection(APP_INFO_DB_NAME, "user")
        .insert_one(user_account);

    return true;
}

std::shared_ptr<User> UserManager::get_user_by_id(const std::string& user_id)
{
    // First, find on user list
    auto it = m_user_list.find(user_id);
    if (it != m_user_list.end())
    {
        return it->second;
    }

    // Create a new User object if cannot find on user list
    std::shared_ptr<User> user = std::make_shared<User>(user_id);
    m_user_list.insert(std::make_pair(user_id, user));

    return user;
}
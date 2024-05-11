#include <user_manager/user_manager.h>
#include <mongo_db/mongo_db.h>
#include <back_testing/back_testing.h>

void UserManager::init()
{
    Json user_activate_source_list = MongoDB::instance()
        .set_db_and_collection(USER_DB_NAME, "activate_source")
        .find_many({});

    user_activate_source_list.for_each([this](Json& activate_source)
    {
        std::string user_id = activate_source["user_id"];
        std::string source_name = activate_source["source"];
        SourceType type = StorageSource::get_source_type_by_name(source_name);

        // Add to m_user_list
        std::shared_ptr<User> user = std::make_shared<User>(user_id);
        user->set_active_storage_source(type);
        m_user_list.insert(std::make_pair(user_id, user));
    });

    register_back_testing_callback();
}

void UserManager::register_back_testing_callback()
{
    m_back_testing_callback_id = BackTesting::instance().register_callback_back_testing_mode([this](bool is_back_testing_mode)
    {
        if (is_back_testing_mode == true)
        {
            for (auto& user : m_user_list)
            {
                user.second->set_active_storage_source(SourceType::BINANCE_SIMULATOR);
            }
        }
        else
        {
            for (auto& user : m_user_list)
            {
                user.second->set_active_storage_source(user.second->get_last_source_type());
            }
        }
    });
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
    user->set_active_storage_source(SourceType::NONE); // Set active source is None by default
    m_user_list.insert(std::make_pair(user_id, user));

    return user;
}
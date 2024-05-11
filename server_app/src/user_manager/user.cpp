#include <user_manager/user.h>
#include <mongo_db/mongo_db.h>
#include <timer.h>

User::User(const std::string& user_id) : m_user_id(user_id)
{}

std::string User::set_active_storage_source(SourceType type)
{
    // Save last source type
    if (m_storage_source != nullptr)
    {
        m_last_source_type = m_storage_source->get_source_type();
    }

    m_storage_source = StorageSource::generate_storage_souce_by_type(type);
    m_storage_source->set_user_id(m_user_id);
    m_init_source_result = m_storage_source->init_info();

    update_activate_storage_source_name_to_db();

    ADD_LOG("User [" + m_user_id + "] is using source: [" + m_storage_source->get_db_name() + "]");

    return m_init_source_result;
}

void User::update_activate_storage_source_name_to_db()
{
    Json data;
    data["user_id"] = m_user_id;
    data["source"] = m_storage_source->get_db_name();

    MongoQuery query = MongoDB::instance()
        .set_db_and_collection(USER_DB_NAME, "activate_source");

    int count = query.count_documents("user_id", m_user_id);
    if (count == 0)
    {
        query.insert_one(data);
    }
    else
    {
        query.replace_one("user_id", m_user_id, data);
    }
}

const std::string& User::get_user_id()
{
    return m_user_id;
}

const std::string User::get_storage_source_db_name()
{
    return m_storage_source->get_db_name();
}

const std::string& User::get_init_storage_source_result()
{
    return m_init_source_result;
}

std::shared_ptr<StorageSource> User::get_active_storage_source()
{
    return m_storage_source;
}

SourceType User::get_last_source_type()
{
    return m_last_source_type;
}
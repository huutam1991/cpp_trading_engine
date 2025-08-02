#include <cstdlib>

#include <mongo_db/mongo_db.h>

mongocxx::pool& MongoDB::get_pool()
{
    if (m_pool == nullptr)
    {
        const char* uri = MONGO_URI;
        if (const char* env = std::getenv("PROD"))
        {
            if (std::string(env) == "true")
            {
                uri = MONGO_URI_PROD;
            }
        }
        spdlog::debug("MONGO_URI = {}", uri);

        // Use SpinLock to ensure thread safety when initializing the pool
        SpinLockGuard lock(m_spin_lock);

        if (m_pool == nullptr)
        {
            m_pool = new mongocxx::pool{mongocxx::uri{uri}};
        }
    }

    return *m_pool;
}

int MongoDB::count_collections(const std::string& db_name)
{
    COUNT_COLLECTIONS(db_name, count);
    return count;
}

void MongoDB::drop_collection(const std::string& db_name, const std::string& collection_name)
{
    GET_COLLECTION(db_name, collection_name, collection);
    collection.drop();
}

std::vector<std::string> MongoDB::get_collection_name_list(const std::string& db_name)
{
    GET_COLLECTION_NAME_LIST(db_name, collection_name_list);
    return collection_name_list;
}

MongoQuery MongoDB::set_db_and_collection(const std::string& db_name, const std::string& collection_name)
{
    return MongoQuery(db_name, collection_name);
}

MongoQuery::MongoQuery(const std::string& db_name, const std::string& collection_name) : m_db(db_name), m_collection(collection_name)
{}

size_t MongoQuery::count_documents(const bsoncxx::v_noabi::document::view_or_value& filter)
{
    GET_COLLECTION(m_db, m_collection, collection);
    return (size_t)collection.count_documents(filter);
}

size_t MongoQuery::count_documents()
{
    GET_COLLECTION(m_db, m_collection, collection);
    return (size_t)collection.count_documents({});
}

std::string MongoQuery::insert_one(const Json& data)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::document::value doc_value = bsoncxx::from_json(data.get_string_value());
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(doc_value.view());

    return result->inserted_id().get_oid().value.to_string();
}

void MongoQuery::drop()
{
    GET_COLLECTION(m_db, m_collection, collection);
    collection.drop();
}

Json MongoQuery::find_any()
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one({});

    if (find)
    {
        return Json::parse(bsoncxx::to_json(find.value()));
    }

    return nullptr;
}

Json MongoQuery::find_one(const bsoncxx::v_noabi::document::view_or_value& filter)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one(filter);

    if (find)
    {
        return Json::parse(bsoncxx::to_json(find.value()));
    }

    return nullptr;
}

Json MongoQuery::find_many(const bsoncxx::v_noabi::document::view_or_value& filter)
{
    GET_COLLECTION(m_db, m_collection, collection);
    mongocxx::cursor cursor = collection.find(filter);

    Json list;

    for (auto doc : cursor) {
        Json data = Json::parse(bsoncxx::to_json(doc));

        if (data != nullptr)
        {
            list.push_back(data);
        }
    }

    return list;
}

Json MongoQuery::find_many()
{
    return find_many({});
}
#ifndef MONGO_DB_H
#define MONGO_DB_H

#include <string>
#include <mutex>
#include <utility>
#include <unordered_map>
#include <tuple>

#include <util_macros.h>
#include <constants.h>
#include <json/json.h>
#include "mongo_db_header.h"

using mongo_find = bsoncxx::stdx::optional<bsoncxx::document::value>;
using mongo_view = bsoncxx::document::view;

class MongoQuery
{
    const std::string m_db;
    const std::string m_collection;

public:
    MongoQuery(const std::string& db_name, const std::string& collection_name);
    MongoQuery() = delete;

    // Count methods
    template<class T>
    size_t count_documents(const std::string& find_key, const T& find_value);
    size_t count_documents(const bsoncxx::v_noabi::document::view_or_value& filter);
    size_t count_documents();

    // Insert methods
    std::string insert_one(const Json& data);

    // Replace methods
    template<class T>
    bool replace_one(const std::string& find_key, const T& find_value, const Json& data);

    // Update methods
    template<class T, class U>
    bool update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const U& update_value);
    template<class T>
    bool update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const Json& update_value);

    // Delete methods
    template<class T>
    bool delete_one(const std::string& find_key, const T& find_value);
    void drop();

    // Find methods
    Json find_any();
    template<class T>
    Json find_one(const std::string& find_key, const T& find_value);
    Json find_one(const bsoncxx::v_noabi::document::view_or_value& filter);
    Json find_many(const bsoncxx::v_noabi::document::view_or_value& filter);
    Json find_many();
};

class MongoDB
{
    Singleton(MongoDB)

private:
    mongocxx::instance m_instance{};
    mongocxx::pool* m_pool = nullptr;

    std::mutex m_mutex;

    std::string m_db;
    std::string m_collection;

public:
    mongocxx::pool& get_pool();
    int count_collections(const std::string& db_name);
    void drop_collection(const std::string& db_name, const std::string& collection_name);
    std::vector<std::string> get_collection_name_list(const std::string& db_name);

    MongoQuery set_db_and_collection(const std::string& db_name, const std::string& collection_name);
};

template<class T>
size_t MongoQuery::count_documents(const std::string& find_key, const T& find_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::v_noabi::document::view_or_value filter = document{} << find_key << find_value << finalize;

    return (size_t)collection.count_documents(filter);
}

template<class T>
bool MongoQuery::replace_one(const std::string& find_key, const T& find_value, const Json& data)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::document::value doc_value = bsoncxx::from_json(data.get_string_value());
    bsoncxx::stdx::optional<mongocxx::result::replace_one> result =
        collection.replace_one(document{} << find_key << find_value << finalize, doc_value.view());

    return result ? true : false;
}

template<class T, class U>
bool MongoQuery::update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const U& update_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::stdx::optional<mongocxx::result::update> result =
        collection.update_one(document{} << find_key << find_value << finalize,
        document{} << "$set" << open_document << update_key << update_value << close_document << finalize);

    return result ? true : false;
}

template<class T>
bool MongoQuery::update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const Json& update_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::document::value doc_value = bsoncxx::from_json(update_value.get_string_value());
    bsoncxx::stdx::optional<mongocxx::result::update> result =
        collection.update_one(document{} << find_key << find_value << finalize,
        document{} << "$set" << open_document << update_key << doc_value.view() << close_document << finalize);

    return result ? true : false;
}

template<class T>
bool MongoQuery::delete_one(const std::string& find_key, const T& find_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::stdx::optional<mongocxx::result::delete_result> result =
        collection.delete_one(document{} << find_key << find_value << finalize);

    return result ? true : false;
}

template<class T>
Json MongoQuery::find_one(const std::string& find_key, const T& find_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::v_noabi::document::view_or_value filter = document{} << find_key << find_value << finalize;
    bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one(filter);

    if (find)
    {
        return Json::parse(bsoncxx::to_json(find.value()));
    }

    return JsonNull();
}

#endif //MONGO_DB_H

#pragma once

#include <string>
#include <utility>
#include <unordered_map>
#include <tuple>

#include <utils/util_macros.h>
#include <utils/constants.h>
#include <c_json/json.h>
#include <utils/spin_lock.h>
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
    std::string insert_one(const JsonNew& data);

    // Replace methods
    template<class T>
    bool replace_one(const std::string& find_key, const T& find_value, const JsonNew& data);

    // Update methods
    template<class T, class U>
    bool update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const U& update_value);
    template<class T>
    bool update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const JsonNew& update_value);

    // Delete methods
    template<class T>
    bool delete_one(const std::string& find_key, const T& find_value);
    void drop();

    // Find methods
    JsonNew find_any();
    template<class T>
    JsonNew find_one(const std::string& find_key, const T& find_value);
    JsonNew find_one(const bsoncxx::v_noabi::document::view_or_value& filter);
    JsonNew find_many(const bsoncxx::v_noabi::document::view_or_value& filter);
    JsonNew find_many();
};

class MongoDB
{
    Singleton(MongoDB)

private:
    mongocxx::instance m_instance{};
    mongocxx::pool* m_pool = nullptr;
    SpinLock m_spin_lock;

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
bool MongoQuery::replace_one(const std::string& find_key, const T& find_value, const JsonNew& data)
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

    // MongoDB doesn't support size_t, so have to cast it to int64_t
    auto update_builder = bsoncxx::builder::stream::document{};
    if constexpr (std::is_same<U, size_t>::value) {
        update_builder << update_key << static_cast<int64_t>(update_value);
    } else {
        update_builder << update_key << update_value;
    }

    bsoncxx::stdx::optional<mongocxx::result::update> result =
        collection.update_one(document{} << find_key << find_value << finalize,
        document{} << "$set" << update_builder << finalize);

    return result ? true : false;
}

template<class T>
bool MongoQuery::update_one(const std::string& find_key, const T& find_value, const std::string& update_key, const JsonNew& update_value)
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
JsonNew MongoQuery::find_one(const std::string& find_key, const T& find_value)
{
    GET_COLLECTION(m_db, m_collection, collection);
    bsoncxx::v_noabi::document::view_or_value filter = document{} << find_key << find_value << finalize;
    bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one(filter);

    if (find)
    {
        return JsonNew::parse(bsoncxx::to_json(find.value()));
    }

    return nullptr;
}

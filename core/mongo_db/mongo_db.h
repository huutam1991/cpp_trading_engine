#pragma once

#include <string>
#include <utility>
#include <unordered_map>
#include <tuple>
#include <atomic>
#include <cstdint>
#include <x86intrin.h>

#include <utils/util_macros.h>
#include <utils/constants.h>
#include <core_dump_diagnostics/gdb_support.h>
#include <json/json.h>
#include <time/measure_time.h>
#include <utils/spin_lock.h>
#include "mongo_db_header.h"

using mongo_find = bsoncxx::stdx::optional<bsoncxx::document::value>;
using mongo_view = bsoncxx::document::view;

// Diagnostic sequence: 1, 2, 3, ... for replace_one calls.
inline std::atomic<uint64_t> g_mongo_replace_seq{0};

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

// template<class T>
// bool MongoQuery::replace_one(const std::string& find_key, const T& find_value, const Json& data)
// {
//     GET_COLLECTION(m_db, m_collection, collection);
//     bsoncxx::document::value doc_value = bsoncxx::from_json(data.get_string_value());
//     bsoncxx::stdx::optional<mongocxx::result::replace_one> result =
//         collection.replace_one(document{} << find_key << find_value << finalize, doc_value.view());

//     return result ? true : false;
// }

template<class T>
GDB_DIAGNOSTIC_FUNCTION
bool MongoQuery::replace_one(const std::string& find_key, const T& find_value, const Json& data)
{
    MeasureTime measure_time("MongoQuery::replace_one");
    GET_COLLECTION(m_db, m_collection, collection);

    std::string raw_json = data.get_string_value();

    const char* raw_json_ptr = raw_json.c_str();
    const size_t raw_json_size = raw_json.size();

    const uint64_t mongo_seq =
        g_mongo_replace_seq.fetch_add(1, std::memory_order_relaxed) + 1;

    _mm_lfence();

    // This is the ONE cross-thread diagnostic value for replace_one().
    //
    // While replace_one() is inside the blocking MongoDB call, the shared value
    // contains its start TSC. As soon as replace_one() returns (or throws), the
    // value is reset to zero. Therefore an MPSC producer that reaches REAL FULL
    // can tell whether replace_one() is currently active and, if so, calculate
    // exactly how many TSC ticks it has been active.
    uint64_t mongo_replace_one_start_tsc = __rdtsc();

    KEEP_FOR_GDB(raw_json);
    KEEP_FOR_GDB(raw_json_ptr);
    KEEP_FOR_GDB(raw_json_size);
    KEEP_FOR_GDB(mongo_seq);
    KEEP_FOR_GDB_SHARE_BETWEEN_THREADS(mongo_replace_one_start_tsc);

    bsoncxx::document::value doc_value =
        bsoncxx::from_json(raw_json);

    try
    {
        auto result = collection.replace_one(
            document{} << find_key << find_value << finalize,
            doc_value.view());

        // replace_one() is no longer active. Clear the process-wide shared
        // timestamp before any following task can be mistaken for this Mongo
        // operation.
        mongo_replace_one_start_tsc = 0;
        KEEP_FOR_GDB_SHARE_BETWEEN_THREADS(mongo_replace_one_start_tsc);

        // Keep the ordinary diagnostic payload alive for unrelated crashes that
        // may still occur after the Mongo call returns.
        KEEP_FOR_GDB(raw_json);
        KEEP_FOR_GDB(raw_json_ptr);
        KEEP_FOR_GDB(raw_json_size);
        KEEP_FOR_GDB(mongo_seq);

        return result ? true : false;
    }
    catch (...)
    {
        // Never leave a stale non-zero "Mongo active" timestamp behind if the
        // driver exits through an exception.
        mongo_replace_one_start_tsc = 0;
        KEEP_FOR_GDB_SHARE_BETWEEN_THREADS(mongo_replace_one_start_tsc);
        throw;
    }
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

    return nullptr;
}

#pragma once

#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>
// #include <glog/logging.h>

#define Singleton(className) \
public: \
    className(className const&)         = delete; \
    className& operator=(className const&)    = delete; \
    static className& instance() { \
        static className instance; \
        return instance; \
    } \
private: \
    className() = default; \
    ~className() = default;

#define Static_Property(propertyName, type) \
static type& get_##propertyName() \
{ \
    static type m_##propertyName; \
    return m_##propertyName; \
} \

#define FORCE_INLINE inline __attribute__((always_inline))
#define SAFE_RELEASE(pointer) if (pointer != nullptr) { delete pointer; pointer = nullptr; }

#define GET_CURRENT_TIME(current_time) \
auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

#define STRING_UPPER_CASE(data) \
std::for_each(data.begin(), data.end(), [](char & c) \
{ \
    c = ::toupper(c); \
});

#define STRING_LOWER_CASE(data) \
std::for_each(data.begin(), data.end(), [](char & c) \
{ \
    c = ::tolower(c); \
});


#define ADD_ROUTE_GROUP(request_type, path) \
RouteController::instance().add_route_group(request_type, path) + [](HttpRequest* request) -> Task<HttpResponse>

#define ADD_ROUTE(request_type, path) \
RouteController::instance().add_route(request_type, path) + [](HttpRequest* request) -> Task<HttpResponse>

#define ADD_CUSTOM_BAD_REQUEST \
HttpRequest::add_custom_bad_request_getter([](HttpRequest* request) -> HttpResponse

#define GET_CLIENT(client) \
auto c = MongoDB::instance().get_pool().acquire(); \
mongocxx::client& client = *c;

#define GET_DB(db, dbname) \
auto c = MongoDB::instance().get_pool().acquire(); \
mongocxx::client& client = *c; \
mongocxx::database db = client[dbname];

#define COUNT_COLLECTIONS(dbname, count) \
auto c = MongoDB::instance().get_pool().acquire(); \
mongocxx::client& client = *c; \
mongocxx::database db = client[dbname]; \
int count = 0; \
auto cursor1 = db.list_collections(); \
for (const bsoncxx::document::view& doc :cursor1) \
{ \
    count++; \
} \

#define GET_COLLECTION_NAME_LIST(dbname, collection_name_list) \
auto c = MongoDB::instance().get_pool().acquire(); \
mongocxx::client& client = *c; \
mongocxx::database db = client[dbname]; \
std::vector<std::string> collection_name_list; \
auto cursor1 = db.list_collections(); \
for (const bsoncxx::document::view& doc :cursor1)  \
{ \
    bsoncxx::document::element ele = doc["name"]; \
    std::string name = ele.get_utf8().value.to_string(); \
    collection_name_list.push_back(name); \
} \

#define GET_COLLECTION(dbname, collection_name, collection) \
auto c = MongoDB::instance().get_pool().acquire(); \
mongocxx::client& client = *c; \
mongocxx::database db = client[dbname]; \
mongocxx::collection collection = db[collection_name];

#define COUNT_DOCUMENT(collection, field_key, field_value, count) \
bsoncxx::v_noabi::document::view_or_value filter = document{} << field_key << field_value << finalize; \
int count = collection.count_documents(filter);

#define CHECK_ERROR(view, code, msg) \
int code; \
std::string msg; \
auto it = view.find("code"); \
if (it != view.end()) \
{ \
    bsoncxx::document::element code_field = view["code"]; \
    code = code_field.get_int32().value; \
    if (code < 0) \
    { \
        bsoncxx::document::element msg_field = view["msg"]; \
        msg = msg_field.get_utf8().value.to_string(); \
    } \
}

#define INSERT_ONE(collection, data, result) \
bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(data.view());

#define UPDATE_ONE(collection, search_key, search_value, update_key, update_value, result) \
bsoncxx::stdx::optional<mongocxx::result::update> result = \
collection.update_one(document{} << search_key << search_value << finalize, \
document{} << "$set" << open_document << update_key << update_value << close_document << finalize);

#define FIND_ONE(collection, find_key, find_value, find, view) \
bsoncxx::v_noabi::document::view_or_value filter = document{} << find_key << find_value << finalize; \
bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one(filter); \
bsoncxx::document::view view; \
if (find) \
{ \
    view = find.value(); \
}

#define FIND_ANY(collection, find, view) \
bsoncxx::stdx::optional<bsoncxx::document::value> find = collection.find_one({}); \
bsoncxx::document::view view; \
if (find) \
{ \
    view = find.value(); \
}

#define REPLACE_ONE(collection, search_key, search_value, data, result) \
bsoncxx::stdx::optional<mongocxx::result::replace_one> result = \
collection.replace_one(document{} << search_key << search_value << finalize, data.view());

#define DELETE_ONE(collection, key, value, result) \
bsoncxx::stdx::optional<mongocxx::result::delete_result> result = collection.delete_one(document{} << key << value << finalize);

#define TO_STRING Utils::to_string_with_precision
#define IS_EQUAL Utils::is_equal

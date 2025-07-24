#include <c_json/json.h>
#include <c_json/json_object.h>
#include <c_json/json_value.h>

JsonNew::JsonNew()
    : m_value(JsonObjectPool::acquire()) // Default to a JsonObject
{}
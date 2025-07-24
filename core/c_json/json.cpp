#include <c_json/json.h>
#include <c_json/json_object.h>
#include <c_json/json_value.h>

JsonNew::JsonNew()
    : m_value(JsonObjectPool::acquire()) // Default to a JsonObject
{}

void JsonNew::check_create_json_value()
{
    if (m_value == nullptr || m_value->is_json_value() == false)
    {
        m_value = JsonValuePool::acquire();
    }
}
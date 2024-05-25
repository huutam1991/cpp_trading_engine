#ifndef DATA_MODEL_H
#define DATA_MODEL_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <data_model/data_field.h>

class DataModel
{
public:
    DataModel();
    DataModel(JsonNull);
    DataModel(const DataModel& copy);
    DataModel(const std::string& db, const std::string& collection);
    DataModel(const std::string& db, const std::string& collection, const std::string& id);

    DataModel& operator=(const Json& json);
    DataModel& operator=(const DataModel& copy);
    DataField& operator[](const char* key);
    DataField& operator[](const std::string& key);

    operator Json();
    Json& get_data();

    friend std::ostream& operator<<(std::ostream& cout, const DataModel& data_model);

    template<class T>
    operator T();

    // Update
    template<class T>
    void update_field(const std::string& key, const T& value);

    // DB
    void set_db(const std::string& db);

    // Collection
    void set_collection(const std::string& collection);

    // Id
    std::string get_id();
    void set_id(const std::string& id);

    void set_callback(std::function<void(Json&)> callback);
    static std::vector<DataModel> get_data_model_list(const std::string& db, const std::string& collection);
    static std::unordered_map<std::string, DataModel> get_data_model_map(const std::string& db, const std::string& collection, const std::string& key_field_name);

protected:
    std::string m_id = "-1";
    std::string m_db;
    std::string m_collection;

    std::shared_ptr<Json> m_data;

private:
    template<class T>
    void update_field_to_DB(const std::string& key, const T& value);

    void save_to_DB();
    void get_from_DB();
    void check_invoke_callback();

    bool m_has_checked_available = false;
    DataField m_data_field = DataField(this);
    std::function<void(Json&)> m_callback = nullptr;
};

template<class T>
void DataModel::update_field(const std::string& key, const T& value)
{
    (*m_data)[key] = value;
    update_field_to_DB(key, value);
    check_invoke_callback();
}

template<class T>
void DataModel::update_field_to_DB(const std::string& key, const T& value)
{
    // Check to insert initial data
    MongoQuery query = MongoDB::instance().set_db_and_collection(m_db, m_collection);
    if (m_id == "-1")
    {
        m_id = query.insert_one(*m_data);
    }

    // Update field
    query.update_one("_id", bsoncxx::oid(m_id), key, value);
}

template<class T>
DataModel::operator T()
{
    return (T)(*m_data);
}

template<class T>
DataField::operator T()
{
    return (T)(*m_field);
}

template<class T>
DataField& DataField::operator=(const T& data)
{
    *m_field = data;

    if (m_root_field == m_field)
    {
        m_parent->update_field(m_field_name, data);
    }
    else
    {
        m_parent->update_field(m_field_name, *m_root_field);
    }

    return *this;
}

template<class T>
bool DataField::operator==(const T& data)
{
    T val = (*m_field);
    return val == data;
}

#endif //DATA_MODEL_H
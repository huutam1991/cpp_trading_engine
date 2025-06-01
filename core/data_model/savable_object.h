#pragma once

#include <data_model/data_model.h>

template<class T>
class SavableObject
{
    T m_object;
    DataModel m_data_model;
    
    std::string m_id = "-1";
    std::string m_db;
    std::string m_collection;
    
    SavableObject() {}

public:
    SavableObject(const std::string& db, const std::string& collection)
        : m_data_model{m_db, m_collection}, m_db{db}, m_collection{collection}
    {}

    SavableObject& operator=(const T& value)
    {
        m_object = value;
        m_data_model = m_object.to_json();
        return *this;
    }

    SavableObject& operator=(T&& value)
    {
        m_object = std::move(value);
        m_data_model = m_object.to_json();
        return *this;
    }

    Json to_json()
    {
        return m_object.to_json();
    }

    T from_json(Json& data)
    {
        return T::from_json(data);
    }

    static SavableObject load_single_object(const std::string& db, const std::string& collection)
    {
        SavableObject res;
        res.m_db = db;
        res.m_collection = collection;
        res.m_data_model = DataModel::load_single_data_model(db, collection);
        res.m_object = T::from_json(res.m_data_model.get_data());

        return res;
    }

    template<class Key>
    static std::unordered_map<Key, SavableObject> load_objects_map(const std::string& db, const std::string& collection, const std::string& key_field_name)
    {
        std::unordered_map<Key, DataModel> data_list = DataModel::load_data_model_map<Key>(db, collection, key_field_name);

        std::unordered_map<Key, SavableObject> res;
        for (auto& [key, dm] : data_list)
        {
            SavableObject object;
            object.m_db = db;
            object.m_collection = collection;
            object.m_data_model = dm;
            object.m_object = T::from_json(dm.get_data());

            res.insert(std::make_pair(key, object));
        }

        return res;
    }

};
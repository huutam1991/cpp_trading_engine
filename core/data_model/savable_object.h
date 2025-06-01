#pragma once

#include <data_model/data_model.h>
#include <coroutine/event_base.h>

template<class T>
class SavableObject
{
    std::shared_ptr<DataModel> m_data_model = nullptr;
    
    std::string m_db;
    std::string m_collection;
    
    SavableObject() : m_data_model{std::make_shared<DataModel>()} {}

public:
    T object;

    SavableObject(const std::string& db, const std::string& collection)
        : m_data_model{std::make_shared<DataModel>()}, m_db{db}, m_collection{collection}
    {
        init_data_model(m_data_model.get(), db, collection).start_running_on(get_even_base());
    }

    SavableObject& operator=(const T& value)
    {
        object = value;
        update_data_model(m_data_model.get(), object).start_running_on(get_even_base());
        return *this;
    }

    SavableObject& operator=(T&& value)
    {
        object = std::move(value);
        update_data_model(m_data_model.get(), object).start_running_on(get_even_base());
        return *this;
    }

    operator T&()
    {
        return object;
    }

    Json to_json()
    {
        return object.to_json();
    }

    T from_json(Json& data)
    {
        return T::from_json(data);
    }

    void remove()
    {
        m_data_model->remove();
    }
    
    TaskVoid init_data_model(DataModel* data_model, std::string db, std::string collection)
    {
        DataModel dm(db, collection);
        *data_model = dm;
        co_return;
    }

    TaskVoid update_data_model(DataModel* data_model, T object)
    {
        *data_model = object.to_json();
        co_return;
    }

    static void init(EventBase* event_base)
    {
        get_even_base() = event_base;
    }

    static EventBase*& get_even_base()
    {
        static EventBase* even_base = nullptr;
        return even_base;
    }

    static SavableObject load_single_object(const std::string& db, const std::string& collection)
    {
        SavableObject res;
        res.m_db = db;
        res.m_collection = collection;
        res.m_data_model = DataModel::load_single_data_model(db, collection);
        res.object = T::from_json(res.m_data_model->get_data());

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
            object.object = T::from_json(dm.get_data());

            res.insert(std::make_pair(key, object));
        }

        return res;
    }
};
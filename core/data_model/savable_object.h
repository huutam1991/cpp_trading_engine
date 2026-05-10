#pragma once

#include <data_model/data_model.h>
#include <coroutine/event_base.h>
#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>

class DBHelper
{
public:
    static EpollBase* get_epoll_base()
    {
        static EpollBase* epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::SYSTEM_IO_TASK);
        return epoll_base;
    }
};

template<class T>
class SavableObject
{
    T object;
    std::shared_ptr<DataModel> m_data_model = nullptr;

    std::string m_db;
    std::string m_collection;

    SavableObject() : m_data_model{std::make_shared<DataModel>()} {}

public:
    SavableObject(const std::string& db, const std::string& collection)
        : m_data_model{std::make_shared<DataModel>()}, m_db{db}, m_collection{collection}
    {
        init_data_model(m_data_model, db, collection).start_running_on(DBHelper::get_epoll_base());
    }

    SavableObject(const std::string& db, const std::string& collection, T data)
        : m_data_model{std::make_shared<DataModel>()}, m_db{db}, m_collection{collection}, object{std::move(data)}
    {
        init_data_model(m_data_model, db, collection).start_running_on(DBHelper::get_epoll_base());
        update_data_model(m_data_model, object).start_running_on(DBHelper::get_epoll_base());
    }

    SavableObject& operator=(const T& value)
    {
        object = value;
        update_data_model(m_data_model, object).start_running_on(DBHelper::get_epoll_base());
        return *this;
    }

    SavableObject& operator=(T&& value)
    {
        object = std::move(value);
        update_data_model(m_data_model, object).start_running_on(DBHelper::get_epoll_base());
        return *this;
    }

    void remove()
    {
        remove_data_model(m_data_model).start_running_on(DBHelper::get_epoll_base());
    }

    static Task<void> init_data_model(std::shared_ptr<DataModel> data_model, std::string db, std::string collection)
    {
        DataModel dm(db, collection);
        *data_model = dm;
        co_return;
    }

    static Task<void> update_data_model(std::shared_ptr<DataModel> data_model, T object)
    {
        *data_model = object.to_json();
        co_return;
    }

    static Task<void> remove_data_model(std::shared_ptr<DataModel> data_model)
    {
        data_model->remove();
        co_return;
    }

    T* operator->()
    {
        return &object;
    }

    const T* operator->() const
    {
        return &object;
    }

    operator const T&()
    {
        return object;
    }

    Json to_json() const
    {
        return object.to_json();
    }

    T from_json(Json& data)
    {
        return T::from_json(data);
    }

    static SavableObject load_single_object(const std::string& db, const std::string& collection)
    {
        SavableObject res;
        DataModel dm = DataModel::load_single_data_model(db, collection);
        res.m_db = db;
        res.m_collection = collection;
        res.object = T::from_json(dm.get_data());
        *res.m_data_model = dm;

        // Single object can be empty one (in the first load) so need to update it with data from [object]
        update_data_model(res.m_data_model, res.object).start_running_on(DBHelper::get_epoll_base());

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
            object.object = T::from_json(dm.get_data());
            *object.m_data_model = dm;

            res.insert(std::make_pair(key, object));
        }

        return res;
    }

    static std::unordered_map<std::string, SavableObject> load_objects_map(const std::string& db, const std::string& collection)
    {
        std::unordered_map<std::string, DataModel> data_list = DataModel::load_data_model_map(db, collection);

        std::unordered_map<std::string, SavableObject> res;
        for (auto& [key, dm] : data_list)
        {
            SavableObject object;
            object.m_db = db;
            object.m_collection = collection;
            object.object = T::from_json(dm.get_data());
            *object.m_data_model = dm;

            res.insert(std::make_pair(key, object));
        }

        return res;
    }
};
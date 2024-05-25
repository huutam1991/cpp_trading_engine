#include <data_model/data_model.h>
#include <app_utils.h>

DataModel::DataModel()
{
    m_data = std::make_shared<Json>();
}

DataModel::DataModel(JsonNull)
{
    m_data = std::make_shared<Json>(JsonNull());
}

DataModel::DataModel(const DataModel& copy) : m_db(copy.m_db), m_collection(copy.m_collection), m_id(copy.m_id), m_data(copy.m_data)
{
}

DataModel::DataModel(const std::string& db, const std::string& collection) : m_db(db), m_collection(collection)
{
    m_data = std::make_shared<Json>();
}

DataModel::DataModel(const std::string& db, const std::string& collection, const std::string& id) : m_db(db), m_collection(collection), m_id(id)
{
    m_data = std::make_shared<Json>();
    get_from_DB();
}

DataModel& DataModel::operator=(const DataModel& copy)
{
    m_db = copy.m_db;
    m_collection = copy.m_collection;
    m_id = copy.m_id;
    m_data = copy.m_data;

    return *this;
}

std::ostream& operator<<(std::ostream& cout, const DataModel& data_model)
{
    cout << *data_model.m_data;
    return cout;
}

void DataModel::get_from_DB()
{
    *m_data = MongoDB::instance()
        .set_db_and_collection(m_db, m_collection)
        .find_one("_id", bsoncxx::oid(m_id));
}

DataModel::operator Json()
{
    return *m_data;
}

DataModel& DataModel::operator=(const Json& json)
{
    // Update data in memory
    *m_data = json;

    // Update data in DB
    save_to_DB();

    // Check invoke callback
    check_invoke_callback();

    return *this;
}

DataField& DataModel::operator[](const std::string& key)
{
    Json& field = (*m_data)[key];
    // root_field == field at the beginning
    m_data_field.m_root_field = &field;
    m_data_field.m_field = &field;
    m_data_field.m_field_name = key;

    return m_data_field;
}

DataField& DataModel::operator[](const char* key)
{
    return operator[](std::string(key));
}

void DataModel::save_to_DB()
{
    MongoQuery query = MongoDB::instance().set_db_and_collection(m_db, m_collection);

    if (m_id == "-1")
    {
        m_id = query.insert_one(*m_data);
    }
    else
    {
        if (m_has_checked_available == false)
        {
            size_t count = query.count_documents("_id", bsoncxx::oid(m_id));
            if (count == 0)
            {
                m_id = query.insert_one(*m_data);
            }
            else
            {
                query.replace_one("_id", bsoncxx::oid(m_id), *m_data);
            }

            m_has_checked_available = true;
        }
        else
        {
            query.replace_one("_id", bsoncxx::oid(m_id), *m_data);
        }
    }
}

void DataModel::set_db(const std::string& db)
{
    m_db = db;
}

void DataModel::set_collection(const std::string& collection)
{
    m_collection = collection;
}

std::string DataModel::get_id()
{
    return m_id;
}

void DataModel::set_id(const std::string& id)
{
    m_id = id;
}

Json& DataModel::get_data()
{
    return *m_data;
}

void DataModel::check_invoke_callback()
{
    if (m_callback != nullptr)
    {
        m_callback(*m_data);
    }
}

void DataModel::set_callback(std::function<void(Json&)> callback)
{
    m_callback = callback;
}

std::vector<DataModel> DataModel::get_data_model_list(const std::string& db, const std::string& collection)
{
    Json data_list = MongoDB::instance()
        .set_db_and_collection(db, collection)
        .find_many();

    std::vector<DataModel> res;
    data_list.for_each_with_index([&res, &db, &collection](size_t index, Json& data)
    {
        std::string _id = data["_id"]["$oid"];
        res.emplace_back(db, collection, _id);
    });

    return res;
}

std::unordered_map<std::string, DataModel> DataModel::get_data_model_map(const std::string& db, const std::string& collection, const std::string& key_field_name)
{
    Json data_list = MongoDB::instance()
        .set_db_and_collection(db, collection)
        .find_many();

    std::unordered_map<std::string, DataModel> res;
    data_list.for_each_with_index([&res, &db, &collection, &key_field_name](size_t index, Json& data)
    {
        std::string _id = data["_id"]["$oid"];
        DataModel dm(db, collection, _id);
        std::string key_id = dm[key_field_name];

        res.insert(std::make_pair(key_id, dm));
    });

    return res;
}
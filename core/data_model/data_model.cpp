#include <data_model/data_model.h>

DataModel::DataModel()
{
    m_data = std::make_shared<JsonNew>();
}

DataModel::DataModel(std::nullptr_t t)
{
    m_data = std::make_shared<JsonNew>(nullptr);
}

DataModel::DataModel(const DataModel& copy) : m_db(copy.m_db), m_collection(copy.m_collection), m_id(copy.m_id), m_data(copy.m_data)
{
}

DataModel::DataModel(DataModel&& copy) : m_db(std::move(copy.m_db)), m_collection(std::move(copy.m_collection)), m_id(std::move(copy.m_id)), m_data(std::move(copy.m_data))
{
}


DataModel::DataModel(const std::string& db, const std::string& collection) : m_db(db), m_collection(collection)
{
    m_data = std::make_shared<JsonNew>();

    // Save empty data to DB, but this action can init [m_id] and make this DataModel a real one
    save_to_DB();
}

DataModel::DataModel(const std::string& db, const std::string& collection, const std::string& id) : m_db(db), m_collection(collection), m_id(id)
{
    m_data = std::make_shared<JsonNew>();
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
    cout << data_model.m_data->get_string_value();
    return cout;
}

void DataModel::get_from_DB()
{
    *m_data = MongoDB::instance()
        .set_db_and_collection(m_db, m_collection)
        .find_one("_id", bsoncxx::oid(m_id));
}

bool DataModel::is_null() const
{
    return *m_data == nullptr;
}

DataModel::operator JsonNew()
{
    return *m_data;
}

DataModel& DataModel::operator=(const JsonNew& json)
{
    // Update data in memory
    *m_data = json;

    // Update data in DB
    save_to_DB();

    return *this;
}

DataField& DataModel::operator[](const std::string& key)
{
    JsonNew& field = (*m_data)[key];
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

void DataModel::remove()
{
    MongoDB::instance()
        .set_db_and_collection(m_db, m_collection)
        .delete_one("_id", bsoncxx::oid(m_id));

    // Reset [m_id] + [m_data]
    m_id = "-1";
    *m_data = JsonNew();
}

JsonNew& DataModel::get_data()
{
    return *m_data;
}

DataModel DataModel::load_single_data_model(const std::string& db, const std::string& collection)
{
    std::vector<DataModel> list = DataModel::load_data_model_list(db, collection);
    if (list.size() > 0)
    {
        return list[0];
    }

    // Create 1
    DataModel dm(db, collection);
    return dm;
}

std::vector<DataModel> DataModel::load_data_model_list(const std::string& db, const std::string& collection)
{
    JsonNew data_list = MongoDB::instance()
        .set_db_and_collection(db, collection)
        .find_many();

    std::vector<DataModel> res;
    data_list.for_each_with_index([&res, &db, &collection](size_t index, JsonNew& data)
    {
        std::string _id = data["_id"]["$oid"];
        res.emplace_back(db, collection, _id);
    });

    return res;
}
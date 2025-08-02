#pragma once

#include <memory>
#include <iostream>

#include <json/json.h>

class DataModel;

class DataField
{
protected:
    DataModel* m_parent;

public:
    DataField();
    DataField(DataModel* parent);
    DataField(DataModel* parent, Json* root_field, Json* field, const std::string& field_name);
    ~DataField()
    {
        // // LOG(INFO) << "DataField, name = " << m_field_name << std::endl;
    }

    Json* m_root_field;
    Json* m_field;
    std::string m_field_name;

    DataField& operator=(const DataField& copy);

    operator Json();

    template<class T>
    operator T();

    template<class T>
    DataField& operator=(const T& data);

    template<class T>
    bool operator==(const T& data);

    DataField& operator[](const std::string& key);
    DataField& operator[](const char* key);
};

#pragma once

#include <memory>
#include <iostream>

#include <c_json/json.h>

class DataModel;

class DataField
{
protected:
    DataModel* m_parent;

public:
    DataField();
    DataField(DataModel* parent);
    DataField(DataModel* parent, JsonNew* root_field, JsonNew* field, const std::string& field_name);
    ~DataField()
    {
        // // LOG(INFO) << "DataField, name = " << m_field_name << std::endl;
    }

    JsonNew* m_root_field;
    JsonNew* m_field;
    std::string m_field_name;

    DataField& operator=(const DataField& copy);

    operator JsonNew();

    template<class T>
    operator T();

    template<class T>
    DataField& operator=(const T& data);

    template<class T>
    bool operator==(const T& data);

    DataField& operator[](const std::string& key);
    DataField& operator[](const char* key);
};

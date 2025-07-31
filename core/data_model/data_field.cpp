#include <data_model/data_field.h>
#include <data_model/data_model.h>

DataField::DataField()
{}

DataField::DataField(DataModel* parent) : m_parent(parent)
{}

DataField::DataField(DataModel* parent, JsonNew* root_field, JsonNew* field, const std::string& field_name)
    : m_parent(parent), m_root_field(root_field), m_field(field), m_field_name(field_name)
{
}

DataField& DataField::operator=(const DataField& copy)
{
    m_parent = copy.m_parent;
    m_root_field = copy.m_root_field;
    m_field = copy.m_field;
    m_field_name = copy.m_field_name;

    return *this;
}

DataField::operator JsonNew()
{
    return *m_field;
}

DataField& DataField::operator[](const std::string& key)
{
    m_field = &(*m_field)[key];
    return *this;
}

DataField& DataField::operator[](const char* key)
{
    return operator[](std::string(key));
}
#include <cache/share_string.h>
#include <time/measure_time.h>

ShareString::ShareString(const std::string& data) :
    m_string_reference{StringPool::acquire()}
{
    m_string_reference->data = data;
    m_string_reference->count = 1;
}

ShareString::ShareString(std::string&& data) :
    m_string_reference{StringPool::acquire()}
{
    m_string_reference->data = std::move(data);
    m_string_reference->count = 1;
}

ShareString::ShareString(const ShareString& copy) :
    m_string_reference{copy.m_string_reference}
{
    m_string_reference->count++;
}

ShareString& ShareString::operator=(const ShareString& copy)
{
    // Release current data if it exists
    check_release_current_data();

    m_string_reference = copy.m_string_reference;
    m_string_reference->count++;

    return *this;
}

ShareString::~ShareString()
{
    check_release_current_data();
}

void ShareString::check_release_current_data()
{
    if (m_string_reference != nullptr)
    {
        m_string_reference->count--;
        if (m_string_reference->count == 0)
        {
            StringPool::release(m_string_reference);
        }

        m_string_reference = nullptr;
    }
}
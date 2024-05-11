#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <string>
#include <stdexcept>

class HttpSocketException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class EpollException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class HttpParserException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class ApiException : public std::exception
{
private:
    const std::string m_msg;

public:
    ApiException(const std::string msg) : m_msg(msg)
    {}

    virtual const std::string msg() const throw()
    {
        return m_msg;
    }
};

#endif //EXCEPTIONS_H

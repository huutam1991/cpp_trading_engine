#pragma once

#include <string>
#include <cxxabi.h>

namespace type_name
{

template <typename T>
constexpr std::string demangled_name()
{
    int status;
    char* realname = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    std::string result = (status == 0 && realname) ? realname : typeid(T).name();
    std::free(realname);
    return result;
}

template <typename T, typename U = void>
struct TypeName
{
    static constexpr std::string name()
    {
        return demangled_name<T>();
    }
};

template <typename T>
struct TypeName<T, std::void_t<decltype(T::name())>>
{
    static constexpr std::string name()
    {
        return T::name();
    }
};

} // namespace type_name
#pragma once

#include <array>
#include <string_view>

namespace enum_reflect
{
    namespace detail
    {
        #define ENUM_MAX 200  // Max number of enum values to scan

        // Get the compiler-specific function signature at compile time
        template<class E, E V>
        constexpr std::string_view enum_name_raw() {
        #if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
        #elif defined(_MSC_VER)
            return __FUNCSIG__;
        #else
        # error Unsupported compiler
        #endif
        }

        // Parse enum name from compiler signature string
        template<class E, E V>
        constexpr std::string_view parse_enum_name()
        {
            constexpr std::string_view full = enum_name_raw<E, V>();
            constexpr std::string_view key = "E V = ";
            auto start = full.find(key);
            if (start == std::string_view::npos) return {};
            start += key.size();

            auto end = full.find(';', start); // For Clang
            if (end == std::string_view::npos) end = full.find(']', start); // For GCC
            if (end == std::string_view::npos) return {};

            auto name = full.substr(start, end - start);
            auto sep = name.rfind("::");
            return (sep != std::string_view::npos) ? name.substr(sep + 2) : name;
        }

        // Helper to expand enum value indices and build array of names
        template<class E, std::size_t... I>
        constexpr auto build_enum_names_impl(std::index_sequence<I...>)
        {
            return std::array<std::string_view, sizeof...(I)>
            {
                parse_enum_name<E, static_cast<E>(I)>()...
            };
        }

        // Build array of enum names for enum values 0 to N-1
        template<class E, std::size_t N>
        constexpr auto build_enum_names()
        {
            return build_enum_names_impl<E>(std::make_index_sequence<N>{});
        }

        // Compile-time global enum-to-string table
        template<class E>
        constexpr std::array<std::string_view, ENUM_MAX> enum_table = build_enum_names<E, ENUM_MAX>();
    }

// Convert enum value to string name
template<class E>
constexpr std::string_view enum_name(E value)
{
    return detail::enum_table<E>[static_cast<int>(value)];
}

// Convert string name to enum value
template<class E>
constexpr E enum_value(std::string_view name)
{
    for (std::size_t i = 0; i < detail::enum_table<E>.size(); ++i)
    {
        if (detail::enum_table<E>[i] == name)
        {
            return static_cast<E>(i);
        }
    }
    // If not found, return the first enum value as default
    return static_cast<E>(0);
}

} // namespace enum_reflect
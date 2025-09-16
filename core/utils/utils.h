#pragma once

#include <time.h>       /* time_t, struct tm, time, gmtime */

#include <utils/constants.h>
#include <utils/util_macros.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <thread_pool/thread_pool.h>
#include <cmath>

#define UTC_PLUS_7_IN_MS 25200000
#define UTC_PLUS_7_IN_S 25200

typedef unsigned char BYTE;

class Utils
{
    Singleton(Utils);

public:
    size_t get_time_now_in_utc_seconds();
    size_t get_time_now_in_utc_milliseconds();
    size_t get_time_now_in_utc_nanoseconds();
    size_t get_time_now_in_utc();
    size_t get_0h_today_in_utc();
    size_t get_0h_tomorrow_in_utc();
    size_t get_0h_by_number_of_day_before_in_utc(size_t number_of_date_before);

    std::string get_string_time(time_t time, time_t offset = UTC_PLUS_7_IN_S);
    std::string get_string_time_YMD(time_t time, time_t offset = UTC_PLUS_7_IN_S);
    std::string get_string_time_YMD_with_millisecond(time_t time, time_t offset = UTC_PLUS_7_IN_MS);

    std::vector<std::string> split_string(const std::string& str, const std::string& del);
    static std::string round_string_number(const std::string& str_number, size_t precision);
    size_t get_decimal_digits(const std::string& str);

    std::string get_request_method_string_by_id(RequestMethod method);
    std::string base64_encode(BYTE const* buf, unsigned int bufLen);
    std::vector<BYTE> base64_decode(std::string const& encoded_string);

    long double round_with_decimal(const long double value, const long decimal_places);

    template <typename T>
    std::string to_string_with_precision(const T a_value, const int n = 6)
    {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return out.str();
    }

    template <typename T>
    bool is_equal(const T value1, const T value2)
    {
        return std::fabs(value1 - value2) <= std::numeric_limits<T>::epsilon();
    }
};

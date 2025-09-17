#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <math.h>

#include <utils/utils.h>

size_t Utils::get_time_now_in_utc()
{
    time_t rawtime;

    // Get number of seconds since 00:00 UTC Jan, 1, 1970 and store in rawtime
    time ( &rawtime );

    return rawtime;
}

size_t Utils::get_0h_today_in_utc()
{
    struct tm * ptm;
    time_t rawtime = get_time_now_in_utc();

    // UTC struct tm
    ptm = gmtime ( &rawtime );

    return rawtime - 3600 * ptm->tm_hour - 60 * ptm->tm_min - ptm->tm_sec;
}

size_t Utils::get_0h_tomorrow_in_utc()
{
    return get_0h_today_in_utc() + 86400;
}

size_t Utils::get_0h_by_number_of_day_before_in_utc(size_t number_of_day_before)
{
    return get_0h_today_in_utc() - 86400 * number_of_day_before;
}

std::string Utils::get_string_time(time_t time_val, time_t offset)
{
    time_val += offset;

    struct tm * timeinfo;
    char buffer[80];

    timeinfo = gmtime(&time_val);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);

    return std::string(buffer);
}

std::string Utils::get_string_time_YMD(time_t time_val, time_t offset)
{
    time_val += offset;

    struct tm * timeinfo;
    char buffer[80];

    timeinfo = gmtime(&time_val);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    return std::string(buffer);
}

std::string Utils::get_string_time_YMD_with_millisecond(time_t time_val, time_t offset)
{
    time_val += offset;

    struct tm * timeinfo = nullptr;
    int millisec = time_val % 1000;
    time_val = time_val / 1000;
    char buffer[80];
    char mili_buffer[5];

    timeinfo = gmtime(&time_val);
    if (timeinfo)
    {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

        // millisecond
        sprintf(mili_buffer, "%03d", millisec);

        return std::string(buffer) + ":" + std::string(mili_buffer);
    }

    return "";
}

std::vector<std::string> Utils::split_string(const std::string& str, const std::string& del)
{
    std::vector<std::string> res;
    int start, end = -1 * del.size();

    do
    {
        start = end + del.size();
        end = str.find(del, start);
        if (end != -1)
        {
            res.push_back(str.substr(start, end - start));
        }
        else if (str.size() - start >= 0)
        {
            res.push_back(str.substr(start, str.size() - start));
        }
    }
    while (end != -1);

    return res;
}

std::string Utils::get_request_method_string_by_id(RequestMethod method)
{
    std::string res;

    switch (method)
    {
    case RequestMethod::GET:
        res = "GET";
        break;
    case RequestMethod::POST:
        res = "POST";
        break;
    case RequestMethod::PUT:
        res = "PUT";
        break;
    case RequestMethod::DELETE:
        res = "DELETE";
        break;

    default:
        res = "GET";
        break;
    }

    return res;
}

std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(BYTE c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Utils::base64_encode(BYTE const* buf, unsigned int bufLen)
{
    std::string ret;
    int i = 0;
    int j = 0;
    BYTE char_array_3[3];
    BYTE char_array_4[4];

    while (bufLen--)
    {
        char_array_3[i++] = *(buf++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];

            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
        char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
        ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
        ret += '=';
    }

    return ret;
}

std::vector<BYTE> Utils::base64_decode(std::string const& encoded_string)
{
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
    {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4)
        {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

size_t Utils::get_decimal_digits(const std::string& str)
{
    int pos = str.find_first_not_of("0."); // find the position of charater # '0' and '.'
    return pos - 1;
}

std::string Utils::round_string_number(const std::string& str_number, size_t precision)
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

long double Utils::round_with_decimal(const long double value, const long decimal_places)
{
    long double multiplier = std::pow(10.0, decimal_places);
    return std::round(value * multiplier) / multiplier;
}

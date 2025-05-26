#include <utils/utils.h>

namespace time_log
{
    std::string get_time_now_in_string()
    {
        Utils& utils = Utils::instance();
        return utils.get_string_time(utils.get_time_now_in_utc());
    }
};

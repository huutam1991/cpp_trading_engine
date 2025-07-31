#pragma once

#include <memory>
#include <unordered_map>

#include <utils/util_macros.h>
#include <c_json/json.h>

#include <user/user.h>

class StrategyReport
{
    Singleton(StrategyReport);

protected:

public:
    void export_24h_strategy_report(User* user);
    void export_strategy_report_by_time(User* user, long from, long to);
    void generate_excel_file(JsonNew& trading_strategy_result);

};

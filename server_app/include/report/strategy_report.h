#ifndef EXCEL_REPORT_H
#define EXCEL_REPORT_H

#include <memory>
#include <unordered_map>

#include <utils/util_macros.h>
#include <json/json.h>

class StrategyReport
{
    Singleton(StrategyReport);

protected:

public:
    void export_24h_strategy_report(User* user);
    void export_strategy_report_by_time(User* user, long from, long to);
    void generate_excel_file(Json& trading_strategy_result);

};

#endif //EXCEL_REPORT_H

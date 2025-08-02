#pragma once

#include <memory>
#include <unordered_map>

#include <utils/util_macros.h>
#include <json/json.h>

#include <user/user.h>

class ExcelReport
{
    Singleton(ExcelReport);

protected:

public:
    void export_24h_price_ticker(User* user);
    void export_price_ticker_by_date(User* user, long from, long to);
    void generate_excel_file(Json& price_ticker_list, Json& execution_report_list);

};

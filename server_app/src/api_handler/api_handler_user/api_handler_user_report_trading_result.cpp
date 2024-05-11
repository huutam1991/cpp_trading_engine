#include <api_handler/api_handler_user/api_handler_user_report_trading_result.h>

#include <app_utils.h>
#include <report/strategy_report.h>

APIHandlerUserReportTradingResult::APIHandlerUserReportTradingResult(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"from", "to"});
}

HttpResponse APIHandlerUserReportTradingResult::child_handle()
{
    long from = std::stol(m_request->get_query_param("from"));
    long to   = std::stol(m_request->get_query_param("to"));

    StrategyReport::instance().export_strategy_report_by_time(m_user.get(), from, to);

    return m_request->send_file_from_directory("/excel/strategy_report.xls", "strategy_report.xls");
}
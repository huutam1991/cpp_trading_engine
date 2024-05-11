#include <api_handler/api_handler_user/api_handler_user_report_price_ticker.h>
#include <app_utils.h>
#include <report/excel_report.h>

APIHandlerUserReportPriceTicker::APIHandlerUserReportPriceTicker(HttpRequest* request) : APIHandler(request)
{
    m_need_check_authentication = true;
    add_mandatory_params({"from", "to"});
}

HttpResponse APIHandlerUserReportPriceTicker::child_handle()
{
    long from = std::stol(m_request->get_query_param("from"));
    long to   = std::stol(m_request->get_query_param("to"));

    ExcelReport::instance().export_price_ticker_by_date(m_user.get(), from, to);

    return m_request->send_file_from_directory("/excel/report.xls", "report_price_ticker.xls");
}
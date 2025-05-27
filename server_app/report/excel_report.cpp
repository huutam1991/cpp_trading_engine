// #include <excel/libxl.h>
// #include <excel/setup.h>
// #include <utils/util_macros.h>
// #include <utils/utils.h>

// #include <binance_utils.h>
// #include <report/excel_report.h>
// #include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>

// void ExcelReport::export_24h_price_ticker(User* user)
// {
//     Json price_ticker_list = BinanceUtils::instance().get_48h_price_ticker(user);
//     Json execution_report_list = APIHandlerBinance24hProfit::get_filled_order_list_by_days_ago(user->get_active_storage_source().get(), 2);

//     generate_excel_file(price_ticker_list, execution_report_list);
// }

// void ExcelReport::export_price_ticker_by_date(User* user, long from, long to)
// {
//     Json price_ticker_list = BinanceUtils::instance().get_price_ticker_by_date(user, from, to);
//     Json execution_report_list = APIHandlerBinance24hProfit::get_filled_order_list_by_day(user->get_active_storage_source().get(), from, to);

//     generate_excel_file(price_ticker_list, execution_report_list);
// }

// void ExcelReport::generate_excel_file(Json& price_ticker_list, Json& execution_report_list)
// {
//     execution_report_list.sort([](Json& a, Json& b){
//         return (long)a["transactTime"] > (long)b["transactTime"];
//     });

//     libxl::Book* book = xlCreateBook();
//     if(book)
//     {
//         libxl::Sheet* sheet = book->addSheet("24h Price Ticker");
//         if(sheet)
//         {
//             // Add column title
//             libxl::Format* title_format = book->addFormat();
//             libxl::Font* boldFont = book->addFont();
//             boldFont->setBold();
//             title_format->setFont(boldFont);
//             title_format->setAlignH(libxl::ALIGNH_CENTER);
//             title_format->setAlignV(libxl::ALIGNV_CENTER);

//             sheet->writeStr(2, 0, "Date Time", title_format);
//             sheet->writeStr(2, 1, "User", title_format);
//             sheet->writeStr(2, 2, "Symbol", title_format);
//             sheet->writeStr(2, 3, "Strategy", title_format);
//             sheet->writeStr(2, 4, "Bid Price", title_format);
//             sheet->writeStr(2, 5, "Ask Price", title_format);
//             sheet->writeStr(2, 6, "Ticker Time", title_format);
//             sheet->writeStr(2, 7, "Finish Calculation", title_format);
//             sheet->writeStr(2, 8, "Finish Place Order", title_format);
//             sheet->setRow(2, 20);
//             sheet->setCol(0, 0, 25);
//             sheet->setCol(1, 2, 12);
//             sheet->setCol(3, 3, 18);
//             sheet->setCol(4, 5, 17);
//             sheet->setCol(6, 8, 25);

//             // Format default
//             libxl::Format* format = book->addFormat();
//             format->setAlignH(libxl::ALIGNH_RIGHT);
//             format->setAlignV(libxl::ALIGNV_BOTTOM);

//             // Format Market Scanning
//             libxl::Format* format_MS = book->addFormat();
//             libxl::Font* font_MS = book->addFont();
//             font_MS->setColor(libxl::COLOR_GREEN);
//             format_MS->setFont(font_MS);
//             format_MS->setAlignH(libxl::ALIGNH_RIGHT);
//             format_MS->setAlignV(libxl::ALIGNV_BOTTOM);

//             // Format MM Arbitrage
//             libxl::Font* font_MMA = book->addFont();
//             libxl::Format* format_MMA = book->addFormat();
//             font_MMA->setColor(libxl::COLOR_BLUE);
//             format_MMA->setFont(font_MMA);
//             format_MMA->setAlignH(libxl::ALIGNH_RIGHT);
//             format_MMA->setAlignV(libxl::ALIGNV_BOTTOM);

//             std::string status;
//             std::string orderId;
//             int line_counter = 0;

//             execution_report_list.for_each([
//                 &book,
//                 &sheet,
//                 format,
//                 format_MS,
//                 format_MMA,
//                 &price_ticker_list,
//                 &status,
//                 &orderId,
//                 &line_counter
//             ] (Json& report)
//             {
//                 orderId = std::to_string((long)report["orderId"]);
//                 if (price_ticker_list.has_field(orderId))
//                 {
//                     Json json = price_ticker_list[orderId];

//                     json["user_id"].set_is_string_format(false);
//                     json["symbol"].set_is_string_format(false);
//                     std::string date_time = Utils::instance().get_string_time_YMD(((long long)json["transactTime"] / 1000));
//                     std::string on_tick_time = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["on_tick_time"]);
//                     std::string finish_calculation_time = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["finish_calculation_time"]);
//                     std::string finish_place_order_time = Utils::instance().get_string_time_YMD_with_millisecond((long long)json["finish_place_order_time"]);

//                     int row = line_counter++ + 3;

//                     // Strategy type
//                     if (((std::string&&)json["type"]) == "MS")
//                     {
//                         sheet->writeStr(row, 3, "Market Scanning", format_MS);
//                     }
//                     else
//                     {
//                         sheet->writeStr(row, 3, "MM Arbitrage", format_MMA);
//                     }

//                     sheet->writeStr(row, 0, date_time.c_str(), format);
//                     sheet->writeStr(row, 1, json["user_id"].get_string_value().c_str(), format);
//                     sheet->writeStr(row, 2, json["symbol"].get_string_value().c_str(), format);
//                     sheet->writeStr(row, 4, json["bid"].get_string_value().c_str(), format);
//                     sheet->writeStr(row, 5, json["ask"].get_string_value().c_str(), format);
//                     sheet->writeStr(row, 6, on_tick_time.c_str(), format);
//                     sheet->writeStr(row, 7, finish_calculation_time.c_str(), format);
//                     sheet->writeStr(row, 8, finish_place_order_time.c_str(), format);

//                     sheet->setRow(row, 15);
//                 }
//             });
//         }

//         if(book->save("web_data/excel/report.xls")) {
//     	}
//         book->release();
//     }
// }
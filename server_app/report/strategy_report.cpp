// #include <excel/libxl.h>
// #include <excel/setup.h>
// #include <utils/util_macros.h>
// #include <utils/utils.h>

// #include <app_utils/app_utils.h>
// #include <report/strategy_report.h>

// void StrategyReport::export_24h_strategy_report(User* user)
// {
//     long today_0h = Utils::instance().get_0h_today_in_utc() * 1000;
//     long tomorrow_0h = Utils::instance().get_0h_tomorrow_in_utc() * 1000;

//     JsonNew trading_strategy_result = AppUtils::instance().get_trading_strategy_result(user, today_0h, tomorrow_0h);
//     generate_excel_file(trading_strategy_result);
// }

// void StrategyReport::export_strategy_report_by_time(User* user, long from, long to)
// {
//     JsonNew trading_strategy_result = AppUtils::instance().get_trading_strategy_result(user, from, to);
//     generate_excel_file(trading_strategy_result);
// }

// void StrategyReport::generate_excel_file(JsonNew& trading_strategy_result)
// {
//     libxl::Book* book = xlCreateBook();
//     if(book)
//     {
//         libxl::Sheet* sheet = book->addSheet("Trading Strategy Result");
//         if(sheet)
//         {
//             // Add column title
//             libxl::Format* title_format = book->addFormat();
//             libxl::Font* boldFont = book->addFont();
//             boldFont->setBold();
//             title_format->setFont(boldFont);
//             title_format->setAlignH(libxl::ALIGNH_CENTER);
//             title_format->setAlignV(libxl::ALIGNV_CENTER);

//             sheet->writeStr(2, 1, "Strategy", title_format);
//             sheet->writeStr(2, 2, "Symbols", title_format);
//             sheet->writeStr(2, 3, "User", title_format);
//             sheet->writeStr(2, 4, "Hit Time", title_format);
//             sheet->writeStr(2, 5, "Profit", title_format);
//             sheet->setRow(2, 20);
//             sheet->setCol(1, 1, 20);
//             sheet->setCol(2, 2, 35);
//             sheet->setCol(3, 3, 7);
//             sheet->setCol(4, 4, 20);
//             sheet->setCol(5, 5, 12);

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

//             trading_strategy_result.for_each_with_key([
//                 &book,
//                 &sheet,
//                 format,
//                 format_MS,
//                 format_MMA,
//                 &status,
//                 &orderId,
//                 &line_counter
//             ] (const std::string& strategy_name, JsonNew& symbol_list)
//             {
//                 symbol_list.for_each_with_key([
//                     &book,
//                     &sheet,
//                     &line_counter,
//                     format,
//                     format_MS,
//                     format_MMA,
//                     &strategy_name
//                 ](const std::string& symbol_name, JsonNew& trading_result_list)
//                 {
//                     trading_result_list.for_each([
//                         &book,
//                         &sheet,
//                         &line_counter,
//                         format,
//                         format_MS,
//                         format_MMA,
//                         &strategy_name
//                     ]
//                     (JsonNew& trading_result)
//                     {
//                         int row = line_counter++ + 3;

//                         trading_result["user_id"].set_is_string_format(false);
//                         trading_result["hit_time_str"].set_is_string_format(false);

//                         // Strategy type
//                         if (strategy_name == "Market Scanning")
//                         {
//                             sheet->writeStr(row, 1, strategy_name.c_str(), format_MS);
//                         }
//                         else
//                         {
//                             sheet->writeStr(row, 1, strategy_name.c_str(), format_MMA);
//                         }

//                         sheet->writeStr(row, 2, trading_result["symbol_list"].get_string_value().c_str(), format);
//                         sheet->writeStr(row, 3, trading_result["user_id"].get_string_value().c_str(), format);
//                         sheet->writeStr(row, 4, trading_result["hit_time_str"].get_string_value().c_str(), format);
//                         sheet->writeStr(row, 5, trading_result["profit"].get_string_value().c_str(), format);

//                         sheet->setRow(row, 15);
//                     });
//                 });
//             });
//         }

//         if(book->save("web_data/excel/strategy_report.xls")) {
//     	}
//         book->release();
//     }
// }
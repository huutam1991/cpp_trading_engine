#include <api_header.h>
#include <data_model/data_model.h>

#include <back_testing/back_testing.h>
#include <strategy_engine/blvt_scanning/scan_blvt_info.h>
#include <asset_manager/binance_asset_manager.h>

// Binance
#include <api_handler/api_handler_binance_spot/api_handler_binance_depth.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_current_price.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_create_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_get_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_cancel_order.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_get_open_orders.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_all_symbols.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_exchange_info.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_profit.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance_24h_orders.h>

// Coinbase
#include <api_handler/api_handler_coinbase/api_handler_coinbase_current_price.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_order.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_cancel_order.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_orders.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_fills.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_all_transfers.h>
#include <api_handler/api_handler_coinbase/api_handler_coinbase_get_balance.h>

// User
#include <api_handler/api_handler_user/api_handler_user_config.h>
#include <api_handler/api_handler_user/api_handler_user_config_get.h>
#include <api_handler/api_handler_user/api_handler_user_login.h>
#include <api_handler/api_handler_user/api_handler_user_register.h>
#include <api_handler/api_handler_user/api_handler_user_websocket_token.h>
#include <api_handler/api_handler_user/api_handler_user_active_source.h>
#include <api_handler/api_handler_user/api_handler_user_delete_source.h>
#include <api_handler/api_handler_user/api_handler_user_update_source_info.h>
#include <api_handler/api_handler_user/api_handler_user_update_source_info_put.h>
#include <api_handler/api_handler_user/api_handler_user_info.h>
#include <api_handler/api_handler_user/api_handler_user_symbol.h>
#include <api_handler/api_handler_user/api_handler_user_symbol_get.h>
#include <api_handler/api_handler_user/api_handler_user_price_ticker_list.h>
#include <api_handler/api_handler_user/api_handler_user_report_price_ticker.h>
#include <api_handler/api_handler_user/api_handler_user_report_trading_result.h>
#include <api_handler/api_handler_user/api_handler_user_auto_trade_info.h>
#include <api_handler/api_handler_user/api_handler_user_auto_trade_info_get.h>
#include <api_handler/api_handler_user/api_handler_user_trading_result_list.h>

// Source
#include <api_handler/api_handler_source/api_handler_source_available.h>

// Strategy
#include <api_handler/api_handler_strategy/api_handler_strategy_post.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_put.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_delete.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_list.h>
#include <api_handler/api_handler_strategy/api_handler_last_strategy_id.h>

// Back Testing
#include <api_handler/api_back_testing/api_handler_back_testing_set_config.h>
#include <api_handler/api_back_testing/api_handler_back_testing_switch_mode.h>
#include <api_handler/api_back_testing/api_handler_back_testing_collection_name_list.h>

std::string CLIENT_DEPLOY_FOLDER = "angular_src/dist/alpha-h-trading";

using namespace std;

void add_app_route()
{
    RouteController::instance().add_dashboard_folder(CLIENT_DEPLOY_FOLDER);

    ADD_ROUTE_GROUP(RequestMethod::GET, "libraries")
    {
        return request->send_file_from_directory(request->get_url());
    };

    ADD_ROUTE_GROUP(RequestMethod::GET, "templates")
    {
        return request->send_file_from_directory(request->get_url());
    };

    ADD_ROUTE_GROUP(RequestMethod::GET, "#/")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/favicon.ico")
    {
        return request->send_file_from_directory("templates/favicon.ico");
    };

    ADD_ROUTE(RequestMethod::GET, "/websocket_client")
    {
        return request->send_file_from_directory("templates/websocket_client.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/scanning_market")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/excel_report_page")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/strategy_report_page")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/trade_errors_page")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/24h_orders_page")
    {
        return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::POST, "/test_json_parse")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);

        Json response;
        response["message"] = "OK";
        response["data"] = data;

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/test_json_clone")
    {
        Json json = {
            {"number", 123},
            {"string", "Tam"},
            {"bool", true},
        };

        Json clone = json.clone();
        clone["number"] = 456;
        clone["string"] = "Nguyen";
        clone["bool"] = "false";

        Json data;
        data["json"] = json;
        data["clone"] = clone;

        Json response;
        response["message"] = "OK";
        response["data"] = data;

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/test_data_model")
    {
        Json user = {
            {"name", "Nguyen Huu Tam"},
            {"age", 32},
            {"role", "Fullstack Developer"},
            {"skills", "C++, Python, Javascript, Backend Server, Web Development"},
        };

        DataModel dm("test_data_model", "user");
        dm.set_id("6411abf9a6e8c43c38086291");
        dm = user;

        dm["age"] = 23;
        dm["company"] = Json{
            {"name", "Alpha H Trading"},
            {"domain", "Crypto trading"},
            {"Tech stack", {
                {"Backend", "C++"},
                {"Client", "Python, Javascript"},
                {"Cloud", "AWS"}
            }},
        };
        dm["company"]["domain"]["Name"] = "Binance Crypto Trading";
        dm["company"]["Tech stack"]["Client"] = "Python, Javascript, HTML, CSS";
        dm["company"]["Tech stack"]["Web"]["CSS"] = "Boostrap";

        Json data = dm;

        Json response;
        response["message"] = "OK";
        response["id"] = dm.get_id();
        response["data"] = data;
        response["company"] = (Json)dm["company"]["Tech stack"];

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/test_clone")
    {
        Json data;
        Json user;

        user["age"] = 23;
        user["company"] = Json{
            {"name", "Alpha H Trading"},
            {"domain", "Crypto trading"},
            {"Tech stack", {
                {"Backend", "C++"},
                {"Client", "Python, Javascript"},
                {"Cloud", "AWS"}
            }},
        };
        user["company"]["domain"]["Name"] = "Binance Crypto Trading";
        user["company"]["Tech stack"]["Client"] = "Python, Javascript, HTML, CSS";
        user["company"]["Tech stack"]["Web"]["CSS"] = "Boostrap";

        Json user_clone = user.deep_clone();
        user_clone["company"]["domain"]["Name"] = "Stock Trading";

        data["user"] = user;
        data["user_clone"] = user_clone;

        Json response;
        response["message"] = "OK";
        response["data"] = data;

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/add_trade")
    {
        MongoDB::instance()
            .set_db_and_collection("test1", "trade")
            .insert_one(Json::parse(request->get_body()));

        Json response;
        response["message"] = "OK";

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/update_trade")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);
        long trade_id = data["trade_id"];
        std::string symbol = data["symbol"];

        MongoDB::instance().set_db_and_collection("test1", "trade")
            .update_one("trade_id", trade_id, "symbol", symbol);

        Json response;
        response["message"] = "OK";

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/delete_trade")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);
        long trade_id = data["trade_id"];

        MongoDB::instance()
            .set_db_and_collection("test1", "trade")
            .delete_one("trade_id", trade_id);

        Json response;
        response["message"] = "OK";

        return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/external_request")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);

        std::string response = ExternalRequest("www.google.com", 80, "/", RequestMethod::GET).send_request();

        return HttpResponse(OK_200, response);
    };

    // ******************************** Binance Rest API
    // Binance's depth
    ADD_ROUTE(RequestMethod::GET, "/depth")
    {
        APIHandlerBinanceDepth api_handler(request);
        api_handler.add_mandatory_params({"symbol"});
        return api_handler.handle();
    };

    // Binance's current price
    ADD_ROUTE(RequestMethod::GET, "/current_price")
    {
        return APIHandlerBinanceCurrentPrice(request).handle();
    };

    // Binance's cancel order
    ADD_ROUTE(RequestMethod::DELETE, "/cancel_order")
    {
        return APIHandlerBinanceCancelOrder(request).handle();
    };

    // Binance's order
    ADD_ROUTE(RequestMethod::POST, "/order")
    {
        return APIHandlerBinanceCreateOrder(request).handle();
    };

    // Binance's get order info
    ADD_ROUTE(RequestMethod::GET, "/order")
    {
        return APIHandlerBinanceGetOrder(request).handle();
    };

    // Binance's order non-cancel
    ADD_ROUTE(RequestMethod::POST, "/order_non_cancel")
    {
        return APIHandlerBinanceCreateOrder(request).handle();
    };

    // Binance's open orders
    ADD_ROUTE(RequestMethod::GET, "/open_orders")
    {
        return APIHandlerBinanceGetOpenOrders(request).handle();
    };

    // Binance's all symbols
    ADD_ROUTE(RequestMethod::GET, "/all_symbols")
    {
        return APIHandlerBinanceAllSymbols(request).handle();
    };

    // Binance's symbol info
    ADD_ROUTE(RequestMethod::GET, "/symbol_info")
    {
        return APIHandlerBinanceExchangeInfo(request).handle();
    };

    // Binance's 24h profit
    ADD_ROUTE(RequestMethod::GET, "/24h_profit")
    {
        return APIHandlerBinance24hProfit(request).handle();
    };

    // Binance's 24h orders list
    ADD_ROUTE(RequestMethod::GET, "/24h_orders")
    {
        return APIHandlerBinance24hOrders(request).handle();
    };

    // ******************************** Coinbase Rest API
    // Coinbase's current price
    ADD_ROUTE(RequestMethod::GET, "/cb_current_price")
    {
        return APIHandlerCoinbaseCurrentPrice(request).handle();
    };

    // Coinbase's get balances
    ADD_ROUTE(RequestMethod::GET, "/cb_get_balance")
    {
        return APIHandlerCoinbaseGetBalance(request).handle();
    };

    // Coinbase's all transfer
    ADD_ROUTE(RequestMethod::GET, "/cb_get_all_transfers")
    {
        return APIHandlerCoinbaseGetAllTransfers(request).handle();
    };

    // User's config
    ADD_ROUTE(RequestMethod::POST, "/user_config")
    {
        APIHandlerUserConfig api_handler(request);
        api_handler.add_mandatory_body_params({
            "symbol",
            "source",
            "prices",
            "max_price_on_screen",
            "default_order_size",
            "order_scroll_size",
            "zoom_scroll_setting",
            "max_price_buy",
            "max_price_sell"
        });
        return api_handler.handle();
    };

    // User's config get
    ADD_ROUTE(RequestMethod::GET, "/user_config")
    {
        APIHandlerUserConfigGet api_handler(request);
        api_handler.add_mandatory_params({"symbol", "source"});
        return api_handler.handle();
    };

    // Register user
    ADD_ROUTE(RequestMethod::POST, "/register")
    {
        return APIHandlerUserRegister(request).handle();
    };

    // User's login
    ADD_ROUTE(RequestMethod::POST, "/login")
    {
        return APIHandlerUserLogin(request).handle();
    };

    // User's websocket token
    ADD_ROUTE(RequestMethod::GET, "/websocket_token")
    {
        return APIHandlerUserWebsocketToken(request).handle();
    };

    // User's update source info
    ADD_ROUTE(RequestMethod::POST, "/update_source_info")
    {
        return APIHandlerUserUpdateSourceInfo(request).handle();
    };

    // User's update source info
    ADD_ROUTE(RequestMethod::PUT, "/update_source_info")
    {
        return APIHandlerUserUpdateSourceInfoPut(request).handle();
    };

    // User's active source
    ADD_ROUTE(RequestMethod::POST, "/active_source")
    {
        return APIHandlerUserActiveSource(request).handle();
    };

    // User's active source
    ADD_ROUTE(RequestMethod::DELETE, "/delete_source")
    {
        return APIHandlerUserDeleteSource(request).handle();
    };

    // User's info
    ADD_ROUTE(RequestMethod::GET, "/user_info")
    {
        return APIHandlerUserInfo(request).handle();
    };

    // User's symbols in use - POST
    ADD_ROUTE(RequestMethod::POST, "/symbols")
    {
        return APIHandlerUserSymbol(request).handle();
    };

    // User's symbols in use - GET
    ADD_ROUTE(RequestMethod::GET, "/symbols")
    {
        return APIHandlerUserSymbolGet(request).handle();
    };

    // User's price ticker list
    ADD_ROUTE(RequestMethod::GET, "/price_ticker_list")
    {
        return APIHandlerUserPriceTickerList(request).handle();
    };

    // User's report price ticker
    ADD_ROUTE(RequestMethod::GET, "/report_price_ticker")
    {
        return APIHandlerUserReportPriceTicker(request).handle();
    };

    // User's report trading result
    ADD_ROUTE(RequestMethod::GET, "/report_trading_result")
    {
        return APIHandlerUserReportTradingResult(request).handle();
    };

    // User's trading result list
    ADD_ROUTE(RequestMethod::GET, "/trading_result_list")
    {
        return APIHandlerUserTradingResultList(request).handle();
    };

    // User's auto trade info
    ADD_ROUTE(RequestMethod::POST, "/auto_trade_info")
    {
        return APIHandlerUserAutoTradeInfo(request).handle();
    };

    // User's auto trade info - GET
    ADD_ROUTE(RequestMethod::GET, "/auto_trade_info")
    {
        return APIHandlerUserAutoTradeInfoGet(request).handle();
    };

    // Source - available source
    ADD_ROUTE(RequestMethod::GET, "/available_source")
    {
        return APIHandlerSourceAvailable(request).handle();
    };

    // ******************************** Strategy Rest API
    // Create a strategy
    ADD_ROUTE(RequestMethod::POST, "/strategy")
    {
        //implement here
        return APIHandlerStrategyPost(request).handle();
    };

    // Cancel a strategy
    ADD_ROUTE(RequestMethod::DELETE, "/strategy")
    {
        //implement here
        return APIHandlerStrategyDelete(request).handle();
    };

    // Update a strategy
    ADD_ROUTE(RequestMethod::PUT, "/strategy")
    {
        //implement here
        return APIHandlerStrategyPut(request).handle();
    };

    // Get all running strategy
    ADD_ROUTE(RequestMethod::GET, "/strategy_list")
    {
        //implement here
        return APIHandlerStrategyList(request).handle();
    };

    // Get last strategy id
    ADD_ROUTE(RequestMethod::GET, "/last_strategy_id")
    {
        //implement here
        return APIHandlerLastStrategyId(request).handle();
    };

    // ******************************** Trade history
    ADD_ROUTE(RequestMethod::GET, "/trade_history")
    {
        long date_from = stol(request->get_query_param("from"));
        long date_to = stol(request->get_query_param("to"));

        Json response;
        response["data"] = BinanceAssetManager::instance().get_trade_result(date_from, date_to);
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;
        return HttpResponse(OK_200, response);
    };

    // ******************************** current assets
    ADD_ROUTE(RequestMethod::GET, "/current_assets")
    {
        Json response;
        response["data"] = BinanceAssetManager::instance().get_current_assets();
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;
        return HttpResponse(OK_200, response);
    };

    // ******************************** Test API
    ADD_ROUTE(RequestMethod::GET, "/test_change_blvt_info")
    {
        std::string info_type = request->get_query_param("type");
        std::string symbol = request->get_query_param("symbol");

        if (info_type == "baskets")
            ScanBLVTInfo::instance().test_change_info(symbol, true);
        else
            ScanBLVTInfo::instance().test_change_info(symbol, false);

        Json response;

        response["data"] = "";
        response["msg"] = "test change baskets or token_issues";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };

 /*   // Back testing - connect to Binance Simulater service
    ADD_ROUTE(RequestMethod::GET, "/connect_binance_simulator")
    {
        Json response;
        response["data"] = {
            {"is_connected", BackTesting::instance().connect_binance_simulator()}
        };
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };

    // Back testing - get config
    ADD_ROUTE(RequestMethod::GET, "/get_config")
    {
        Json response;

        response["data"] = {
            {"speed_time", BackTesting::instance().get_speed_time()},
            {"db_name", BackTesting::instance().get_db_name()},
            {"is_start", BackTesting::instance().is_start()}
        };
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };

    // Back testing - get list of DB name
    ADD_ROUTE(RequestMethod::GET, "/get_db_name_list")
    {
        Json response;

        response["data"] = BackTesting::instance().get_db_name_list();
        response["msg"] = "";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };

    // Back testing - get list of Collection name by DB name
    ADD_ROUTE(RequestMethod::GET, "/get_collection_name_list")
    {
        return APIHandlerBackTestingCollectionNameList(request).handle();
    };

    // Back testing - set speed time
    ADD_ROUTE(RequestMethod::POST, "/set_config")
    {
        return APIHandlerBackTestingSetConfig(request).handle();
    };

    // Back testing - switch Back Testing mode
    ADD_ROUTE(RequestMethod::POST, "/switch_back_testing_mode")
    {
        return APIHandlerBackTestingSwitchMode(request).handle();
    };

    // Back testing - get Back Testing mode
    ADD_ROUTE(RequestMethod::GET, "/get_back_testing_mode")
    {
        Json response;

        bool is_back_testing_mode = BackTesting::instance().is_back_testing_mode();
        std::string mode = is_back_testing_mode == true ? "on" : "off";

        response["data"] = {
            {"back_testing_mode", mode}
        };
        response["msg"] = "OK";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };

    // Back testing - clean Back Testing data
    ADD_ROUTE(RequestMethod::GET, "/clean_back_testing_data")
    {
        Json response;

        BackTesting::instance().clean_back_testing_data();

        response["data"] = "";
        response["msg"] = "OK";
        response["status_code"] = OK_200;
        response["error"] = false;

        return HttpResponse(OK_200, response);
    };*/
}
#include <utils/api_header.h>
#include <data_model/data_model.h>
#include <coroutine/event_base_manager.h>
#include <order/order_manager.h>

// User
#include <api_handler/api_handler_user/api_handler_user_register.h>
#include <api_handler/api_handler_user/api_handler_user_login.h>

// Account
#include <api_handler/api_handler_account/api_handler_add_account.h>
#include <api_handler/api_handler_account/api_handler_add_activate_account.h>
#include <api_handler/api_handler_account/api_handler_activate_account_balances.h>

// Strategy
#include <api_handler/api_handler_strategy/api_handler_strategy_config.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_current_info.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_config.h>
#include <api_handler/api_handler_strategy/api_handler_strategy_price_arbitrage_current_info.h>

#include <gateways/gateway_manager.h>

std::string CLIENT_DEPLOY_FOLDER = "angular_src/dist/alpha-h-trading";

using namespace std;

void add_app_route()
{
    RouteController::instance().add_dashboard_folder(CLIENT_DEPLOY_FOLDER);

    ADD_ROUTE_GROUP(RequestMethod::GET, "libraries")
    {
        co_return request->send_file_from_directory(request->get_url());
    };

    ADD_ROUTE_GROUP(RequestMethod::GET, "templates")
    {
        co_return request->send_file_from_directory(request->get_url());
    };

    ADD_ROUTE_GROUP(RequestMethod::GET, "#/")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/favicon.ico")
    {
        co_return request->send_file_from_directory("templates/favicon.ico");
    };

    ADD_ROUTE(RequestMethod::GET, "/websocket_client")
    {
        co_return request->send_file_from_directory("templates/websocket_client.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/scanning_market")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/excel_report_page")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/strategy_report_page")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/trade_errors_page")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/24h_orders_page")
    {
        co_return request->send_file_from_directory(CLIENT_DEPLOY_FOLDER + "/index.html");
    };

    ADD_ROUTE(RequestMethod::GET, "/test_random")
    {
        DataModel data("test_data_model", "user");

        data["tam"] = 123;

        Json response;
        response["message"] = "OK";
        response["data"] = data.get_data();
        // response["data"] = 123123;

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/test_json_parse")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);

        Json response;
        response["message"] = "OK";
        response["data"] = data;

        co_return HttpResponse(OK_200, response);
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

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/test_new_data_model")
    {
        Json user = {
            {"name", "tam_pattern"},
            {"age", 33},
            {"role", "Trading Engine Developer"},
            {"company", "Pattern Research"},
            {"skills", "C++, Rust, gRPC, Bazel, NixOS"},
        };

        DataModel dm("test_data_model", "user");
        dm = user;

        co_return HttpResponse(OK_200, dm.get_data());
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

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::GET, "/test_data_model_get_map")
    {
        std::string name = request->get_query_param("name");
        std::unordered_map<std::string, DataModel> users = DataModel::load_data_model_map<std::string>("test_data_model", "user", "name");
        DataModel a = users[name];

        co_return HttpResponse(OK_200, a.get_data());
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

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/add_trade")
    {
        MongoDB::instance()
            .set_db_and_collection("test1", "trade")
            .insert_one(Json::parse(request->get_body()));

        Json response;
        response["message"] = "OK";

        co_return HttpResponse(OK_200, response);
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

        co_return HttpResponse(OK_200, response);
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

        co_return HttpResponse(OK_200, response);
    };

    ADD_ROUTE(RequestMethod::POST, "/test_place")
    {
        Json params = request->get_body_json();

        std::string symbol = params["symbol"];
        std::string side = params["side"];
        std::string type = params["type"];
        double price = params["price"];
        double quantity = params["quantity"];

        Order order(
            OrderManager::generate_order_id(),
            InstrumentType::SPOT,
            Order::Status::NOT_AVAILABLE,
            Instrument::get_instrument_by_symbol(ExchangeId::BINANCE, InstrumentType::SPOT, symbol),
            enum_reflect::enum_value<Order::Side>(side),
            enum_reflect::enum_value<Order::OrderType>(type),
            price,
            quantity
        );

        GatewayManager::instance()
            .get_gateway(ExchangeId::BINANCE)
            ->place_none_wait(order);

        co_return HttpResponse(OK_200, Json());
    };

    ADD_ROUTE(RequestMethod::POST, "/external_request")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);

        std::string response = ExternalRequest("www.google.com", 80, "/", RequestMethod::GET).send_request();

        co_return HttpResponse(OK_200, response);
    };
    
    // Register new user
    ADD_ROUTE(RequestMethod::POST, "/register_new_user")
    {
        co_return co_await APIHandlerUserRegister(request).handle();
    };

    // User's login
    ADD_ROUTE(RequestMethod::POST, "/login")
    {
        co_return co_await APIHandlerUserLogin(request).handle();
    };

    // Add account
    ADD_ROUTE(RequestMethod::POST, "/add_account")
    {
        co_return co_await APIHandlerAddAccount(request).handle();
    };

    // Add activate account
    ADD_ROUTE(RequestMethod::POST, "/add_activate_account")
    {
        co_return co_await APIHandlerAddActivateAccount(request).handle();
    };

    // Add activate account
    ADD_ROUTE(RequestMethod::POST, "/activate_account_balances")
    {
        co_return co_await APIHandlerActivateAccountBalances(request).handle();
    };

    // Update strategy's config
    ADD_ROUTE(RequestMethod::POST, "/strategy_config")
    {
        co_return co_await APIHandlerStrategyConfig(request).handle();
    };

    // Get strategy's config
    ADD_ROUTE(RequestMethod::GET, "/strategy_config")
    {
        co_return co_await APIHandlerStrategyConfig(request).handle();
    };

    // Update strategy's config
    ADD_ROUTE(RequestMethod::POST, "/strategy_current_info")
    {
        co_return co_await APIHandlerStrategyCurrentInfo(request).handle();
    };

    // Update strategy's config
    ADD_ROUTE(RequestMethod::POST, "/strategy_price_arbitrage_config")
    {
        co_return co_await APIHandlerStrategyPAConfig(request).handle();
    };

    // Get strategy's config
    ADD_ROUTE(RequestMethod::GET, "/strategy_price_arbitrage_config")
    {
        co_return co_await APIHandlerStrategyPAConfig(request).handle();
    };

    // Update strategy's config
    ADD_ROUTE(RequestMethod::GET, "/strategy_price_arbitrage_current_info")
    {
        co_return co_await APIHandlerStrategyPACurrentInfo(request).handle();
    };
}
#include <api_header.h>
#include <data_model/data_model.h>

// User
#include <api_handler/api_handler_user/api_handler_user_register.h>

// Account
#include <api_handler/api_handler_account/api_handler_add_account.h>
#include <api_handler/api_handler_account/api_handler_add_activate_account.h>

#include <gateways/gateway_manager.h>

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

    ADD_ROUTE(RequestMethod::GET, "/get_list_trade")
    {
        Json response;
        // response["trades"] = MongoDB::instance()
        //     .set_db_and_collection("test1", "trade")
        //     .find_many();

        Order order(
            "CVXUSDT",
            Order::Side::BUY,
            "MARKET",
            2.0,
            7.5
        );

        Json order_response = GatewayManager::instance()
            .get_gateway(GatewayEnum::BINANCE)
            ->place(order);

        return HttpResponse(OK_200, order_response);
    };

    ADD_ROUTE(RequestMethod::POST, "/external_request")
    {
        std::string body = request->get_body();
        Json data = Json::parse(body);

        std::string response = ExternalRequest("www.google.com", 80, "/", RequestMethod::GET).send_request();

        return HttpResponse(OK_200, response);
    };

    // Register new user
    ADD_ROUTE(RequestMethod::POST, "/register_new_user")
    {
        return APIHandlerUserRegister(request).handle();
    };

    // Add account
    ADD_ROUTE(RequestMethod::POST, "/add_account")
    {
        return APIHandlerAddAccount(request).handle();
    };

    // Add activate account
    ADD_ROUTE(RequestMethod::POST, "/add_activate_account")
    {
        return APIHandlerAddActivateAccount(request).handle();
    };

}
#include <gateways/binance/binance_gateway.h>
#include <app_utils/app_utils.h>
#include <account/account.h>

BinanceGateway::BinanceGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data_spot(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT)
    // m_market_data_perpetual(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT)
{
    Json account = Account::load_account_by_key(key);
    bool is_testnet = account["is_testnet"];

    // Update url + port for market data SPOT
    std::string md_spot_url  = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_URL : BINANCE_SPOT_WS_URL;
    std::string md_spot_port = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_PORT : BINANCE_SPOT_WS_PORT;
    m_market_data_spot.update_url_and_port(md_spot_url, md_spot_port);

    // // Update url + port for market data PERPETUAL
    // std::string md_perpetual_url  = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_URL  : BINANCE_FUTURES_WS_URL;
    // std::string md_perpetual_port = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_PORT : BINANCE_FUTURES_WS_PORT;
    // m_market_data_perpetual.update_url_and_port(md_perpetual_url, md_perpetual_port);
}

std::string BinanceGateway::get_name()
{
    return "binance";
}

void BinanceGateway::init()
{
    Gateway::init();

    m_symbols_info["spot"] = get_spot_symbols_info();
    m_symbols_info["perpetual"] = get_perpetual_symbols_info();

    // spdlog::debug("spot: {}", m_symbols_info["spot"]);
    // spdlog::debug("perpetual: {}", m_symbols_info["perpetual"]);
}

Task<Json> BinanceGateway::get_exchange_info()
{
    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), BINANCE_SPOT_URL, BINANCE_SPOT_PORT);
    std::string response = co_await client->get("/api/v3/exchangeInfo");

    co_return Json::parse(response);
}

Json BinanceGateway::get_spot_symbols_info()
{
    Json exchange_info = get_exchange_info()
        .start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::ORDER))
        .get();

    Json symbols_info;
    exchange_info["symbols"].for_each([&symbols_info, this](Json& data)
    {
        if ((std::string)data["status"] != "TRADING")
        {
            return;
        }

        std::string exchange_id = data["symbol"];
        std::string base_asset = data["baseAsset"];
        std::string quote_asset = data["quoteAsset"];
        std::string symbol_name = base_asset + "-" + quote_asset;

        if (m_instruments.find(symbol_name) == m_instruments.end())
        {
            m_instruments.insert(std::make_pair(symbol_name, SavableObject<Instrument>(INSTRUMENT_DB_NAME, m_gateway_name)));

            auto instrument = m_instruments.find(symbol_name)->second;
            instrument = Instrument {
                ExchangeEnum::BINANCE,
                InstrumentType::SPOT,
                symbol_name,
                exchange_id,
                get_rounded_number(data["filters"][1]["stepSize"]),
                std::stod((std::string&&)data["filters"][0]["tickSize"])
            };
        }

        symbols_info[symbol_name]["tickSize"] = std::stold((std::string&&)data["filters"][0]["tickSize"]);
        symbols_info[symbol_name]["lotSize"] = get_rounded_number(data["filters"][1]["stepSize"]);
        symbols_info[symbol_name]["roundUpPrice"] = get_rounded_number(data["filters"][0]["tickSize"]);
    });

    return symbols_info;
}

Json BinanceGateway::get_perpetual_symbols_info()
{
    // ExternalRequestSsl binance_request(BINANCE_FUTURES_URL, BINANCE_FUTURES_PORT, "/fapi/v1/exchangeInfo", RequestMethod::GET);

    // Json exchange_info = Json::parse(binance_request.send_request());
    // Json symbols_info;

    // exchange_info["symbols"].for_each([&symbols_info, this](Json& data)
    // {
    //     std::string symbol_name = data["symbol"];

    //     if (symbol_name == "ETHUSDT" || symbol_name == "BTCUSDT")
    //     {
    //         symbols_info[symbol_name]["tickSize"] = std::stold((std::string&&)data["filters"][0]["tickSize"]);
    //         symbols_info[symbol_name]["lotSize"] = get_rounded_number(data["filters"][1]["stepSize"]);
    //         symbols_info[symbol_name]["roundUpPrice"] = get_rounded_number(data["filters"][0]["tickSize"]);
    //     }
    // });

    // return symbols_info;

    return {};
}

size_t BinanceGateway::get_rounded_number(const std::string& lot_size)
{
    int pos_1 = lot_size.find_first_of("1"); // find the position of charater '1'
    return pos_1 == 0 ? 0 : pos_1 - 1;
}

std::string BinanceGateway::round_string_number(const std::string& str_number, size_t precision)
{
    int point_pos = str_number.find_first_of(".");
    if (point_pos > -1)
    {
        return str_number.substr(0, point_pos + (precision == 0 ? 0 : precision + 1));
    }

    return str_number;
}

void BinanceGateway::subscribe_symbol(std::vector<std::string> symbols)
{
    // Spot
    m_market_data_spot.subscribe_symbol(symbols, [this](const std::string& symbol, Json& payload)
    {
        this->on_depth_update(symbol, payload);
    });
    m_market_data_spot.start();

    // Perpetual
    // m_market_data_perpetual.subscribe_symbol(symbol, [this](const std::string& symbol, Json& payload)
    // {
    //     this->on_depth_update(symbol, payload);
    // });
    // m_market_data_perpetual.start();
}

void BinanceGateway::on_depth_update(const std::string& symbol, Json& payload)
{
    double best_bid = payload["bids"][0][0];
    double best_ask = payload["asks"][0][0];
    // ADD_LOG("On depth update - symbol: " << symbol << " - best_bid: " << best_bid << " - best_ask: " << best_ask);

    m_price_update_callback(symbol, best_ask);
}

Task<std::unordered_set<OrderId>> BinanceGateway::get_open_orders_on_exchange(std::string symbol)
{
    std::unordered_set<OrderId> res;

    // Currently, only implement for SPOT
    Json open_orders = co_await m_quoter_spot.get_open_orders(std::move(symbol));

    // Add order_id to [res]
    if (open_orders.is_array() == true)
    {
        open_orders.for_each([&res](Json& order)
        {
            if (order.has_field("clientOrderId"))
            {
                OrderId order_id = AppUtils::instance().parse_order_id(order["clientOrderId"]);

                if (order_id != 0)
                {
                    res.insert(order_id);
                }
            }
        });

    }

    co_return res;
}

TaskVoid BinanceGateway::cancel_all_on_exchange(std::string symbol)
{
    // Currently, only implement for SPOT
    co_await m_quoter_spot.cancel_all(std::move(symbol));

    co_return;
}

Task<Json> BinanceGateway::cancel_on_exchange(Order order)
{
    // Currently, only implement for SPOT
    co_return co_await m_quoter_spot.cancel(std::move(order));
}

Task<Json> BinanceGateway::place_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.exchange_type == InstrumentType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    Json response = co_await quoter->place(order);

    // Check if order is rejected
    if (response.has_field("code") && response["code"].is_object() == false && (long)response["code"] < 0)
    {
        order.status = Order::REJECTED;
        OrderManager::instance().update_order(order);
    }

    // co_return quoter->get_trade_result_from_response(response);

    co_return response;
}

Task<Json> BinanceGateway::get_balances()
{
    Json balances = co_await m_quoter_spot.get_balances();

    balances["balances"].for_each([](Json& balance)
    {
        balance["available"] = std::stod((std::string)balance["free"]) + std::stod((std::string)balance["locked"]);

        balance.remove_field("btcValuation");
        balance.remove_field("withdrawing");
        balance.remove_field("ipoable");
        balance.remove_field("locked");
        balance.remove_field("freeze");
        balance.remove_field("free");
    });

    co_return balances["balances"];
}

double BinanceGateway::round_up_quantity(const std::string& type, const std::string& symbol, double quantity)
{
    size_t lot_size = m_symbols_info[type][symbol]["lotSize"];
    std::string round_str_number = round_string_number(std::to_string(quantity), lot_size);

    return std::stod(round_str_number);
}

size_t BinanceGateway::get_lot_size(const std::string& type, const std::string& symbol)
{
    return m_symbols_info[type][symbol]["lotSize"];
}
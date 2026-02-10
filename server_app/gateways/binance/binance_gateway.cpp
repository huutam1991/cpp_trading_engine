#include <gateways/binance/binance_gateway.h>
#include <app_utils/app_utils.h>
#include <account/account.h>

BinanceGateway::BinanceGateway(const std::string& key) :
    m_quoter_spot(key),
    m_quoter_perpetual(key),
    m_market_data_spot(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT),
    m_market_data_perpetual(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT)
{
    Json account = Account::load_account_by_key(key);
    bool is_testnet = account["is_testnet"];

    // Update url + port for market data SPOT
    std::string md_spot_url  = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_URL : BINANCE_SPOT_WS_URL;
    std::string md_spot_port = is_testnet == true ? BINANCE_TESTNET_SPOT_WS_PORT : BINANCE_SPOT_WS_PORT;
    m_market_data_spot.update_url_and_port(md_spot_url, md_spot_port);

    // Update url + port for market data PERPETUAL
    std::string md_perpetual_url  = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_URL  : BINANCE_FUTURES_WS_URL;
    std::string md_perpetual_port = is_testnet == true ? BINANCE_TESTNET_FUTURES_WS_PORT : BINANCE_FUTURES_WS_PORT;
    m_market_data_perpetual.update_url_and_port(md_perpetual_url, md_perpetual_port);
}

ExchangeId BinanceGateway::get_exchange()
{
    return ExchangeId::BINANCE;
}

std::vector<Instrument> BinanceGateway::fetch_instruments()
{
    get_spot_symbols_info();
    get_perpetual_symbols_info();

    return m_instruments;
}

Task<Json> BinanceGateway::get_exchange_info()
{
    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), BINANCE_SPOT_URL, BINANCE_SPOT_PORT);
    std::string response = co_await client->get("/api/v3/exchangeInfo");

    co_return Json::parse(response);
}

Task<Json> BinanceGateway::get_exchange_info_perpetual()
{
    auto client = std::make_shared<HttpsClientAsync>(IOCPool::get_ioc_by_id(IOCId::ORDER_ENTRY), BINANCE_FUTURES_URL, BINANCE_FUTURES_PORT);
    std::string response = co_await client->get("/fapi/v1/exchangeInfo");

    co_return Json::parse(response);
}

void BinanceGateway::get_spot_symbols_info()
{
    Json exchange_info = get_exchange_info()
        .start_running_on(EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY))
        .get();

    exchange_info["symbols"].for_each([this](Json& data)
    {
        if (data["status"] != "TRADING")
        {
            return;
        }

        std::string exchange_symbol = data["symbol"];
        std::string base_asset = data["baseAsset"];
        std::string quote_asset = data["quoteAsset"];
        std::string symbol_name = base_asset + "-" + quote_asset;

        m_instruments.push_back(Instrument {
            ExchangeId::BINANCE,
            InstrumentType::SPOT,
            symbol_name,
            exchange_symbol,
            get_rounded_number(data["filters"][1]["stepSize"]),
            get_rounded_number(data["filters"][0]["tickSize"])
        });
    });
}

void BinanceGateway::get_perpetual_symbols_info()
{
    Json exchange_info = get_exchange_info_perpetual()
        .start_running_on(EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY))
        .get();

    exchange_info["symbols"].for_each([this](Json& data)
    {
        if (data["status"] != "TRADING")
        {
            return;
        }

        // Only process PERPETUAL contracts
        if (data["contractType"] != "PERPETUAL")
        {
            return;
        }

        std::string exchange_symbol = data["symbol"];
        std::string base_asset = data["baseAsset"];
        std::string quote_asset = data["quoteAsset"];
        std::string symbol_name = base_asset + "-" + quote_asset + "-PERPETUAL";

        m_instruments.push_back(Instrument {
            ExchangeId::BINANCE,
            InstrumentType::PERPETUAL,
            symbol_name,
            exchange_symbol,
            get_rounded_number(data["filters"][1]["stepSize"]),
            get_rounded_number(data["filters"][0]["tickSize"])
        });
    });
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

void BinanceGateway::subscribe_instruments(std::vector<const Instrument*> instruments)
{
    // Split instruments into SPOT and PERPETUAL
    std::vector<const Instrument*> spot_instruments;
    std::vector<const Instrument*> perpetual_instruments;
    for (const Instrument* instrument : instruments)
    {
        if (instrument->instrument_type == InstrumentType::SPOT)
        {
            spot_instruments.push_back(instrument);
        }
        else if (instrument->instrument_type == InstrumentType::PERPETUAL)
        {
            perpetual_instruments.push_back(instrument);
        }
    }

    // Spot
    m_market_data_spot.subscribe_instruments(spot_instruments);
    m_market_data_spot.start();

    // Perpetual
    m_market_data_perpetual.subscribe_instruments(perpetual_instruments);
    m_market_data_perpetual.start();
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

Task<void> BinanceGateway::cancel_all_on_exchange(std::string symbol)
{
    co_await m_quoter_spot.cancel_all(symbol);
    co_await m_quoter_perpetual.cancel_all(symbol);

    co_return;
}

Task<Json> BinanceGateway::cancel_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.instrument->instrument_type == InstrumentType::SPOT ?
        (BinanceQuoter*)&m_quoter_spot :
        (BinanceQuoter*)&m_quoter_perpetual;

    co_return co_await quoter->cancel(std::move(order));
}

Task<Json> BinanceGateway::place_on_exchange(Order order)
{
    // Get [m_quoter_spot] or [m_quoter_perpetual] base on ExchangeType of [order]
    BinanceQuoter* quoter = order.instrument->instrument_type == InstrumentType::SPOT ?
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

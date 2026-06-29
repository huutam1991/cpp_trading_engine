#include <network/https_client_request/https_client_request.h>

#include <gateways/binance/binance_gateway.h>
#include <gateways/binance/binance_order_entry.h>
#include <app_utils/app_utils.h>
#include <account/account_db.h>

BinanceGateway::BinanceGateway() :
    m_epoll_base((EpollBase*)EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY)),
    m_market_data_spot(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT),
    m_market_data_perpetual(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT)
{
    m_market_data_spot.update_url_and_port(BINANCE_SPOT_WS_URL, BINANCE_SPOT_WS_PORT);
    m_market_data_perpetual.update_url_and_port(BINANCE_FUTURES_WS_URL, BINANCE_FUTURES_WS_PORT);
}

ExchangeId BinanceGateway::get_exchange()
{
    return ExchangeId::BINANCE;
}

std::shared_ptr<OrderEntry> BinanceGateway::get_order_entry(std::shared_ptr<AccountBase> account)
{
    return std::make_shared<BinanceOrderEntry>("", m_epoll_base);
}

std::expected<bool, std::string> BinanceGateway::validate_account(std::shared_ptr<AccountBase> account)
{
    auto task_validate = account->m_order_entry->get_balances();
    auto future_validate = task_validate.get_future();
    task_validate.start_running_on(m_event_base);

    Json balances = future_validate.get();
    if (balances.has_field("code") && (int)balances["code"] < 0)
    {
        std::string error_msg = balances["msg"];
        error_msg += ", code: " + std::to_string((int)balances["code"]);

        return std::unexpected(error_msg);
    }

    return true;
}

std::vector<Instrument> BinanceGateway::fetch_instruments()
{
    get_spot_symbols_info();
    get_perpetual_symbols_info();

    return m_instruments;
}

Task<Json> BinanceGateway::get_exchange_info()
{
    HttpsClientRequest client(m_epoll_base, BINANCE_SPOT_URL, std::stoi(BINANCE_SPOT_PORT));
    HttpsClientResponse response = co_await client.get("/api/v3/exchangeInfo");

    co_return Json::parse(response.body);
}

Task<Json> BinanceGateway::get_exchange_info_perpetual()
{
    HttpsClientRequest client(m_epoll_base, BINANCE_FUTURES_REST_URL, std::stoi(BINANCE_FUTURES_REST_PORT));
    HttpsClientResponse response = co_await client.get("/fapi/v1/exchangeInfo");

    co_return Json::parse(response.body);
}

void BinanceGateway::get_spot_symbols_info()
{
    auto task = get_exchange_info();
    auto future = task.get_future();

    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY));
    Json exchange_info = future.get();

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
    auto task = get_exchange_info_perpetual();
    auto future = task.get_future();

    task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::EPOLL_GATEWAY));
    Json exchange_info = future.get();

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
    // m_market_data_perpetual.start();
}

void BinanceGateway::subscribe_instrument(const Instrument* instrument)
{
    if (instrument->instrument_type == InstrumentType::SPOT)
    {
        m_market_data_spot.subscribe_instrument(instrument);
    }
    else if (instrument->instrument_type == InstrumentType::PERPETUAL)
    {
        // m_market_data_perpetual.subscribe_instrument(instrument);
    }

    m_market_data_spot.start();
    m_market_data_perpetual.start(instrument);
}

void BinanceGateway::unsubscribe_instrument(const Instrument* instrument)
{
    if (instrument->instrument_type == InstrumentType::SPOT)
    {
        m_market_data_spot.unsubscribe_instrument(instrument);
    }
    else if (instrument->instrument_type == InstrumentType::PERPETUAL)
    {
        m_market_data_perpetual.unsubscribe_instrument(instrument);
    }
}

Json BinanceGateway::get_status()
{
    Json status;
    status["status"] = "Connected";
    status["environment"] = "Production";
    status["endpoints"] = {
        {"spot", "https://" + std::string(BINANCE_SPOT_URL)},
        {"perpetual", "https://" + std::string(BINANCE_FUTURES_REST_URL)}
    };
    status["exchange_id"] = enum_reflect::enum_name(ExchangeId::BINANCE);
    status["instruments"] = Instrument::get_instrument_list(ExchangeId::BINANCE, InstrumentType::PERPETUAL).size();
    status["latency"] = "18.4ms";
    status["up_time"] = "2d 14h 36m";
    status["accounts"] = m_accounts.size();
    status["messages_per_minute"] = 12532;

    return status;
}

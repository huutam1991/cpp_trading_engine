#include <exchanges/exchange_gateway.h>
#include <exchanges/binance_futures.h>
#include <exchanges/binance_spot.h>
#include <exchanges/binance_blvt.h>

void ExchangeGateWay::initialize()
{
    Exchange* binance_spot = new BinanceSpot();
    m_ExchangeList[BINANCE_SPOT] = binance_spot;

    Exchange* binance_blvt = new BinanceBLVT();
    m_ExchangeList[BINANCE_BLVT] = binance_blvt;

    Exchange* binance_futures = new BinanceFutures();
    m_ExchangeList[BINANCE_FUTURES] = binance_futures;

    binance_spot->initialize();
    binance_blvt->initialize();
    binance_futures->initialize();
}

Json ExchangeGateWay::get_exchange_info(Market market, const std::string& symbol)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->get_exchange_info(symbol);
    }
    return Json();
}

Json ExchangeGateWay::get_account_info(Market market)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->get_account_info();
    }
    return Json();
}

Json ExchangeGateWay::get_order(Market market, Json& query_json)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->get_order(query_json);
    }
    return Json();
}

Json ExchangeGateWay::get_open_order(Market market, const std::string& symbol)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->get_open_order(symbol);
    }
    return Json();
}

Json ExchangeGateWay::create_order(Market market, Json& query_json)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->create_order(query_json);
    }
    return Json();
}

Json ExchangeGateWay::replace_order(Market market, Json& query_json)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->replace_order(query_json);
    }
    return Json();
}

Json ExchangeGateWay::cancel_order(Market market, const std::string& symbol, const long orderId)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->cancel_order(symbol, orderId);
    }
    return Json();
}

void ExchangeGateWay::start_user_feed(Market market)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        rc->second->start_user_feed();
    }
}

size_t ExchangeGateWay::subscribe_user_feed(Market market, UserCallback callback)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->subscribe_user_feed(callback);
    }
    return 0;
}

void ExchangeGateWay::unsubscribe_user_feed(Market market, size_t callback_id)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        return rc->second->unsubscribe_user_feed(callback_id);
    }
}

size_t ExchangeGateWay::subscribe_data(Market market, MDSubscribeType type, const std::string& symbol, DataCallback callback)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        Exchange* ex = rc->second;
        if (type == DEPTH)
        {
            ex->start_depth_feed(symbol, callback);
        }
        else if (type == BOOK_TICKER)
        {
            ex->start_book_ticker_feed(symbol, callback);
        }
        else if (type == AGG_TRADE)
        {
            ex->start_agg_trade_feed(symbol, callback);
        }
        else if (type == MARKET_INFO)
        {
            ex->start_market_info_feed(symbol, callback);
        }
        else if (type == K_LINE)
        {
            ex->start_kline_feed(symbol, callback);
        }
    }

    return 1;
}

void ExchangeGateWay::unsubscribe_data(Market market, MDSubscribeType type, const std::string& symbol)
{
    auto rc = m_ExchangeList.find(market);
    if (rc != m_ExchangeList.end())
    {
        Exchange* ex = rc->second;
        ex->unsubscribe(type, symbol);
    }
}
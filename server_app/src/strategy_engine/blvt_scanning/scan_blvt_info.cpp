#include <strategy_engine/blvt_scanning/scan_blvt_info.h>

#include <exchanges/exchange_gateway.h>

#include <json/json.h>

#include <app_constants.h>
#include <utils.h>
#include <app_utils.h>

using namespace std;

void ScanBLVTInfo::subscribe_symbol(const string& symbol)
{
    if (!m_blvt_map.contains(symbol))
    {
        // init data
        m_blvt_map[symbol] = 1;
        m_baskets_map[symbol] = 0;
        m_token_issued_map[symbol] = 0;
        
        // subscribe blvt data
        ExchangeGateWay::instance().subscribe_data(
                    BINANCE_BLVT,
                    MARKET_INFO,
                    symbol,
                    [this](const std::string& symbol, Json& payload)
            {
                long double token_issued = (long double)payload["m"];
                long double baskets = (long double)payload["bn"];
                // check the changing of baskets & token_issued
                if (!IS_EQUAL(token_issued, m_token_issued_map[symbol]) ||
                    !IS_EQUAL(baskets, m_baskets_map[symbol]))
                {
                    ADD_LOG("baskets: " << baskets << " token_issued: " << token_issued);
                    ADD_LOG("m_baskets_map: " << m_baskets_map[symbol] << " m_token_issued_map: " << m_token_issued_map[symbol]);
                    
                    // not the first time running
                    if (!IS_EQUAL((long double)0.0, m_token_issued_map[symbol]) && 
                        !IS_EQUAL((long double)0.0, m_baskets_map[symbol]))
                    {
                        Json data;
                        long state = 0;

                        // change baskets
                        if (!IS_EQUAL(baskets, m_baskets_map[symbol]))
                            state = NOTIFICATION_STATE_BASKETS_CHANGE;
                        // change token_issued
                        else if (!IS_EQUAL(token_issued, m_token_issued_map[symbol]))
                            state = NOTIFICATION_STATE_TOKEN_ISSUED_CHANGE;
                        
                        data["message"] = "The Baskets or Token Issued of " + symbol + " is updated!";
                        data["code"] = state;
                        // send to client
                        WebSocketServerType::instance().send_data_through_channel(
                            CHANNEL_SCANNING_MARKET_NOTIFICATION,
                            "root",
                            data);
                    }

                    // update values
                    m_baskets_map[symbol] = baskets;
                    m_token_issued_map[symbol] = token_issued;
                }
            });
    }
    else
    {
        m_blvt_map[symbol] = m_blvt_map[symbol] + 1;
    }
}

void ScanBLVTInfo::unsubscribe_symbol(const string& symbol)
{
    if (m_blvt_map.contains(symbol))
    {
        if (m_blvt_map[symbol] == 0)
        {
            // unsubscribe blvt data
            ExchangeGateWay::instance().unsubscribe_data(BINANCE_BLVT, MARKET_INFO, symbol);
        }
        else
        {
            m_blvt_map[symbol] = m_blvt_map[symbol] - 1;
        }
    }
}

void ScanBLVTInfo::test_change_info(const string& symbol, bool is_baskets_changed)
{
    if (m_blvt_map.contains(symbol))
    {
        if (is_baskets_changed)
            m_baskets_map[symbol] = 1;
        else 
            m_token_issued_map[symbol] = 1;
    }
}

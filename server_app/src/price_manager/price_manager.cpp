#include <price_manager/price_manager.h>
#include <api_handler/api_handler_binance_spot/api_handler_binance.h>
// #include <api_handler/api_handler_binance_futures/api_handler_binance_futures.h> //MultiMarket

void PriceManager::init()
{
    APIHandlerBinance api_handler(nullptr);
    Json price_list = api_handler.send_binance_normal_request("/api/v3/ticker/price", "");

    // APIHandlerBinanceFutures api_handler(nullptr);
    // Json price_list = api_handler.send_binance_normal_request("/fapi/v1/ticker/price", ""); //MultiMarket

    price_list.for_each([this](Json& price)
    {
        std::string symbol = price["symbol"];
        m_price_list[symbol] = std::stold((std::string&&)price["price"]);
    });
}

void PriceManager::set_price(Json& price)
{
    std::string symbol = price["s"];
    m_price_list[symbol] = std::stold((std::string&&)price["c"]);
}

long double PriceManager::get_price_by_symbol(const std::string& symbol)
{
    if (m_price_list.has_field(symbol))
    {
        return (long double)m_price_list[symbol];
    }

    return -1.0;
}
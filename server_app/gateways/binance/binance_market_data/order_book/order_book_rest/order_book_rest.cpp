#include <gateways/binance/binance_market_data/order_book/order_book_rest/order_book_rest.h>

OrderBookRest::OrderBookRest(net::io_context& ioc)
    : m_ioc{ioc}
{}

Task<std::string> OrderBookRest::get_order_book(const std::string& symbol, size_t depth)
{
    std::string endpoint = "/fapi/v1/depth?symbol=" + symbol + "&limit=" + std::to_string(depth);
    auto http_client_async = std::make_shared<HttpsClientAsync>(m_ioc, "fapi.binance.com", "443");
    co_return co_await http_client_async->get(endpoint);
}
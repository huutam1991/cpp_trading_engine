#include <gateways/binance/binance_market_data/order_book/order_book_rest/order_book_rest.h>

#include <network/https_client_request/https_client_request.h>

OrderBookRest::OrderBookRest()
{}

Task<std::string> OrderBookRest::get_order_book(const std::string& symbol, size_t depth)
{
    std::string endpoint = "/fapi/v1/depth?symbol=" + symbol + "&limit=" + std::to_string(depth);
    HttpsClientRequest client(m_epoll_base, "fapi.binance.com", 443);

    co_return (co_await client.get(endpoint)).body;
}
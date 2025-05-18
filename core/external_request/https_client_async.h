#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <functional>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;

class HttpsClientAsync : public std::enable_shared_from_this<HttpsClientAsync>
{
public:
    using ResponseCallback = std::function<void(std::string)>;

    HttpsClientAsync(net::io_context& ioc, const std::string& host, const std::string& port);
    void fetch(const std::string& target, ResponseCallback cb);

private:
    tcp::resolver m_resolver;
    beast::ssl_stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::empty_body> m_request;
    http::response<http::string_body> m_res;
    std::string m_host;
    tcp::resolver::results_type m_resolve_result;
    std::string m_target;
    ResponseCallback m_callback;

    static ssl::context& get_ssl_ctx();

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void fail(const std::string& where, beast::error_code ec);
};
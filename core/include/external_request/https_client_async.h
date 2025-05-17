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

class HttpsClientAsync : public std::enable_shared_from_this<HttpsClientAsync> {
public:
    using ResponseCallback = std::function<void(std::string)>;

    HttpsClientAsync(net::io_context& ioc, ssl::context& ssl_ctx)
        : resolver_(ioc), stream_(ioc, ssl_ctx) {}

    void fetch(const std::string& host, const std::string& port, const std::string& target, ResponseCallback cb) {
        host_ = host;
        target_ = target;
        callback_ = std::move(cb);

        resolver_.async_resolve(host, port,
            beast::bind_front_handler(&HttpsClientAsync::on_resolve, shared_from_this()));
    }

private:
    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;
    std::string host_;
    std::string target_;
    ResponseCallback callback_;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) return fail("resolve", ec);
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&HttpsClientAsync::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if (ec) return fail("connect", ec);
        stream_.async_handshake(ssl::stream_base::client,
            beast::bind_front_handler(&HttpsClientAsync::on_handshake, shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
        if (ec) return fail("handshake", ec);

        req_.version(11);
        req_.method(http::verb::get);
        req_.target(target_);
        req_.set(http::field::host, host_);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::async_write(stream_, req_,
            beast::bind_front_handler(&HttpsClientAsync::on_write, shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) return fail("write", ec);

        http::async_read(stream_, buffer_, res_,
            beast::bind_front_handler(&HttpsClientAsync::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) return fail("read", ec);

        if (callback_) {
            callback_(res_.body());
        }

        beast::error_code shutdown_ec;
        stream_.shutdown(shutdown_ec);
    }

    void fail(const std::string& where, beast::error_code ec) {
        std::cerr << "HttpsClientAsync - Error in " << where << ": " << ec.message() << std::endl;
    }
};

// To use:
// auto client = std::make_shared<HttpsClientAsync>(ioc, ssl_ctx);
// client->fetch("api.binance.com", "443", "/api/v3/depth?symbol=BTCUSDT&limit=5", [](const std::string& res) {
//     std::cout << "Response: " << res << std::endl;
// });
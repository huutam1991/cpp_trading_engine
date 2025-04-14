#ifndef WEBSOCKET_CLIENT_ASYNC_H
#define WEBSOCKET_CLIENT_ASYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
using tcp = net::ip::tcp;

class WebsocketClientAsync : public std::enable_shared_from_this<WebsocketClientAsync> {
public:
    WebsocketClientAsync(net::io_context& ioc)
        : resolver_(ioc),
          ws_(ioc)
    {}

    void connect(const std::string& host, const std::string& port, const std::string& path = "/");
    void send(const std::string& msg);
    void close();
    void set_on_disconnect(std::function<void()> cb) { m_on_disconnect = std::move(cb); }

private:
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string path_;

    // Callbacks
    std::function<void(std::string)> m_on_message;
    std::function<void()> m_on_disconnect;
    std::function<void()> m_on_close;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::iterator);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);
    void fail(const std::string& where, beast::error_code ec);
};

#endif //WEBSOCKET_CLIENT_ASYNC_H
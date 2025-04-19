#ifndef WEBSOCKET_CLIENT_ASYNC_H
#define WEBSOCKET_CLIENT_ASYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <deque>
#include <thread>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
using tcp = net::ip::tcp;

class WebsocketClientAsync : public std::enable_shared_from_this<WebsocketClientAsync> {
public:
    WebsocketClientAsync();
    ~WebsocketClientAsync();

    void connect(const std::string& host, const std::string& port, const std::string& path = "/");
    void send(const std::string& msg);
    void close();
    void set_on_message(std::function<void(std::string)> cb) { m_on_message = std::move(cb); }
    void set_on_disconnect(std::function<void()> cb) { m_on_disconnect = std::move(cb); }

private:
    net::io_context& m_ioc;
    tcp::resolver m_resolver;
    boost::asio::ssl::context m_ssl_ctx;
    websocket::stream<beast::ssl_stream<tcp::socket>> m_ws;
    beast::flat_buffer m_buffer;
    std::string m_host;
    std::string m_path;

    // Callbacks
    std::function<void(std::string)> m_on_message;
    std::function<void()> m_on_disconnect;
    std::function<void()> m_on_close;

    // Write queue
    std::deque<std::string> m_write_queue;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::iterator);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);
    void do_write();
    void fail(const std::string& where, beast::error_code ec);

    // Static ioc context
    static net::io_context& get_ioc();
};

#endif //WEBSOCKET_CLIENT_ASYNC_H
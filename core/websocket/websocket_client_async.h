#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <deque>
#include <thread>
#include <functional>

#include <coroutine/event_base.h>
#include <coroutine/task.h>
#include <time/timer.h>

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
using tcp = net::ip::tcp;

class WebsocketClientAsync : public std::enable_shared_from_this<WebsocketClientAsync>
{
public:
    WebsocketClientAsync(net::io_context& io_context, EventBase* event_base, std::string name = "");
    ~WebsocketClientAsync();

    void set_callbacks(std::function<Task<void>()> on_connect, std::function<Task<void>(std::string)> on_message, std::function<Task<void>()> on_disconnect, std::function<Task<void>()> on_close);
    void connect(const std::string& host, const std::string& port, const std::string& path = "/");
    void send(const std::string& msg);
    void send_ping();
    void close();

    void add_keep_websocket_alive_task(std::function<Task<void>()> keep_alive_logic, size_t tick_in_milliseconds);

private:
    net::io_context& m_ioc;
    tcp::resolver m_resolver;
    boost::asio::ssl::context m_ssl_ctx;
    websocket::stream<beast::ssl_stream<tcp::socket>> m_ws;
    beast::flat_buffer m_buffer;

    EventBase* m_event_base = nullptr;
    std::string m_name;

    std::string m_host;
    std::string m_path;

    // Callbacks
    std::function<Task<void>()> m_on_connect = nullptr;
    std::function<Task<void>(std::string)> m_on_message = nullptr;
    std::function<Task<void>()> m_on_disconnect = nullptr;
    std::function<Task<void>()> m_on_close = nullptr;

    // Write queue
    std::deque<std::string> m_write_queue;

    // Intend close
    bool m_intend_close = false;

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::iterator);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_ping(beast::error_code ec);
    void on_close(beast::error_code ec);
    void do_write();
    void fail(const std::string& where, beast::error_code ec);
    void on_keep_websocket_alive(std::function<Task<void>()> keep_alive_logic, size_t tick_in_milliseconds);

    // Common method for invoking callbacks
    template<class T, class... Args>
    void invoke_callback(T& cb, Args&&... args);
};
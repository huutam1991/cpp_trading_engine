#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <memory>

#include <constants.h>
#include <util_macros.h>
#include <thread_pool.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class WebsocketClientHandle
{
public:
    WebsocketClientHandle(websocket::stream<beast::ssl_stream<tcp::socket>>& ws);
    WebsocketClientHandle(WebsocketClientHandle&) = delete;

private:
    websocket::stream<beast::ssl_stream<tcp::socket>>& m_ws;

public:
    void write(const std::string& buffer);
    void close();

};

class WebsocketClient
{
public:
    WebsocketClient(const std::string& host, const std::string& port, std::string path);
    WebsocketClient(WebsocketClient&) = delete;
    ~WebsocketClient();

    static ThreadPool& get_thread_pool()
    {
        static ThreadPool thread_pool(NUMBER_OF_WEBSOCKET_CLIENT_THREADS, "Websocket Client Pool");
        return thread_pool;
    }

public:
    std::shared_ptr<bool> m_is_shut_down;
    void set_use_valid_data(bool use_valid_data);

private:
    std::function<void(const std::string&, WebsocketClientHandle&)> m_on_message = nullptr;
    std::function<void(WebsocketClientHandle&)> m_on_connect = nullptr;
    std::function<void(websocket::close_code code)> m_on_close = nullptr;

protected:
    std::string m_host;
    std::string m_port;
    std::string m_path;
    bool        m_use_valid_data = false;

    std::mutex m_websocket_client_mutex;

public:
    void on_message(std::function<void(const std::string&, WebsocketClientHandle&)> func);
    void on_connect(std::function<void(WebsocketClientHandle&)> func);
    void on_close(std::function<void(websocket::close_code code)> func);
    void run();

};

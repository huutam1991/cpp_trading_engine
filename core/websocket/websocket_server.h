#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <functional>
#include <thread>

#include <utils/constants.h>
#include <utils/util_macros.h>
#include <thread_pool/thread_pool.h>
#include <json/json.h>
#include <jwt/jwt_manager.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

    std::mutex m_session_mutex;
    std::mutex m_session_invoke_close_mutex;

    std::function<void(const std::string&, session&)> m_on_message = nullptr;
    std::function<void(session&)> m_on_connect = nullptr;
    std::function<void(session&)> m_on_close = nullptr;

public:
    // Take ownership of the socket
    explicit session(tcp::socket&& socket)
        : ws_(std::move(socket))
    {
    }

    ~session()
    {
        ADD_LOG("Delete session = " << get_id());
    }

    // Get on the correct executor
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    void on_message(std::function<void(const std::string&, session&)> func)
    {
        m_on_message = func;
    }

    void on_connect(std::function<void(session&)> func)
    {
        m_on_connect = func;
    }

    void on_close(std::function<void(session&)> func)
    {
        m_on_close = func;
    }

    void write(const std::string& buffer)
    {
        std::unique_lock lock(m_session_mutex);

        try
        {
            // Echo the message
            ws_.text(ws_.got_text());
            ws_.write(net::buffer(buffer)); // use synchronous write - need to check later to use asynchronous

            // ws_.async_write(
            //     net::buffer(buffer),
            //     beast::bind_front_handler(
            //         &session::on_write,
            //         shared_from_this()));
        }
        catch (std::exception const& e)
        {
            // LOG(ERROR) << "Websocket Server write error: " << e.what() << std::endl;

            // Invoke callback
            check_invoke_close_callback();
        }
    }

    void close()
    {
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close_session,
                shared_from_this()));
    }

    size_t get_id()
    {
        return reinterpret_cast<size_t>(this);
    }

private:

    // Start the asynchronous operation
    void on_run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));

        // Set the 24h timeout options on the stream.
        ws_.set_option({
            std::chrono::seconds(30),   // handshake timeout
            std::chrono::hours(24),     // idle timeout
            false
        });

        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if(ec)
        {
            // Invoke callback
            check_invoke_close_callback();

            return fail(ec, "session accept");
        }

        // Invoke callback
        if (m_on_connect != nullptr)
        {
            m_on_connect(*this);
        }

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
        {
            // LOG(ERROR) << "Websocker session (" << this << ") close: " << ec.message() << "\n";

            // Invoke callback
            check_invoke_close_callback();

            return;
        }

        if(ec)
        {
            // LOG(ERROR) << "Websocker session (" << this << ") close: " << ec.message() << "\n";

            // Invoke callback
            check_invoke_close_callback();

            return fail(ec, "read");
        }

        // Invoke callback
        if (m_on_message != nullptr)
        {
            m_on_message(beast::buffers_to_string(buffer_.data()), *this);
        }

        // Clear the buffer
        buffer_.consume(buffer_.size());

        do_read();
    }

    void on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
        {
            // LOG(ERROR) << "Websocker session (" << this << ") close: " << ec.message() << "\n";

            // Invoke callback
            check_invoke_close_callback();

            return fail(ec, "write");
        }
    }

    void on_close_session(beast::error_code ec)
    {
        // Invoke callback
        check_invoke_close_callback();

        if(ec)
        {
            return fail(ec, "close");
        }
    }

    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        // LOG(ERROR) << what << ": " << ec.message() << "\n";
    }

    void check_invoke_close_callback()
    {
        std::unique_lock lock(m_session_invoke_close_mutex);

        try
        {
            if (m_on_close != nullptr)
            {
                m_on_close(*this);
                m_on_close = nullptr;
            }
        }
        catch(const std::exception& e)
        {
            // LOG(ERROR) << "Websocket Server check_invoke_close_callback error: " << e.what() << '\n';
        }
    }
};

//------------------------------------------------------------------------------

template<class SessionType, class ListenerType>
class WebsocketServer
{
    Singleton(WebsocketServer);

public:
    WebsocketServer(WebsocketServer&) = delete;

private:
    std::thread m_thread;
    std::mutex m_mutex;
    std::unordered_map<size_t, std::shared_ptr<SessionType>> m_session_list;
    std::unordered_map<std::string, std::vector<size_t>> m_channel_list;

    // template<class session>
    Json handle_method_subscribe(Json& subscribe, SessionType& session);
    // template<class session>
    Json handle_method_unsubscribe(Json& subscribe, SessionType& session);
    Json handle_method_unknown();
    bool check_channel_available(const std::string& channel_name);
    std::string verify_token(const std::string& token);
    std::string check_error(const std::string& channel_name, const std::string& token);
    bool is_common_channel(const std::string& channel_name);
    void subscribe_channel(std::string channel_name, const std::string& user_id, size_t session_id);
    void unsubscribe_channel(std::string channel_name, const std::string& user_id, size_t session_id);
    void unsubscribe_all_channel(size_t session_id);
    void close_session(size_t session_id);

protected:
    std::string m_host;
    std::string m_port;
    std::vector<std::string> m_available_channel_name;
    std::vector<std::string> m_available_common_channel_name;
    std::unordered_map<std::string, bool> m_minor_channel_list;

public:
    void set_info(const std::string& host, const std::string& port);
    void set_available_channel_name(const std::vector<std::string>& available_channel);
    void set_available_common_channel_name(const std::vector<std::string>& available_channel);
    void set_minor_channel_name(const std::vector<std::string>& minor_channel);
    void start();

    // template<class session>
    void on_new_session(std::shared_ptr<SessionType> session_ptr);
    void send_data_through_channel(std::string channel, const std::string& user_id, Json data);
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

    WebsocketServer<session,listener>& m_websocket_server;

public:
    listener(
        net::io_context& ioc,
        ssl::context& ctx,
        tcp::endpoint endpoint,
        WebsocketServer<session,listener>& websocket_server)
        : ioc_(ioc)
        , acceptor_(ioc)
        , m_websocket_server(websocket_server)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        // LOG(ERROR) << what << ": " << ec.message() << "\n";
    }

    // Start accepting incoming connections
    void run()
    {
        do_accept();
    }

private:
    void do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        ADD_LOG("Websocket Server on_accept");

        if(ec)
        {
            fail(ec, "listener accept");
        }
        else
        {
            // Create the session and run it
            std::shared_ptr<session> session_ptr = std::make_shared<session>(std::move(socket));
            ADD_LOG("Websocket Server - create session: " << session_ptr.get());
            m_websocket_server.on_new_session(session_ptr);
            ADD_LOG("Websocket Server - run session: " << session_ptr.get());
            session_ptr->run();
        }

        // Accept another connection
        do_accept();
    }
};


template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::set_info(const std::string& host, const std::string& port)
{
    m_host = host;
    m_port = port;
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::set_available_channel_name(const std::vector<std::string>& available_channel)
{
    m_available_channel_name = available_channel;

    for (auto& channel_name: m_available_channel_name)
    {
        m_minor_channel_list[channel_name] = false;
    }
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::set_available_common_channel_name(const std::vector<std::string>& available_channel)
{
    m_available_common_channel_name = available_channel;
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::set_minor_channel_name(const std::vector<std::string>& minor_channel)
{
    for (auto& channel_name: minor_channel)
    {
        m_minor_channel_list[channel_name] = true;
    }
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::start()
{
    m_thread = std::thread(
    [
        host = std::move(m_host),
        port = std::move(m_port),
        this
    ]()
    {
        auto const threads = NUMBER_OF_WEBSOCKET_SERVER_THREADS;
        // The io_context is required for all I/O
        net::io_context ioc{threads};

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12};
        // This holds the self-signed certificate used by the server
        ctx.use_certificate_chain_file(SSL_SERVER_CERTIFICATE);
        ctx.use_private_key_file(SSL_PRIVATE_KEY, ssl::context::pem);
        ctx.set_options(ssl::context::default_workarounds |
            ssl::context::sslv23_server |
            ssl::context::tlsv12 |
            ssl::context::no_sslv2);

        // Create and launch a listening port
        net::ip::address endpoint_host = net::ip::make_address(host);
        unsigned short endpoint_port = (unsigned short)stoi(port);
        std::make_shared<ListenerType>(ioc, ctx, tcp::endpoint{endpoint_host, endpoint_port}, *this)->run();

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for(auto i = threads - 1; i > 0; --i)
        {
            v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
        }
        ADD_LOG("Websocket Server is running on: " << host << ":" << port);
        ioc.run();
    });
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::on_new_session(std::shared_ptr<SessionType> session_ptr)
{
    // Set callback functions for the session
    session_ptr->on_connect([](SessionType& session)
    {
        ADD_LOG("Session connect = " << session.get_id());
    });

    session_ptr->on_message([this](const std::string& buffer, SessionType& session)
    {
        ADD_LOG("Session message = " << buffer);

        if (buffer == "close")
        {
            session.close();
        }

        // Check handle method
        Json subscribe = Json::parse(buffer);

        if (subscribe.has_field("method"))
        {
            Json response;
            std::string method = subscribe["method"];

            if (method == "subscribe")
            {
                response = handle_method_subscribe(subscribe, session);
            }
            else if (method == "unsubscribe")
            {
                response = handle_method_unsubscribe(subscribe, session);
            }
            else
            {
                response = handle_method_unknown();
            }

            session.write(response.get_string_value());
        }
    });

    session_ptr->on_close([this](SessionType& session)
    {
        ADD_LOG("Close session");
        close_session(session.get_id());
    });

    std::unique_lock lock(m_mutex);
    m_session_list.insert(std::make_pair(session_ptr->get_id(), session_ptr));
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::close_session(size_t session_id)
{
    // Unsubscribe all channels
    unsubscribe_all_channel(session_id);

    // Remove closed session from m_session_list
    std::unique_lock lock(m_mutex);
    auto it = m_session_list.find(session_id);
    if (it != m_session_list.end())
    {
        m_session_list.erase(it);
        ADD_LOG("WebsocketServer - session " << session_id << " remove from m_session_list");
    }
    else
    {
        ADD_LOG("WebsocketServer - session " << session_id << " cannot find in m_session_list");
    }

    ADD_LOG("WebsocketServer remaining sessions = " << m_session_list.size());
}

template<class SessionType, class ListenerType>
Json WebsocketServer<SessionType,ListenerType>::handle_method_subscribe(Json& subscribe, SessionType& session)
{
    Json response;
    std::string channel = subscribe["channel"];
    std::string token = subscribe["token"];

    std::string error_result = check_error(channel, token);
    if (error_result != "NO_ERROR")
    {
        response["msg"] = error_result;
        response["error"] = true;
        return response;
    }

    // Get user_id
    Json payload = JWTManager::instance().get_payload(token);
    std::string user_id = payload["user_id"];

    // Do subscribe
    subscribe_channel(channel, user_id, session.get_id());

    response["msg"] = "Subscribe to channel [" + channel + "] for user [" + user_id + "] successfully";
    response["error"] = false;

    return response;
}

template<class SessionType, class ListenerType>
Json WebsocketServer<SessionType,ListenerType>::handle_method_unsubscribe(Json& subscribe, SessionType& session)
{
    Json response;
    std::string channel = subscribe["channel"];
    std::string token = subscribe["token"];

    std::string error_result = check_error(channel, token);
    if (error_result != "NO_ERROR")
    {
        response["msg"] = error_result;
        response["error"] = true;
        return response;
    }

    // Get user_id
    Json payload = JWTManager::instance().get_payload(token);
    std::string user_id = payload["user_id"];

    // Do unsubscribe
    unsubscribe_channel(channel, user_id, session.get_id());

    response["msg"] = "Unsubscribe to channel [" + channel + "] for user [" + user_id + "] successfully";
    response["error"] = false;

    return response;
}

template<class SessionType, class ListenerType>
Json WebsocketServer<SessionType,ListenerType>::handle_method_unknown()
{
    Json response;
    response["msg"] = "Method is not available";
    response["error"] = true;

    return response;
}

template<class SessionType, class ListenerType>
std::string WebsocketServer<SessionType,ListenerType>::check_error(const std::string& channel_name, const std::string& token)
{
    // Check channel is available
    bool available = check_channel_available(channel_name);
    if (available == false) return "Channel is not available";

    // Verify token
    std::string verify_result = JWTManager::instance().verify_token(token);
    if (verify_result != VALID_TOKEN)
    {
        return verify_result;
    }

    return "NO_ERROR";
}

template<class SessionType, class ListenerType>
bool WebsocketServer<SessionType,ListenerType>::is_common_channel(const std::string& channel_name)
{
    // Check if channel_name is valid
    auto it = std::find(m_available_common_channel_name.begin(), m_available_common_channel_name.end(), channel_name);
    if (it == m_available_common_channel_name.end())
    {
        return false;
    }

    return true;
}

template<class SessionType, class ListenerType>
bool WebsocketServer<SessionType,ListenerType>::check_channel_available(const std::string& channel_name)
{
    std::unique_lock lock(m_mutex);

    // Check if channel_name is valid
    auto it = std::find(m_available_channel_name.begin(), m_available_channel_name.end(), channel_name);
    if (it != m_available_channel_name.end())
    {
        return true;
    }

    it = std::find(m_available_common_channel_name.begin(), m_available_common_channel_name.end(), channel_name);
    if (it != m_available_common_channel_name.end())
    {
        return true;
    }

    return false;
}

template<class SessionType, class ListenerType>
std::string WebsocketServer<SessionType,ListenerType>::verify_token(const std::string& token)
{
    std::string verify_result = JWTManager::instance().verify_token(token);
    if (verify_result != VALID_TOKEN)
    {
        return verify_result;
    }
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::subscribe_channel(std::string channel_name, const std::string& user_id, size_t session_id)
{
    std::unique_lock lock(m_mutex);

    // Get subscribed session ids list
    if (is_common_channel(channel_name) == false)
    {
        channel_name += "_" + user_id;
    }
    std::vector<size_t>& subscribed_session_id = m_channel_list[channel_name];
    auto it = std::find(subscribed_session_id.begin(), subscribed_session_id.end(), session_id);

    // Add session_id to subscribed list if it's not there
    if (it == subscribed_session_id.end())
    {
        subscribed_session_id.push_back(session_id);
    }

    ADD_LOG("Channel " << channel_name << " size: " << m_channel_list[channel_name].size());
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::unsubscribe_channel(std::string channel_name, const std::string& user_id, size_t session_id)
{
    std::unique_lock lock(m_mutex);

    // Get subscribed session ids list
    if (is_common_channel(channel_name) == false)
    {
        channel_name += "_" + user_id;
    }
    std::vector<size_t>& subscribed_session_id = m_channel_list[channel_name];
    auto it = std::find(subscribed_session_id.begin(), subscribed_session_id.end(), session_id);

    // Remove session_id to subscribed list if it's there
    if (it != subscribed_session_id.end())
    {
        subscribed_session_id.erase(it);
        ADD_LOG("WebsocketServer - session " << session_id << " unsubscribe from channel: " << channel_name);
    }

    ADD_LOG("Channel " << channel_name << " size: " << m_channel_list[channel_name].size());
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::unsubscribe_all_channel(size_t session_id)
{
    std::unique_lock lock(m_mutex);

    for (auto it = m_channel_list.begin(); it != m_channel_list.end(); it++)
    {
        std::vector<size_t>& subscribed_session_id = it->second;
        const std::string channel_name = it->first;
        auto find = std::find(subscribed_session_id.begin(), subscribed_session_id.end(), session_id);

        // Remove session_id to subscribed list if it's there
        if (find != subscribed_session_id.end())
        {
            subscribed_session_id.erase(find);
            ADD_LOG("WebsocketServer - session " << session_id << " unsubscribe from channel: " << channel_name);
        }

        ADD_LOG("Channel " << channel_name << " size: " << m_channel_list[channel_name].size());
    }
}

template<class SessionType, class ListenerType>
void WebsocketServer<SessionType,ListenerType>::send_data_through_channel(std::string channel, const std::string& user_id, Json data)
{
    // ADD_LOG("Websocket Server - start write to channel: " << channel);
    // ADD_LOG("Websocket Server - total session = " << m_session_list.size());

    bool is_minor_chanel = m_minor_channel_list[channel];

    // Json send_data = data.clone();
    // send_data["data"] = data;
    data["channel"] = channel;
    // send_data["id"] = user_id;

    // if (is_common_channel(channel) == false)
    // {
    //     channel += "_" + user_id;
    // }

    const std::vector<size_t>& subscribed_session_id = m_channel_list[channel];
    for (int i = 0; i < subscribed_session_id.size(); i++)
    {
        std::shared_ptr<SessionType>& session = m_session_list[subscribed_session_id[i]];

        // Check to avoid sending data to minor channel
        if (is_minor_chanel == true && session->is_writing()) continue;

        try
        {
            session->write(data.get_string_value());
        }
        catch(const std::exception& e)
        {
            // LOG(ERROR) << "Websocket Server - session write error: " << e.what() << std::endl;
            continue;
        }

        // ADD_LOG("Websocket Server - write to session id = " << subscribed_session_id[i]);
    }

    // ADD_LOG("Websocket Server - total write = " << subscribed_session_id.size());
}

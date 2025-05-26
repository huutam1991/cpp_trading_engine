#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <constants.h>
#include <util_macros.h>
#include <thread_pool/thread_pool.h>
#include <json/json.h>
#include <websocket/websocket_server.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Echoes back all received WebSocket messages
class session_ssl : public std::enable_shared_from_this<session_ssl>
{
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;

    std::mutex m_mutex;
    std::mutex m_session_invoke_close_mutex;

    bool m_is_close = false;
    bool m_is_writing = false;

    std::function<void(const std::string&, session_ssl&)> m_on_message = nullptr;
    std::function<void(session_ssl&)> m_on_connect = nullptr;
    std::function<void(session_ssl&)> m_on_close = nullptr;

public:
    // Take ownership of the socket
    session_ssl(tcp::socket&& socket, ssl::context& ctx)
        : ws_(std::move(socket), ctx)
    {
    }

    ~session_ssl()
    {
        ADD_LOG("Delete session_ssl = " << get_id());
    }

    bool is_writing()
    {
        return m_is_writing;
    }

    // Get on the correct executor
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session_ssl. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session_ssl::on_run,
                shared_from_this()));
    }

    void on_message(std::function<void(const std::string&, session_ssl&)> func)
    {
        m_on_message = func;
    }

    void on_connect(std::function<void(session_ssl&)> func)
    {
        m_on_connect = func;
    }

    void on_close(std::function<void(session_ssl&)> func)
    {
        m_on_close = func;
    }

    void write(const std::string& buffer)
    {
        std::unique_lock lock(m_mutex);

        if (m_is_close) return;

        m_is_writing = true;

        try
        {
            // Echo the message
            ws_.write(net::buffer(buffer)); // use synchronous write - need to check later to use asynchronous

            // ws_.async_write(
            //     net::buffer(buffer),
            //     beast::bind_front_handler(
            //         &session_ssl::on_write,
            //         shared_from_this()));
        }
        catch (std::exception const& e)
        {
            LOG(ERROR) << "Websocket Server SSL write error: " << e.what() << std::endl;

            // Invoke callback
            check_invoke_close_callback();

            m_is_close = true;

            lock.unlock();
        }
        m_is_writing = false;
    }

    void close()
    {
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session_ssl::on_close_session,
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
        // Set the timeout.
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

         // Perform the SSL handshake
        ws_.next_layer().async_handshake(
            ssl::stream_base::server,
            beast::bind_front_handler(
                &session_ssl::on_handshake,
                shared_from_this()));
    }

    void on_handshake(beast::error_code ec)
    {
        if(ec)
        {
            // Invoke callback
            check_invoke_close_callback();

            return fail(ec, "handshake");
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

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
                        " websocket-server-async-ssl");
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
                &session_ssl::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if(ec)
        {
            // Invoke callback
            check_invoke_close_callback();

            return fail(ec, "session_ssl accept");
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
                &session_ssl::on_read,
                shared_from_this()));
    }

    void on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session_ssl was closed
        if(ec == websocket::error::closed)
        {
            LOG(ERROR) << "Websocker session_ssl (" << this << ") close: " << ec.message() << "\n";

            // Invoke callback
            check_invoke_close_callback();

            return;
        }

        if(ec)
        {
            LOG(ERROR) << "Websocker session_ssl (" << this << ") close: " << ec.message() << "\n";

            // Invoke callback
            check_invoke_close_callback();

            ADD_LOG("After check_invoke_close_callback");

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
            LOG(ERROR) << "Websocker session_ssl (" << this << ") close: " << ec.message() << "\n";

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
        LOG(ERROR) << what << ": " << ec.message() << "\n";
        ADD_LOG("After Report a failure");
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
            LOG(ERROR) << "Websocket Server SSL check_invoke_close_callback error: " << e.what() << '\n';
        }
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener_ssl : public std::enable_shared_from_this<listener_ssl>
{
    net::io_context& ioc_;
    ssl::context& ctx_;
    tcp::acceptor acceptor_;

    WebsocketServer<session_ssl,listener_ssl>& m_websocket_server;

public:
    listener_ssl(
        net::io_context& ioc,
        ssl::context& ctx,
        tcp::endpoint endpoint,
        WebsocketServer<session_ssl,listener_ssl>& websocket_server)
        : ioc_(ioc)
        , ctx_(ctx)
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
        LOG(ERROR) << what << ": " << ec.message() << "\n";
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
                &listener_ssl::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        ADD_LOG("Websocket Server SSL on_accept");

        if(ec)
        {
            fail(ec, "listener_ssl accept");
        }
        else
        {
            // Create the session_ssl and run it
            std::shared_ptr<session_ssl> session_ptr = std::make_shared<session_ssl>(std::move(socket), ctx_);
            ADD_LOG("Websocket Server SSL - create session_ssl: " << session_ptr.get());
            m_websocket_server.on_new_session(session_ptr);
            ADD_LOG("Websocket Server SSL - run session_ssl: " << session_ptr.get());
            session_ptr->run();
        }

        // Accept another connection
        do_accept();
    }
};

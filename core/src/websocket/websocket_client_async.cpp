#include <websocket/websocket_client_async.h>
#include <util_macros.h>

WebsocketClientAsync::WebsocketClientAsync(EventBase* event_base) :
    m_ioc(WebsocketClientAsync::get_ioc()),
    m_resolver(m_ioc),
    m_ssl_ctx(boost::asio::ssl::context::tlsv12_client),
    m_ws(m_ioc, m_ssl_ctx),
    m_event_base(event_base)
{
    m_ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    m_ssl_ctx.set_default_verify_paths();
}

WebsocketClientAsync::~WebsocketClientAsync()
{
    close();
}

void WebsocketClientAsync::set_callbacks(std::function<TaskVoid()> on_connect, std::function<TaskVoid(std::string)> on_message, std::function<TaskVoid()> on_disconnect, std::function<TaskVoid()> on_close)
{
    m_on_connect = std::move(on_connect);
    m_on_message = std::move(on_message);
    m_on_disconnect = std::move(on_disconnect);
    m_on_close = std::move(on_close);
}

void WebsocketClientAsync::connect(const std::string& host, const std::string& port, const std::string& path)
{
    m_host = host;
    m_path = path;

    m_resolver.async_resolve(host, port,
        beast::bind_front_handler(&WebsocketClientAsync::on_resolve, shared_from_this()));
}

void WebsocketClientAsync::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) return fail("resolve", ec);

    if (SSL_set_tlsext_host_name(m_ws.next_layer().native_handle(), m_host.c_str()) == false)
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        return fail("SNI set", ec);
    }

    net::async_connect(m_ws.next_layer().next_layer(), results.begin(), results.end(),
        beast::bind_front_handler(&WebsocketClientAsync::on_connect, shared_from_this()));
}

void WebsocketClientAsync::on_connect(beast::error_code ec, tcp::resolver::iterator)
{
    if (ec) return fail("connect", ec);

    // SSL handshake before WebSocket handshake
    m_ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client,
        beast::bind_front_handler(&WebsocketClientAsync::on_ssl_handshake, shared_from_this()));
}

void WebsocketClientAsync::on_ssl_handshake(beast::error_code ec)
{
    if (ec) return fail("ssl_handshake", ec);

    // Then do WebSocket handshake
    m_ws.async_handshake(m_host, m_path,
        beast::bind_front_handler(&WebsocketClientAsync::on_handshake, shared_from_this()));
}

void WebsocketClientAsync::on_handshake(beast::error_code ec)
{
    if (ec) return fail("handshake", ec);

    // Start reading
    m_ws.async_read(m_buffer,
        beast::bind_front_handler(&WebsocketClientAsync::on_read, shared_from_this()));

    // Invoke [m_on_connect]
    invoke_callback(m_on_connect);
}

void WebsocketClientAsync::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        ADD_LOG("WebsocketClientAsync - on_read error: " << ec.message());

        if (
            ec == websocket::error::closed ||                     // WebSocket close
            ec == boost::asio::error::eof ||                      // TCP close
            ec == boost::asio::error::connection_reset ||         // Server reset
            ec == boost::asio::error::operation_aborted ||        // Socket aborted
            ec == boost::asio::error::broken_pipe ||              // Gửi khi socket đã đóng (Linux)
            ec == boost::asio::error::timed_out                   // Timeout
        )
        {
            // Invoke [m_on_disconnect]
            invoke_callback(m_on_disconnect);
        }

        return;
    }

    std::string data = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(m_buffer.size());

    // Separate base on new line
    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line))
    {
        if (!line.empty())
        {
            // Invoke [m_on_message]
            invoke_callback(m_on_message, std::move(line));
        }
    }

    // Continue reading
    m_ws.async_read(m_buffer,
        beast::bind_front_handler(&WebsocketClientAsync::on_read, shared_from_this()));
}

void WebsocketClientAsync::send(const std::string& msg)
{
    net::post(m_ws.get_executor(), [w = weak_from_this(), msg = msg]()
    {
        if (auto self = w.lock())
        {
            bool ready_to_write = self->m_write_queue.empty();
            self->m_write_queue.push_back(std::move(msg));

            if (ready_to_write)
            {
                self->do_write();
            }
        }
    });
}

void WebsocketClientAsync::do_write()
{
    m_ws.async_write(net::buffer(m_write_queue.front()),
        beast::bind_front_handler(&WebsocketClientAsync::on_write, shared_from_this()));
}

void WebsocketClientAsync::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("write", ec);

    m_write_queue.pop_front();

    if (!m_write_queue.empty())
    {
        do_write();
    }
}

void WebsocketClientAsync::close()
{
    m_ws.async_close(websocket::close_code::normal,
        beast::bind_front_handler(&WebsocketClientAsync::on_close, shared_from_this()));
}

void WebsocketClientAsync::on_close(beast::error_code ec)
{
    if (ec)
    {
        ADD_LOG("WebsocketClientAsync - Close error: " << ec.message());
    }

    // Invoke [m_on_close]
    invoke_callback(m_on_close);
}

void WebsocketClientAsync::fail(const std::string& where, beast::error_code ec)
{
    ADD_LOG("WebsocketClientAsync - Error in " << where << ": " << ec.message());
}

template<class T, class... Args>
void WebsocketClientAsync::invoke_callback(T& cb, Args&&... args)
{
    auto task = cb(std::forward<Args>(args)...);
    task.start_running_on(m_event_base);
}

net::io_context& WebsocketClientAsync::get_ioc()
{
    static net::io_context ioc;
    static auto guard = net::make_work_guard(ioc);
    static std::thread t([ioc = &ioc]()
    {
        ioc->run();
    });

    return ioc;
}
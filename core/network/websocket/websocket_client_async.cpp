#include <network/websocket/websocket_client_async.h>
#include <utils/util_macros.h>

WebsocketClientAsync::WebsocketClientAsync(net::io_context& io_context, EventBase* event_base, std::string name) :
    m_ioc(io_context),
    m_resolver(m_ioc),
    m_ssl_ctx(boost::asio::ssl::context::tlsv12_client),
    m_ws(m_ioc, m_ssl_ctx),
    m_event_base(event_base),
    m_name(std::move(name))
{
    m_ssl_ctx.set_verify_mode(boost::asio::ssl::verify_peer);
    m_ssl_ctx.set_default_verify_paths();
}

WebsocketClientAsync::~WebsocketClientAsync()
{
    spdlog::info("Close WebsocketClientAsync: [{}]", m_name);
}

void WebsocketClientAsync::set_callbacks(std::function<Task<void>()> on_connect, std::function<Task<void>(std::string)> on_message, std::function<Task<void>()> on_disconnect, std::function<Task<void>()> on_close)
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
        spdlog::info("WebsocketClientAsync: [{}] - on_read error: {}", m_name, ec.message());

        if (
            ec == websocket::error::closed ||                     // WebSocket close
            ec == boost::asio::error::eof ||                      // TCP close
            ec == boost::asio::error::connection_reset ||         // Server reset
            ec == boost::asio::error::operation_aborted ||        // Socket aborted
            ec == boost::asio::error::broken_pipe ||              // Send when socket is closed (Linux)
            ec == boost::asio::error::timed_out                   // Timeout
        )
        {
            if (m_intend_close == false)
            {
                // Invoke [m_on_disconnect]
                invoke_callback(m_on_disconnect);
            }
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

void WebsocketClientAsync::send_ping()
{
    m_ws.async_ping({}, beast::bind_front_handler(&WebsocketClientAsync::on_ping, shared_from_this()));
}

void WebsocketClientAsync::on_ping(beast::error_code ec)
{
    if (ec)
    {
        if (m_on_disconnect)
        {
            m_on_disconnect();
        }

        return fail("on_ping", ec);
    }

    spdlog::debug("WebsocketClientAsync: [{}] - Ping sent successfully, ec: {}", m_name, ec.message());
}

void WebsocketClientAsync::close()
{
    m_intend_close = true;
    m_ws.async_close(websocket::close_code::normal,
        beast::bind_front_handler(&WebsocketClientAsync::on_close, shared_from_this()));
}

void WebsocketClientAsync::on_close(beast::error_code ec)
{
    if (ec)
    {
        spdlog::info("WebsocketClientAsync: [{}] - Close error: {}", m_name, ec.message());
    }

    // Invoke [m_on_close]
    invoke_callback(m_on_close);
}

void WebsocketClientAsync::fail(const std::string& where, beast::error_code ec)
{
    spdlog::info("WebsocketClientAsync: [{}] - Error in {}: {}", m_name, where, ec.message());
}

void WebsocketClientAsync::add_keep_websocket_alive_task(std::function<Task<void>()> keep_alive_logic, size_t tick_in_milliseconds)
{
    Timer::add_schedule_task([name = m_name, weak_ptr = weak_from_this(), kal = std::move(keep_alive_logic), tick = tick_in_milliseconds]() mutable
    {
        if (auto self = weak_ptr.lock())
        {
            self->on_keep_websocket_alive(std::move(kal), tick);
        }
        else
        {
            spdlog::warn("WebsocketClientAsync: [{}] has been destroyed, cannot run keep alive logic", name);
        }
    }, tick_in_milliseconds);
}

void WebsocketClientAsync::on_keep_websocket_alive(std::function<Task<void>()> keep_alive_logic, size_t tick_in_milliseconds)
{
    if (keep_alive_logic != nullptr && m_event_base != nullptr)
    {
        keep_alive_logic().start_running_on(m_event_base);
        add_keep_websocket_alive_task(std::move(keep_alive_logic), tick_in_milliseconds);
    }
}

template<class T, class... Args>
void WebsocketClientAsync::invoke_callback(T& cb, Args&&... args)
{
    if (cb != nullptr)
    {
        auto task = cb(std::forward<Args>(args)...);
        task.start_running_on(m_event_base);
    }
}
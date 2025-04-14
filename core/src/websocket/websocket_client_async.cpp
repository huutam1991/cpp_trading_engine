#include <websocket/websocket_client_async.h>

void WebsocketClientAsync::connect(const std::string& host, const std::string& port, const std::string& path)
{
    host_ = host;
    path_ = path;

    resolver_.async_resolve(host, port,
        beast::bind_front_handler(&WebsocketClientAsync::on_resolve, shared_from_this()));
}

void WebsocketClientAsync::send(const std::string& msg)
{
    net::post(ws_.get_executor(), [w = weak_from_this(), msg = msg]()
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
    ws_.async_write(net::buffer(m_write_queue.front()),
        beast::bind_front_handler(&WebsocketClientAsync::on_write, shared_from_this()));
}

void WebsocketClientAsync::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) return fail("resolve", ec);

    net::async_connect(ws_.next_layer(), results.begin(), results.end(),
        beast::bind_front_handler(&WebsocketClientAsync::on_connect, shared_from_this()));
}

void WebsocketClientAsync::on_connect(beast::error_code ec, tcp::resolver::iterator)
{
    if (ec) return fail("connect", ec);

    ws_.async_handshake(host_, path_,
        beast::bind_front_handler(&WebsocketClientAsync::on_handshake, shared_from_this()));
}

void WebsocketClientAsync::on_handshake(beast::error_code ec)
{
    if (ec) return fail("handshake", ec);

    std::cout << "Connected!\n";

    // Start reading
    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebsocketClientAsync::on_read, shared_from_this()));
}

void WebsocketClientAsync::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
    {
        std::cerr << "on_read error: " << ec.message() << "\n";

        if (
            ec == websocket::error::closed ||                     // WebSocket close
            ec == boost::asio::error::eof ||                      // TCP close
            ec == boost::asio::error::connection_reset ||         // Server reset
            ec == boost::asio::error::operation_aborted ||        // Socket aborted
            ec == boost::asio::error::broken_pipe ||              // Gửi khi socket đã đóng (Linux)
            ec == boost::asio::error::timed_out                   // Timeout
        )
        {
            if (m_on_disconnect) m_on_disconnect();
        }

        return;
    }

    std::string data = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    // Separate base on new line
    std::stringstream ss(data);
    std::string line;
    while (std::getline(ss, line))
    {
        if (!line.empty())
        {
            if (m_on_message) m_on_message(std::move(line));
        }
    }

    // Continue reading
    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebsocketClientAsync::on_read, shared_from_this()));
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
    ws_.async_close(websocket::close_code::normal,
        beast::bind_front_handler(&WebsocketClientAsync::on_close, shared_from_this()));
}

void WebsocketClientAsync::on_close(beast::error_code ec)
{
    if (ec)
    {
        std::cerr << "Close error: " << ec.message() << "\n";
    }

    if (m_on_close) m_on_close();
}

void WebsocketClientAsync::fail(const std::string& where, beast::error_code ec)
{
    std::cerr << "Error in " << where << ": " << ec.message() << "\n";
}
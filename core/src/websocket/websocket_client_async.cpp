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
    ws_.async_write(net::buffer(msg),
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

    if (ec) return fail("read", ec);

    std::cout << "Received: " << beast::make_printable(buffer_.data()) << std::endl;
    buffer_.consume(buffer_.size());

    // Continue reading
    ws_.async_read(buffer_,
        beast::bind_front_handler(&WebsocketClientAsync::on_read, shared_from_this()));
}

void WebsocketClientAsync::on_write(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("write", ec);
}

void WebsocketClientAsync::fail(const std::string& where, beast::error_code ec)
{
    std::cerr << "Error in " << where << ": " << ec.message() << "\n";
}
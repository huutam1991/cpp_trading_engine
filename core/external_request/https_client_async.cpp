#include <external_request/https_client_async.h>

HttpsClientAsync::HttpsClientAsync(net::io_context& ioc, const std::string& host, const std::string& port)
        : resolver_(ioc), stream_{ioc, get_ssl_ctx()}, m_host{host}
{
    beast::error_code ec;
    m_resolve_result = resolver_.resolve(host, port, ec);
    if (ec) {
        throw std::runtime_error("Resolve failed: " + ec.message());
    }

    std::cout << "Resolve ec: " << ec << std::endl;
}

ssl::context& HttpsClientAsync::get_ssl_ctx()
{
    static ssl::context ssl_ctx(ssl::context::tlsv12_client);
    static bool initialized = [] 
    {
        ssl_ctx.set_verify_mode(ssl::verify_peer);
        ssl_ctx.set_default_verify_paths();
        return true;
    }();

    return ssl_ctx;
}

void HttpsClientAsync::fetch(const std::string& target, ResponseCallback cb) 
{
    target_ = target;
    callback_ = std::move(cb);

    beast::get_lowest_layer(stream_).async_connect(
        m_resolve_result,
        beast::bind_front_handler(&HttpsClientAsync::on_connect, shared_from_this()));
}

void HttpsClientAsync::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) 
{
    if (ec) return fail("connect", ec);
    stream_.async_handshake(ssl::stream_base::client,
        beast::bind_front_handler(&HttpsClientAsync::on_handshake, shared_from_this()));
}

void HttpsClientAsync::on_handshake(beast::error_code ec) 
{
    if (ec) return fail("handshake", ec);

    req_.version(11);
    req_.method(http::verb::get);
    req_.target(target_);
    req_.set(http::field::host, m_host);
    req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::async_write(stream_, req_,
        beast::bind_front_handler(&HttpsClientAsync::on_write, shared_from_this()));
}

void HttpsClientAsync::on_write(beast::error_code ec, std::size_t bytes_transferred) 
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("write", ec);

    http::async_read(stream_, buffer_, res_,
        beast::bind_front_handler(&HttpsClientAsync::on_read, shared_from_this()));
}

void HttpsClientAsync::on_read(beast::error_code ec, std::size_t bytes_transferred) 
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("read", ec);

    if (callback_) {
        callback_(res_.body());
    }

    beast::error_code shutdown_ec;
    stream_.shutdown(shutdown_ec);
}

void HttpsClientAsync::fail(const std::string& where, beast::error_code ec) 
{
    std::cerr << "HttpsClientAsync - Error in " << where << ": " << ec.message() << std::endl;
}
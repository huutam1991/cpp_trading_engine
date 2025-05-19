#include <external_request/https_client_async.h>

HttpsClientAsync::HttpsClientAsync(net::io_context& ioc, const std::string& host, size_t port)
    : m_resolver(ioc), m_stream{ioc, get_ssl_ctx()}, m_host{host}
{
    beast::error_code ec;
    m_resolve_result = m_resolver.resolve(host, std::to_string(port), ec);
    if (ec) 
    {
        throw std::runtime_error("Resolve failed: " + ec.message());
    }
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

void HttpsClientAsync::add_header(const std::string& key, const std::string value)
{
    m_headers.insert(std::make_pair(key, value));
}

void HttpsClientAsync::get(const std::string& endpoint, ResponseCallback cb) 
{
    m_method = http::verb::get;
    m_endpoint = endpoint;
    m_body = "";
    m_callback = std::move(cb);

    beast::get_lowest_layer(m_stream).async_connect(
        m_resolve_result,
        beast::bind_front_handler(&HttpsClientAsync::on_connect, shared_from_this()));
}

void HttpsClientAsync::post(const std::string& endpoint, std::string body, ResponseCallback cb)
{
    m_method = http::verb::post;
    m_endpoint = endpoint;
    m_body = std::move(body);
    m_callback = std::move(cb);

    beast::get_lowest_layer(m_stream).async_connect(
        m_resolve_result,
        beast::bind_front_handler(&HttpsClientAsync::on_connect, shared_from_this()));
}

void HttpsClientAsync::del(const std::string& endpoint, std::string body, ResponseCallback cb)
{
    m_method = http::verb::delete_;
    m_endpoint = endpoint;
    m_body = std::move(body);
    m_callback = std::move(cb);

    beast::get_lowest_layer(m_stream).async_connect(
        m_resolve_result,
        beast::bind_front_handler(&HttpsClientAsync::on_connect, shared_from_this()));
}

void HttpsClientAsync::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) 
{
    if (ec) return fail("connect", ec);
    m_stream.async_handshake(ssl::stream_base::client,
        beast::bind_front_handler(&HttpsClientAsync::on_handshake, shared_from_this()));
}

void HttpsClientAsync::on_handshake(beast::error_code ec) 
{
    if (ec) return fail("handshake", ec);

    m_request.version(11);
    m_request.method(m_method);
    m_request.target(m_endpoint);
    m_request.body() = m_body;
    m_request.set(http::field::host, m_host);
    m_request.set(http::field::content_type, "application/json");
    m_request.set(http::field::content_length, std::to_string(m_body.size()));
    m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Custom header
    for (auto it = m_headers.begin(); it != m_headers.end(); it++)
    {
        m_request.set(it->first, it->second);
    }

    http::async_write(m_stream, m_request,
        beast::bind_front_handler(&HttpsClientAsync::on_write, shared_from_this()));
}

void HttpsClientAsync::on_write(beast::error_code ec, std::size_t bytes_transferred) 
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("write", ec);

    http::async_read(m_stream, m_buffer, m_res,
        beast::bind_front_handler(&HttpsClientAsync::on_read, shared_from_this()));
}

void HttpsClientAsync::on_read(beast::error_code ec, std::size_t bytes_transferred) 
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("read", ec);

    if (m_callback) 
    {
        m_callback(m_res.body());
    }

    beast::error_code shutdown_ec;
    m_stream.shutdown(shutdown_ec);
}

void HttpsClientAsync::fail(const std::string& where, beast::error_code ec) 
{
    std::cerr << "HttpsClientAsync - Error in " << where << ": " << ec.message() << std::endl;
}
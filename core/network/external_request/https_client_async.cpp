#include <network/external_request/https_client_async.h>
#include <unordered_map>
#include <mutex>

HttpsClientAsync::HttpsClientAsync(net::io_context& ioc, const std::string& host, const std::string& port)
    :   m_resolver(ioc),
        m_resolve_result{get_resolve_result_cache(m_resolver, host, port)},
        m_stream{ioc, get_ssl_ctx()},
        m_host{host}
{
}

Future<std::string> HttpsClientAsync::get(const std::string& endpoint)
{
    return send_request(http::verb::get, endpoint, "");
}

Future<std::string> HttpsClientAsync::post(const std::string& endpoint, std::string body)
{
    return send_request(http::verb::post, endpoint, std::move(body));
}

Future<std::string> HttpsClientAsync::del(const std::string& endpoint, std::string body)
{
    return send_request(http::verb::delete_, endpoint, std::move(body));
}

Future<std::string> HttpsClientAsync::put(const std::string& endpoint, std::string body)
{
    return send_request(http::verb::put, endpoint, std::move(body));
}

Future<std::string> HttpsClientAsync::send_request(http::verb method, const std::string& endpoint, std::string body)
{
    m_method = method;
    m_endpoint = endpoint;
    m_body = std::move(body);

    return Future<std::string>([self = shared_from_this()](Future<std::string>::FutureValue* value) mutable
    {
        self->m_future_value = value;

        beast::get_lowest_layer(self->m_stream).async_connect(
            self->m_resolve_result,
            beast::bind_front_handler(&HttpsClientAsync::on_connect, self));
    });
}

tcp::resolver::results_type& HttpsClientAsync::get_resolve_result_cache(tcp::resolver& resolver, const std::string& host, const std::string& port)
{
    static std::unordered_map<std::string, tcp::resolver::results_type> resolve_results_map;

    // Create pair of [host/port] = host + port
    auto key = host + port;

    if (resolve_results_map.find(key) == resolve_results_map.end())
    {
        // Protect [resolve_results_map] from race condition
        static std::mutex resolve_mutex;
        std::lock_guard<std::mutex> lock(resolve_mutex);

        beast::error_code ec;
        auto resolve_result = resolver.resolve(host, port, ec);
        if (ec)
        {
            throw std::runtime_error("Resolve failed: " + ec.message());
        }

        resolve_results_map.insert(std::make_pair(key, resolve_result));
    }

    return resolve_results_map[key];
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

    m_parser.get().clear();
    m_parser.body_limit(50 * 1024 * 1024); // Reset body limit to 10MB

    http::async_read(m_stream, m_buffer, m_parser,
        beast::bind_front_handler(&HttpsClientAsync::on_read, shared_from_this()));
}

void HttpsClientAsync::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail("read", ec);

    m_res = m_parser.get();
    m_future_value->set_value(m_res.body());

    beast::error_code shutdown_ec;
    m_stream.shutdown(shutdown_ec);
}

void HttpsClientAsync::fail(const std::string& where, beast::error_code ec)
{
    std::cerr << "HttpsClientAsync - Error in " << where << ": " << ec.message() << std::endl;
    spdlog::error("HttpsClientAsync - data: {}", beast::buffers_to_string(m_buffer.data()));
}
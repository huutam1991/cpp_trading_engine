

#include <external_request/external_request_ssl.h>

#include <utils/util_macros.h>

ExternalRequestSsl::ExternalRequestSsl(const std::string& url, const std::string& port, const std::string& path, RequestMethod request_method):
    m_url(url), m_port(port), m_path(path), m_request_method(request_method)
{
}

void ExternalRequestSsl::add_header(const std::string& key, const std::string value)
{
    m_headers.insert(std::make_pair(key, value));
}

void ExternalRequestSsl::add_body(Json& body)
{
    m_body = body;
}

http::verb ExternalRequestSsl::transform_request_method(RequestMethod method)
{
    http::verb res;

    switch (method)
    {
    case RequestMethod::GET:
        res = http::verb::get;
        break;
    case RequestMethod::POST:
        res = http::verb::post;
        break;
    case RequestMethod::DELETE:
        res = http::verb::delete_;
        break;
    case RequestMethod::PUT:
        res = http::verb::put;
        break;
    case RequestMethod::OPTIONS:
        res = http::verb::options;
        break;
    case RequestMethod::PATCH:
        res = http::verb::patch;
        break;
    case RequestMethod::HEAD:
        res = http::verb::head;
        break;

    default:
        res = http::verb::get;
        break;
    }

    return res;
}

net::io_context ExternalRequestSsl::ioc;
ssl::context ExternalRequestSsl::ctx(ssl::context::tlsv12_client);

void ExternalRequestSsl::init_io_context()
{
    // // This holds the root certificate used for verification
    // load_root_certificates(ctx);

    ExternalRequestSsl::ctx.set_default_verify_paths();

    // Verify the remote server's certificate
    ExternalRequestSsl::ctx.set_verify_mode(ssl::verify_peer);
}

std::string ExternalRequestSsl::send_request()
{
    try
    {
        static bool is_init_io_context = false;
        if (is_init_io_context == false)
        {
            ExternalRequestSsl::init_io_context();
            is_init_io_context = true;
        }

        // These objects perform our I/O
        tcp::resolver resolver(ExternalRequestSsl::ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ExternalRequestSsl::ioc, ExternalRequestSsl::ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), m_url.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(m_url, m_port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Set disable verifying for "localhost" or "127.0.0.1"
        if (m_url == "localhost" || m_url == "127.0.0.1" || m_url == "binance_simulator")
        {
            stream.set_verify_callback([](bool preverified, boost::asio::ssl::verify_context& ctx)
            {
                return true;
            });
        }

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Version
        int version = 10;

        // Set up an HTTP GET request message
        std::string body = m_body.is_null() ? "" : m_body.get_string_value();
        http::request<http::string_body> req{transform_request_method(m_request_method), m_path, version};
        req.body() = body;
        req.set(http::field::host, m_url);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::content_length, std::to_string(body.size()));
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Custom header
        for (auto it = m_headers.begin(); it != m_headers.end(); it++)
        {
            req.set(it->first, it->second);
        }

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Receive the HTTP response
        struct { size_t hbytes = 0, bbytes = 0; } ret;

        std::string response_str;
        http::parser<false, http::buffer_body> p;
        // p.header_limit(8192);
        // p.body_limit(1024);
        boost::beast::error_code ec;
        char buf[2048];
        size_t size_buf = sizeof(buf) - 1;
        p.get().body().data = buf;
        p.get().body().size = size_buf;

        // Read header
        ret.hbytes = read_header(stream, buffer, p, ec);
        if(ec)
        {
            LOG(INFO) << ec.message() << std::endl;
            // return ec.message();
        }

        // Read body
        while(! p.is_done())
        {
            p.get().body().data = buf;
            p.get().body().size = size_buf;
            ret.bbytes += http::read_some(stream, buffer, p, ec);
            if(ec == http::error::need_buffer)
                ec = {};
            if(ec)
                break;

            buf[size_buf - p.get().body().size] = '\0';
            response_str += std::string(buf);
        }

        buffer.consume(buffer.size());
        resolver.cancel();
        req.clear();
        p.release();

        // Gracefully close the stream
        // beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        // if(ec)
        //     throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully

        return response_str;
    }
    catch(std::exception const& e)
    {
        LOG(ERROR) << "Error: " << e.what() << std::endl;
        return "EXIT_FAILURE";
    }

    return "";
}
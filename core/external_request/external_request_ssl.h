#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include <utils/constants.h>

class ExternalRequestSsl
{
public:
    ExternalRequestSsl(const std::string& url, const std::string& port, const std::string& path, RequestMethod request_method);
    ExternalRequestSsl(ExternalRequestSsl&) = delete;

protected:
    std::string m_url;
    std::string m_path;
    std::string m_port;
    RequestMethod m_request_method;

private:
    static void init_io_context();
    static net::io_context ioc;
    static ssl::context ctx;

    std::unordered_map<std::string, std::string> m_headers;
    Json m_body = JsonNull();

public:
    void add_header(const std::string& key, const std::string value);
    void add_body(Json& body);
    std::string send_request();
    http::verb transform_request_method(RequestMethod method);

};
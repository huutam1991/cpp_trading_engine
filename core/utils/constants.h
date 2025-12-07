#pragma once

#include <unordered_map>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <network/https_server/exception.h>

#define BACKLOG_SOCKET 125                     // number of connections
#define BACKLOG_EPOLL 125                      // number of epoll events
#define BUFFER_SIZE 2048                      // size of buffer for reading request data

#define MONGO_URI "mongodb://127.0.0.1:27017"
#define MONGO_URI_PROD "mongodb://172.31.9.78:27017"
#define DB_APP_CONFIG "app_config"

#define SSL_SERVER_CERTIFICATE "server-certificate.crt"
#define SSL_PRIVATE_KEY "server-private-key.pem"

const std::string LINE_ENDING = "\r\n";
const std::string HTTP_VERSION = "HTTP/1.1";
const std::string NOT_FOUND_ERROR_MESSAGE = "The given document couldn't be found";
const std::string PARAM_NOT_FOUND = "PARAM_NOT_FOUND";
const std::string PARAM_NO_MISSING = "PARAM_NO_MISSING";

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct epoll_event epoll_event;

enum class RequestMethod
{
    GET,
    POST,
    OPTIONS,
    HEAD,
    PUT,
    DELETE,
    PATCH,
    UNKNOWN,
};

namespace RequestContentType
{
    const std::string ApplicationJson = "application/json";
    const std::string ApplicationJavascript = "application/javascript";
    const std::string ApplicationXml = "application/xml";
    const std::string Text = "text/plain;charset=UTF-8";
    const std::string TextPlain = "text/plain";
    const std::string TextXml = "text/xml";
    const std::string TextHtml = "text/html";
};

enum class RouteType
{
    GROUP,
    NORMAL
};

const std::unordered_map<std::string, RequestMethod> request_method_map =
{
    {"OPTIONS", RequestMethod::OPTIONS },
    {"GET",     RequestMethod::GET     },
    {"HEAD",    RequestMethod::HEAD    },
    {"POST",    RequestMethod::POST    },
    {"PUT",     RequestMethod::PUT     },
    {"DELETE",  RequestMethod::DELETE  },
    {"PATCH",   RequestMethod::PATCH   },
};

const std::unordered_map<size_t, std::string> request_method_map_string =
{
    {(size_t)RequestMethod::OPTIONS, "OPTIONS"},
    {(size_t)RequestMethod::GET    , "GET"    },
    {(size_t)RequestMethod::HEAD   , "HEAD"   },
    {(size_t)RequestMethod::POST   , "POST"   },
    {(size_t)RequestMethod::PUT    , "PUT"    },
    {(size_t)RequestMethod::DELETE , "DELETE" },
    {(size_t)RequestMethod::PATCH  , "PATCH"  },
};

enum ResponseStatusCode
{
    OK_200                      = 200,
    CREATED_201                 = 201,
    BAD_REQUEST_400             = 400,
    UNAUTHORIZED_REQUEST_401    = 401,
    NOT_FOUND_404               = 404,
    RISK_ERROR_410              = 410,  // Internal only
    INTERNAL_SERVER_ERROR_500   = 500,
};

const std::unordered_map<int, std::string> response_status_code_map =
{
    {OK_200,                    "OK"},
    {CREATED_201,               "Created"},
    {BAD_REQUEST_400,           "Bad Request"},
    {UNAUTHORIZED_REQUEST_401,  "Unauthorized Request"},
    {NOT_FOUND_404,             "Not Found"},
    {INTERNAL_SERVER_ERROR_500, "Internal Server Error"},
};


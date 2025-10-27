#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>

#include <https_server/http_server.h>

enum SSL_ACCEPT_STATUS
{
    ERROR = -1,
    OK = 0,
    WANT_IO = 1
};

class HttpsServer : public HttpServer
{
private:
    SSL_CTX* m_ctx;
    std::unordered_map<int, SSL*> m_ssl_by_socket_id;
    std::unordered_map<int, bool> m_ssl_accept_success;

    SSL_CTX *create_context();
    void configure_context(SSL_CTX *ctx);
    bool check_valid_socket_id(int socket_id);
    SSL_ACCEPT_STATUS do_ssl_accept(int client_fd);

public:
    HttpsServer(int port, std::string dir_path, EventBase* event_base);
    ~HttpsServer();

    virtual int accept_new_connection();
    virtual int read_buffer(int client_fd, char* const buffer);
    virtual void handle_client_request(int client_fd);
    virtual void close_connection(int client_fd);
    virtual void write_to_socket_io(int client_fd, const char* buffer, std::uint32_t size);
};


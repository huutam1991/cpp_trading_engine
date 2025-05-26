#include <cstring>
#include <openssl/bio.h>

#include <https_server/https_server.h>
#include <utils/util_macros.h>

HttpsServer::HttpsServer(int port, std::string dir_path, EventBase* event_base) : HttpServer(port, dir_path, event_base)
{
    m_ctx = create_context();
    configure_context(m_ctx);
}

HttpsServer::~HttpsServer()
{
}

SSL_CTX* HttpsServer::create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void HttpsServer::configure_context(SSL_CTX *ctx)
{
    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, SSL_SERVER_CERTIFICATE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, SSL_PRIVATE_KEY, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int HttpsServer::accept_new_connection()
{
    int client_fd = HttpServer::accept_new_connection();

    if (client_fd < 0)
    {
        return client_fd;
    }

    SSL* ssl = SSL_new(m_ctx);
    SSL_set_fd(ssl, client_fd);

    m_ssl_by_socket_id.insert(std::make_pair(client_fd, ssl));
    m_ssl_accept_success[client_fd] = false;

    // Do ssl_accept
    SSL_ACCEPT_STATUS accept_status = do_ssl_accept(client_fd);
    if (accept_status == SSL_ACCEPT_STATUS::ERROR)
    {
        return -1;
    }

    return client_fd;
}

SSL_ACCEPT_STATUS HttpsServer::do_ssl_accept(int client_fd)
{
    SSL_ACCEPT_STATUS accept_status;
    SSL* ssl = m_ssl_by_socket_id[client_fd];

    try
    {
        int accept = SSL_accept(ssl);

        switch (SSL_get_error(ssl, accept))
        {
            case SSL_ERROR_NONE:
                accept_status = SSL_ACCEPT_STATUS::OK;
                break;
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
                accept_status = SSL_ACCEPT_STATUS::WANT_IO;
                break;
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_SYSCALL:
            default:
                accept_status = SSL_ACCEPT_STATUS::ERROR;
                break;
        }
    }
    catch(const std::exception& e)
    {
        // LOG(ERROR) << "HttpsServer - SSL_accept error = " << e.what() << '\n';

        // Close socket fd
        // LOG(INFO) << "error https - accept_new_connection: " << std::strerror(errno) << std::endl;
        // LOG(INFO) << "close socket id: " << client_fd << std::endl;
        close_connection(client_fd);
        return SSL_ACCEPT_STATUS::ERROR;
    }

    if (accept_status == SSL_ACCEPT_STATUS::ERROR)
    {
        // LOG(INFO) << "error https - accept_new_connection: " << std::strerror(errno) << std::endl;
        // LOG(INFO) << "close socket id: " << client_fd << std::endl;
        close_connection(client_fd);
        return SSL_ACCEPT_STATUS::ERROR;
    }

    if (accept_status == SSL_ACCEPT_STATUS::OK)
    {
        m_ssl_accept_success[client_fd] = true;

        // Set non-blocking BIO
        BIO *ssl_bio = BIO_new_socket(client_fd, BIO_NOCLOSE);
        BIO_set_write_buf_size(ssl_bio, 15 * 1024 * 1024);
        SSL_set_bio(ssl, ssl_bio, ssl_bio);
        BIO_set_nbio( ssl_bio, 1 );
    }

    return accept_status;
}

void HttpsServer::handle_client_request(int client_fd)
{
    // Continue with ssl_accept if it's not finish yet
    if (m_ssl_accept_success[client_fd] == false)
    {
        do_ssl_accept(client_fd);
    }
    // If ssl_accept is finished, handle client request
    else
    {
        HttpServer::handle_client_request(client_fd);
    }
}

bool HttpsServer::check_valid_socket_id(int socket_id)
{
    return m_ssl_by_socket_id.find(socket_id) != m_ssl_by_socket_id.end();
}

int HttpsServer::read_buffer(int client_fd, char* const buffer)
{
    if (check_valid_socket_id(client_fd))
    {
        return SSL_read(m_ssl_by_socket_id[client_fd], buffer, BUFFER_SIZE);
    }
    return 0;
}

void HttpsServer::write_to_socket_io(int client_fd, const char* buffer, std::uint32_t size)
{
    std::unique_lock lock(m_server_mutex);

    if (check_valid_socket_id(client_fd))
    {
        SSL_write(m_ssl_by_socket_id[client_fd], buffer, size);
    }
}

void HttpsServer::close_connection(int client_fd)
{
    std::unique_lock lock(m_server_mutex);

    // Remove old ssl from m_ssl_by_socket_id
    if (check_valid_socket_id(client_fd))
    {
        SSL* old_ssl = m_ssl_by_socket_id[client_fd];
        SSL_shutdown(old_ssl);
        SSL_free(old_ssl);
        m_ssl_by_socket_id.erase(client_fd);
    }
    lock.unlock();

    m_ssl_accept_success[client_fd] = false;

    // Close connection as normal
    HttpServer::close_connection(client_fd);
}
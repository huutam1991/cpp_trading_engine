#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <coroutine/epoll_base.h>
#include "https_client_request_io.h"

#define BUFFER_SIZE 2048

HttpClientRequestIO::HttpClientRequestIO(const std::string& hostname_value, int port_value)
    : hostname{hostname_value}, ip{resolve_hostname()}, port{port_value}, m_tls_wrapper{std::make_unique<TlsWrapper>(get_tls_context())}
{
    // Set SNI
    SSL_set_tlsext_host_name(m_tls_wrapper->get_ssl(), hostname.c_str());
}

HttpClientRequestIO::~HttpClientRequestIO()
{
    spdlog::info("HttpClientRequestIO::~HttpClientRequestIO - Destroying HttpClientRequestIO, fd = {}, ip: {}, port: {}", fd, ip, port);

    // No need to call [on_disconnect_callback] + [on_response_received_callback], because this is intend release
    on_disconnect_callback = nullptr;
    on_response_received_callback = nullptr;

    if (fd != -1 && epoll_base != nullptr)
    {
        epoll_base->del_fd(fd, this);
    }
}

void HttpClientRequestIO::set_on_disconnect_callback(std::function<void()> callback)
{
    on_disconnect_callback = callback;
}

void HttpClientRequestIO::set_on_response_received_callback(std::function<void(const char* buffer, std::uint32_t size)> callback)
{
    on_response_received_callback = callback;
}

void HttpClientRequestIO::write(std::string data)
{
    write_queue.push(std::move(data));
    check_to_write();
}

TlsContext* HttpClientRequestIO::get_tls_context()
{
    static TlsClientContext client_ctx{false, ""};
    return &client_ctx;
}

std::string HttpClientRequestIO::resolve_hostname()
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    addrinfo* result = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0)
    {
        spdlog::error("HttpClientRequestIO::resolve_hostname - getaddrinfo failed for {}: {}", hostname, gai_strerror(ret));
        return ""; // fail
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    std::string ip = ip_str;

    freeaddrinfo(result);

    spdlog::info("HttpClientRequestIO::resolve_hostname - Resolved {} to {}", hostname, ip);

    return ip;
}

int HttpClientRequestIO::read_buffer(char* const buffer)
{
    return m_tls_wrapper->read(buffer, BUFFER_SIZE);
}

int HttpClientRequestIO::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    return m_tls_wrapper->write(buffer, size);
}

int HttpClientRequestIO::check_connect_and_handshake()
{
    current_state = State::CONNECTING_AND_HANDSHAKING;

    if (is_connected == false)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err != 0)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - Connect failed: {}, ip: {}, port: {}", strerror(err), ip, port);
            return -1; // close
        }

        is_connected = true;
        spdlog::info("HttpClientRequestIO::handle_io_data - TCP connect success, ip: {}, port: {}", ip, port);

        // Attach fd to TLS wrapper
        if (m_tls_wrapper->attach_fd(fd) == false)
        {
            spdlog::error("HttpClientRequestIO::handle_io_data - attach_fd failed");
            return -1;
        }
    }

    // Check to TLS handshake non-blocking
    if (m_tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = m_tls_wrapper->handshake();
        if (result == TlsResult::OK)
        {
            spdlog::info("HttpClientRequestIO::handle_io_data - TLS handshake success, ip: {}, port: {}", ip, port);
        }
        return result != TlsResult::ERROR ? 0 : -1;
    }

    return 0;
}

int HttpClientRequestIO::handle_read_data()
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    int read_bytes = 0;
    int buffer_length = 0;

    if ((read_bytes = read_buffer(temp_buffer)) >= 0)
    {
        memcpy(buffer + buffer_length, temp_buffer, read_bytes);
        buffer_length += read_bytes;

        while ((read_bytes = read_buffer(temp_buffer)) > 0)
        {
            memcpy(buffer + buffer_length, temp_buffer, read_bytes);
            buffer_length += read_bytes;
        }

        buffer[buffer_length] = '\0';

        if (buffer_length > 0 && on_response_received_callback != nullptr)
        {
            on_response_received_callback(buffer, buffer_length);
        }
    }
    else
    {
        spdlog::debug("HttpClientRequestIO::handle_io_data - connection lost, fd = {}", fd);
        // Clean save buffer
        // save_buffer = "";
        return -1;
    }

    return 0;
}

int HttpClientRequestIO::check_to_write()
{
    if (current_state != State::WRITING)
    {
        return 0;
    }

    while (write_queue.empty() == false)
    {
        const std::string& data = write_queue.front();
        int written_bytes = write_to_socket_io(data.c_str(), data.size());
        if (written_bytes == -1)
        {
            spdlog::error("HttpClientRequestIO::handle_write - write failed, ip: {}, port: {}", ip, port);
            return -1;
        }

        write_queue.pop();
    }

    return 0;
}

int HttpClientRequestIO::generate_fd()
{
    fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    return fd;
}

int HttpClientRequestIO::activate()
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int res = ::connect(fd, (sockaddr*)&addr, sizeof(addr));

    if (res < 0)
    {
        // Non-blocking connect in progress
        if (errno == EINPROGRESS)
        {
            return 0;
        }

        close(fd);
        return -1;
    }
    return 0;
}

int HttpClientRequestIO::handle_read()
{
    // Check connect and handshake
    if (is_connected == false || m_tls_wrapper->is_handshake_done() == false)
    {
        return check_connect_and_handshake();
    }

    // Handle read data
    current_state = State::READING;
    return handle_read_data();
}

int HttpClientRequestIO::handle_write()
{
    // Check connect and handshake
    if (is_connected == false || m_tls_wrapper->is_handshake_done() == false)
    {
        return check_connect_and_handshake();
    }

    // Handle write data
    current_state = State::WRITING;
    return check_to_write();
}

void HttpClientRequestIO::release()
{
    spdlog::info("HttpClientRequestIO::release - Releasing HttpClientRequestIO, fd = {}, ip: {}, port: {}", fd, ip, port);

    fd = -1;
    epoll_base = nullptr;
    is_connected = false;

    if (on_disconnect_callback != nullptr)
    {
        on_disconnect_callback();
    }
}
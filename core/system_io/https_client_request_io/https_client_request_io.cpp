#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <coroutine/epoll_base.h>
#include "https_client_request_io.h"

#define BUFFER_SIZE 3000000
#define BUFFER_TEMP_SIZE 2048

HttpsClientRequestIO::HttpsClientRequestIO(const std::string& hostname_value, int port_value)
    : hostname{hostname_value}, ip{resolve_hostname()}, port{port_value}, m_tls_wrapper{std::make_unique<TlsWrapper>(get_tls_context())}
{
    // Set SNI
    SSL_set_tlsext_host_name(m_tls_wrapper->get_ssl(), hostname.c_str());
}

HttpsClientRequestIO::~HttpsClientRequestIO()
{
    spdlog::debug("HttpsClientRequestIO::~HttpsClientRequestIO - Destroying HttpsClientRequestIO, fd = {}, ip: {}, port: {}", fd, ip, port);

    // No need to call [on_disconnect_callback] + [on_response_received_callback], because this is intend release
    on_disconnect_callback = nullptr;
    on_response_received_callback = nullptr;

    if (fd != -1 && epoll_base != nullptr)
    {
        epoll_base->del_fd(fd, this);
    }
}

void HttpsClientRequestIO::set_on_connect_callback(std::function<void()> callback)
{
    on_connect_callback = std::move(callback);
}

void HttpsClientRequestIO::set_on_disconnect_callback(std::function<void()> callback)
{
    on_disconnect_callback = std::move(callback);
}

void HttpsClientRequestIO::set_on_response_received_callback(std::function<void(const char* buffer, std::uint32_t size)> callback)
{
    on_response_received_callback = std::move(callback);
}

void HttpsClientRequestIO::write(std::string data)
{
    if (data.empty())
    {
        return;
    }

    // If there is pending data in the queue, push new data to the queue and wait for the turn to write
    if (!m_write_queue.empty() || current_state != State::READING_AND_WRITING)
    {
        m_write_queue.push_back(std::move(data));
        enable_write_event();
        return;
    }

    const int n = write_to_socket_io(data.data(), 0, static_cast<std::uint32_t>(data.size()));

    if (n == static_cast<int>(data.size()))
    {
        // Already write full data, return
        return;
    }

    if (n > 0)
    {
        // Partial write: queue the remaining data
        data.erase(0, static_cast<std::size_t>(n));
        m_write_queue.push_back(std::move(data));
        m_write_offset = 0;
        enable_write_event();
        return;
    }

    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        // Socket buffer full: queue the whole data and wait for the turn to write
        m_write_queue.push_back(std::move(data));
        m_write_offset = 0;
        enable_write_event();
        return;
    }

    spdlog::error("HttpsClientRequestIO::write_raw - write failed fd = {}, err = {}, data = {}", fd, std::strerror(errno), data);
}

TlsContext* HttpsClientRequestIO::get_tls_context()
{
    static TlsClientContext client_ctx{false, ""};
    return &client_ctx;
}

std::string HttpsClientRequestIO::resolve_hostname()
{
    addrinfo hints{};
    hints.ai_family   = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    addrinfo* result = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0)
    {
        spdlog::error("HttpsClientRequestIO::resolve_hostname - getaddrinfo failed for {}: {}", hostname, gai_strerror(ret));
        return ""; // fail
    }

    char ip_str[INET_ADDRSTRLEN] = {0};

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, sizeof(ip_str));
    std::string ip = ip_str;

    freeaddrinfo(result);

    spdlog::debug("HttpsClientRequestIO::resolve_hostname - Resolved {} to {}", hostname, ip);

    return ip;
}

int HttpsClientRequestIO::read_buffer(char* const buffer)
{
    return m_tls_wrapper->read(buffer, BUFFER_TEMP_SIZE);
}

int HttpsClientRequestIO::write_to_socket_io(const char* buffer, int current_write_offset, std::uint32_t size)
{
    return m_tls_wrapper->write(buffer, current_write_offset, size);
}

int HttpsClientRequestIO::check_connect_and_handshake()
{
    current_state = State::CONNECTING_AND_HANDSHAKING;

    if (is_connected == false)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err != 0)
        {
            spdlog::error("HttpsClientRequestIO::handle_io_data - Connect failed: {}, ip: {}, port: {}", strerror(err), ip, port);
            return -1; // close
        }

        is_connected = true;
        spdlog::debug("HttpsClientRequestIO::handle_io_data - TCP connect success, ip: {}, port: {}", ip, port);

        // Attach fd to TLS wrapper
        if (m_tls_wrapper->attach_fd(fd) == false)
        {
            spdlog::error("HttpsClientRequestIO::handle_io_data - attach_fd failed");
            return -1;
        }
    }

    // Check to TLS handshake non-blocking
    if (m_tls_wrapper->is_handshake_done() == false)
    {
        TlsResult result = m_tls_wrapper->handshake();
        if (result == TlsResult::OK)
        {
            spdlog::debug("HttpsClientRequestIO::handle_io_data - TLS handshake success, ip: {}, port: {}", ip, port);
        }

        int connect_result = result != TlsResult::ERROR ? 0 : -1;
        if (result == TlsResult::OK && connect_result == 0 && on_connect_callback != nullptr)
        {
            current_state = State::READING_AND_WRITING;
            on_connect_callback();
        }

        return connect_result;
    }

    return 0;
}

int HttpsClientRequestIO::handle_read_data()
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_TEMP_SIZE];
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
        spdlog::debug("HttpsClientRequestIO::handle_io_data - connection lost, fd = {}", fd);
        // Clean save buffer
        // save_buffer = "";
        return -1;
    }

    return 0;
}

int HttpsClientRequestIO::check_to_write()
{
    while (!m_write_queue.empty())
    {
        std::string& data = m_write_queue.front();
        const int n = write_to_socket_io(data.data(), m_write_offset, data.size());

        if (n > 0)
        {
            m_write_offset += static_cast<std::size_t>(n);

            if (m_write_offset == data.size())
            {
                m_write_queue.pop_front();
                m_write_offset = 0;
                continue;
            }

            // Partial again. Wait for next EPOLLOUT.
            return 0;
        }

        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            // Still not writable enough.
            return 0;
        }

        spdlog::error("HttpsClientRequestIO::handle_write - write failed fd = {}, err = {}", fd, std::strerror(errno));
        return -1;
    }

    // Queue empty => no need to receive EPOLLOUT anymore.
    disable_write_event();
    return 0;
}

void HttpsClientRequestIO::enable_write_event()
{
    static constexpr uint32_t READ_WRITE_EVENTS = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if (m_write_event_enabled == true)
    {
        return;
    }

    m_write_event_enabled = true;
    epoll_base->mod_fd_events(fd, this, READ_WRITE_EVENTS);
}

void HttpsClientRequestIO::disable_write_event()
{
    static constexpr uint32_t READ_EVENTS = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if (m_write_event_enabled == false)
    {
        return;
    }

    m_write_event_enabled = false;
    epoll_base->mod_fd_events(fd, this, READ_EVENTS);
}

int HttpsClientRequestIO::generate_fd()
{
    fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;

    return fd;
}

int HttpsClientRequestIO::activate()
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

        return -1;
    }
    return 0;
}

int HttpsClientRequestIO::handle_read()
{
    // Check connect and handshake
    if (is_connected == false || m_tls_wrapper->is_handshake_done() == false)
    {
        return check_connect_and_handshake();
    }

    // Handle read data
    return handle_read_data();
}

int HttpsClientRequestIO::handle_write()
{
    // Check connect and handshake
    if (is_connected == false || m_tls_wrapper->is_handshake_done() == false)
    {
        return check_connect_and_handshake();
    }

    // Handle write data
    return check_to_write();
}

void HttpsClientRequestIO::release()
{
    spdlog::debug("HttpsClientRequestIO::release - Releasing HttpsClientRequestIO, fd = {}, ip: {}, port: {}", fd, ip, port);

    fd = -1;
    epoll_base = nullptr;
    is_connected = false;

    if (on_disconnect_callback != nullptr)
    {
        on_disconnect_callback();
    }
}
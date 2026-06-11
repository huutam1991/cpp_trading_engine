#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "http_socket_connection.h"

#define BUFFER_SIZE 2048

void HttpSocketConnection::set_server_fd(int fd_value)
{
    server_fd = fd_value;
}

void HttpSocketConnection::refresh()
{
    server_fd = -1;
    save_buffer = "";
    client_ip = "";
}

int HttpSocketConnection::generate_fd()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1)
    {
        spdlog::error("HttpSocketConnection::generate_fd - HttpServer - accept: {}", std::strerror(errno));
        return -1;
    }
    else
    {
        client_ip = inet_ntoa(client_addr.sin_addr);
        spdlog::info("HttpSocketConnection::generate_fd - Connection from {}, established (fd = {})", client_ip, fd);

        // Set non-blocking
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        {
            spdlog::error("fcntl failed fd {} err {}", fd, strerror(errno));
            return -1;
        }

        int dwTimeout = 1000; // milliseconds
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&dwTimeout, sizeof dwTimeout);
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&dwTimeout, sizeof dwTimeout);

        int buffer_size = 1024 * 1024; // 1 MB
        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

        int n;
        unsigned int m = sizeof(n);
        getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&n, &m);
        // spdlog::debug("fd = {}, Receive buffer = {}", fd, n);
        getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&n, &m);
        // spdlog::debug("fd = {}, Send buffer = {}", fd, n);
    }

    return fd;
}

int HttpSocketConnection::activate()
{
    // Nothing to do for client socket
    return 0;
}

int HttpSocketConnection::handle_read()
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
    }
    else
    {
        spdlog::debug("HttpSocketConnection::handle_io_data - connection lost, fd = {}", fd);
        // Clean save buffer
        save_buffer = "";
        return -1;
    }

    std::string message = save_buffer + std::string(buffer);
    HttpRequest* request = HttpRequest::CreateNewHttpRequest(message.c_str(), "web_data"); // Temporarily hard code path: web_data

    // Check if request is nullptr (wrong format), return error 404
    if (request == nullptr)
    {
        // Execute on a single thread
        auto task = send_404_response(request);
        task.start_running_on((EventBase*)epoll_base);

        return 0;
    }

    // Post request is invalid format when it's is separated into 2 buffer, that's why we need to save the first buffer
    if (request->is_valid_format() == false)
    {
        save_buffer += std::string(buffer);

        delete request;
        return 0;
    }

    // Otherwise, clean save buffer + and execute request
    save_buffer = "";

    // Set client IP to request
    request->set_client_ip(client_ip);

    // Execute request on a single thread
    auto task = execute_request(request);
    task.start_running_on((EventBase*)epoll_base);

    return 0;
}

int HttpSocketConnection::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void HttpSocketConnection::release()
{
    HttpClientSocketPool::release(this);
}

Task<void> HttpSocketConnection::send_404_response(HttpRequest* request)
{
    std::string response = request->response_not_found_404().get_response_in_string();

    int res = write_to_socket_io(response.c_str(), response.size());
    if (res == -1)
    {
        spdlog::error("HttpSocketConnection::send_404_response - write failed, socket fd = {}", fd);
        epoll_base->del_fd(fd, this);
    }

    // Clean save buffer
    save_buffer = "";

    co_return;
}

Task<void> HttpSocketConnection::execute_request(HttpRequest* request)
{
    size_t start_time = Utils::get_time_now_in_utc_nanoseconds();

    std::string response = co_await RouteController::instance().handle_request_base_on_route(request);

    int res = write_to_socket_io(response.c_str(), response.size());
    if (res == -1)
    {
        spdlog::error("HttpSocketConnection::send_404_response - write failed, socket fd = {}", fd);
        epoll_base->del_fd(fd, this);
    }

    std::string endpoint = request->get_url();
    std::string client_ip = request->get_client_ip();
    std::string host = request->get_header_param("Host");
    std::string user_agent = request->get_header_param("User-Agent");
    RequestMethod method = request->get_request_method();

    // Check version lowercase for better log consistency
    host = host != PARAM_NOT_FOUND ? host : request->get_header_param("host");
    user_agent = user_agent != PARAM_NOT_FOUND ? user_agent : request->get_header_param("user-agent");

    delete request;

    size_t end_time = Utils::get_time_now_in_utc_nanoseconds();
    size_t duration = end_time - start_time;
    std::string start_time_str = Utils::get_string_time_from_utc_nanoseconds(start_time);
    std::string duration_str = Utils::get_duration_string_from_nanoseconds(duration);

    MongoDB::instance()
        .set_db_and_collection("system_monitoring", "request_log")
        .insert_one(Json{
            {"created_at_ns", start_time},
            {"start_time", start_time_str},
            {"duration", duration_str},
            {"endpoint", endpoint},
            {"method", enum_reflect::enum_name<RequestMethod>(method)},
            {"client_ip", client_ip},
            {"host", host},
            {"user_agent", user_agent}
        });

    co_return;
}

int HttpSocketConnection::read_buffer(char* const buffer)
{
    return ::read(fd, buffer, BUFFER_SIZE);
}

int HttpSocketConnection::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    return ::write(fd, buffer, size);
}
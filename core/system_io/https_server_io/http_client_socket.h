#pragma once

#include <spdlog/spdlog.h>
#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <cache/cache_pool.h>
#include <network/https_server/request/http_request.h>
#include <network/https_server/route/route_controller.h>
#include <system_io/system_io_object.h>

struct HttpClientSocket : public SystemIOObject
{
    int server_fd;
    std::string save_buffer;

    void set_server_fd(int fd_value);
    void clear();

    // SystemIOObject's methods
    virtual int generate_fd() override;
    virtual int get_io_events() override { return EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;

    // Handle data methods
    virtual int read_buffer(char* const buffer);
    virtual void write_to_socket_io(const char* buffer, std::uint32_t size);

    Task<void> send_404_response(HttpRequest* request);
    Task<void> execute_request(HttpRequest* request);
};

using HttpClientSocketPool = CachePool<HttpClientSocket, 100>;
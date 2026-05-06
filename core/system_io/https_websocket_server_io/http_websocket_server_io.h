#pragma once

#include <functional>
#include <string>

#include <spdlog/spdlog.h>
#include <coroutine/task.h>
#include <system_io/system_io_object.h>

#define MAX_WEBSOCKET_CONNECTIONS 1000

struct HttpWebsocketConnectionIO;

struct HttpWebsocketServerIO : public NamedIOObject<HttpWebsocketServerIO>
{
    int port;
    std::function<Task<bool>(int, std::string, std::string)> on_connect = nullptr;
    std::function<Task<void>(int, std::string)> on_message = nullptr;
    std::function<Task<void>(int)> on_disconnect = nullptr;

    HttpWebsocketServerIO(int port_value);

    void set_callbacks(
        std::function<Task<bool>(int, std::string, std::string)> on_connect_callback = nullptr,
        std::function<Task<void>(int, std::string)> on_message_callback = nullptr,
        std::function<Task<void>(int)> on_disconnect_callback = nullptr);

    // Write message to a specific connection by fd
    void write_to_connection(int fd, std::string message);
    Task<void> write_to_connection_task(int fd, std::string message);

    virtual int generate_fd() override;
    virtual int get_io_events() override { return EPOLLIN; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;

private:
    std::array<HttpWebsocketConnectionIO*, MAX_WEBSOCKET_CONNECTIONS> m_websocket_connections_by_fd{nullptr};

protected:
    void establish_connection(HttpWebsocketConnectionIO* connection);
};

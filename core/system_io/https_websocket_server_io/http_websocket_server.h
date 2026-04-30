#pragma once

#include <functional>
#include <string>

#include <spdlog/spdlog.h>
#include <coroutine/task.h>
#include <system_io/system_io_object.h>

#define MAX_WEBSOCKET_CONNECTIONS 1000

struct HttpWebsocketConnection;

struct HttpWebsocketServer : public NamedIOObject<HttpWebsocketServer>
{
    int port;
    std::function<Task<void>(int)> on_connect = nullptr;
    std::function<Task<void>(int, std::string)> on_message = nullptr;
    std::function<Task<void>(int)> on_disconnect = nullptr;

    HttpWebsocketServer(
        int port_value,
        std::function<Task<void>(int)> on_connect_callback = nullptr,
        std::function<Task<void>(int, std::string)> on_message_callback = nullptr,
        std::function<Task<void>(int)> on_disconnect_callback = nullptr);

    // Public method to send message to a specific websocket connection by fd
    void write_to_connection(int fd, std::string message);

    virtual int generate_fd() override;
    virtual int get_io_events() override { return EPOLLIN; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;

private:
    std::array<HttpWebsocketConnection*, MAX_WEBSOCKET_CONNECTIONS> m_websocket_connections_by_fd{nullptr};

protected:
    void establish_connection(HttpWebsocketConnection* connection);
};

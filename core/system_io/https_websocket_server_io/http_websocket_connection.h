#pragma once

#include <functional>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <cache/cache_pool.h>
#include <system_io/system_io_object.h>

#include <network/https_client_websocket/websocket_frame_builder.h>
#include <network/https_client_websocket/websocket_frame_parser.h>

struct HttpWebsocketConnection : public NamedIOObject<HttpWebsocketConnection>
{
    enum class WebsocketState
    {
        WaitingHttpUpgrade,
        WebSocketOpen,
        Closing
    };

    int server_fd = -1;
    std::string save_buffer;
    WebSocketFrameParser frame_parser;
    WebsocketState WebsocketState = WebsocketState::WaitingHttpUpgrade;

    std::function<Task<void>(int, HttpWebsocketConnection*)> on_connect = nullptr;
    std::function<Task<void>(int, std::string)> on_message = nullptr;
    std::function<Task<void>(int)> on_disconnect = nullptr;

    void set_server_fd(int fd_value);
    void set_callbacks(
        std::function<Task<void>(int, HttpWebsocketConnection*)> on_connect_callback,
        std::function<Task<void>(int, std::string)> on_message_callback,
        std::function<Task<void>(int)> on_disconnect_callback);
    void refresh();

    virtual int generate_fd() override;
    virtual int get_io_events() override { return EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP; }
    virtual int activate() override;
    virtual int handle_read() override;
    virtual int handle_write() override;
    virtual void release() override;

    virtual int read_buffer(char* const buffer, std::size_t size);
    virtual int write_to_socket_io(const char* buffer, std::uint32_t size) override;

    void write_text(const std::string& message);
    void write_ping(const std::string& payload = "");
    void write_pong(const std::string& payload = "");
    void write_close();
    void write_close(std::uint16_t close_code, const std::string& reason = "");

protected:
    static constexpr std::size_t READ_BUFFER_SIZE = 8192;
    static constexpr const char* WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    int handle_http_upgrade_bytes(const char* data, std::size_t size);
    int handle_websocket_bytes(const char* data, std::size_t size);

    bool try_extract_http_request(std::string& request_text, std::string& leftover_data);
    bool is_websocket_upgrade_request(const std::string& request_text) const;
    std::string get_header_value(const std::string& request_text, const std::string& header_name) const;
    std::string trim(const std::string& value) const;
    std::string to_lower(std::string value) const;
    std::string build_websocket_upgrade_response(const std::string& sec_websocket_key) const;
    std::string compute_websocket_accept_key(const std::string& sec_websocket_key) const;
    std::string base64_encode(const unsigned char* data, std::size_t size) const;

    Task<void> run_on_connect();
    Task<void> run_on_message(std::string message);
    Task<void> run_on_disconnect();
    void write_raw_frame(const std::vector<char>& frame);
};

using HttpWebsocketConnectionPool = CachePool<HttpWebsocketConnection, 100>;

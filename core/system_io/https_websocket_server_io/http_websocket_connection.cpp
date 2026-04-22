#include "http_websocket_connection.h"

#include <algorithm>
#include <cctype>
#include <array>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

void HttpWebsocketConnection::set_server_fd(int fd_value)
{
    server_fd = fd_value;
}

void HttpWebsocketConnection::set_callbacks(
    std::function<Task<void>(int)> on_connect_callback,
    std::function<Task<void>(int, std::string)> on_message_callback,
    std::function<Task<void>(int)> on_disconnect_callback)
{
    on_connect = std::move(on_connect_callback);
    on_message = std::move(on_message_callback);
    on_disconnect = std::move(on_disconnect_callback);
}

void HttpWebsocketConnection::clear()
{
    server_fd = -1;
    save_buffer.clear();
    frame_parser.clear();
    state = State::WaitingHttpUpgrade;
    on_connect = nullptr;
    on_message = nullptr;
    on_disconnect = nullptr;
}

int HttpWebsocketConnection::generate_fd()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len)) == -1)
    {
        spdlog::error("HttpWebsocketConnection::generate_fd - accept failed: {}", std::strerror(errno));
        return -1;
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        spdlog::error("HttpWebsocketConnection::generate_fd - fcntl failed for fd {}: {}", fd, std::strerror(errno));
        return -1;
    }

    int buffer_size = 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

    spdlog::info(
        "HttpWebsocketConnection::generate_fd - Connection to {}, established (fd = {})",
        inet_ntoa(client_addr.sin_addr),
        fd);

    return fd;
}

int HttpWebsocketConnection::activate()
{
    return 0;
}

int HttpWebsocketConnection::handle_read()
{
    std::array<char, READ_BUFFER_SIZE> buffer{};

    while (true)
    {
        const int read_bytes = read_buffer(buffer.data(), buffer.size());

        if (read_bytes > 0)
        {
            const int result = (state == State::WaitingHttpUpgrade)
                ? handle_http_upgrade_bytes(buffer.data(), static_cast<std::size_t>(read_bytes))
                : handle_websocket_bytes(buffer.data(), static_cast<std::size_t>(read_bytes));

            if (result != 0)
            {
                return result;
            }

            continue;
        }

        if (read_bytes == 0)
        {
            spdlog::debug("HttpWebsocketConnection::handle_read - peer closed connection, fd = {}", fd);
            return close_connection();
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }

        spdlog::error("HttpWebsocketConnection::handle_read - read failed, fd = {}, err = {}", fd, std::strerror(errno));
        return close_connection();
    }
}

int HttpWebsocketConnection::handle_write()
{
    return 0;
}

void HttpWebsocketConnection::release()
{
    clear();
    HttpWebsocketConnectionPool::release(this);
}

int HttpWebsocketConnection::read_buffer(char* const buffer, std::size_t size)
{
    spdlog::debug("HttpWebsocketConnection::read_buffer - Attempting to read from fd {}, size {}", fd, size);
    spdlog::debug("HttpWebsocketConnection::read_buffer: {}", std::string(buffer, size));
    return ::read(fd, buffer, size);
}

int HttpWebsocketConnection::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    return ::write(fd, buffer, size);
}

void HttpWebsocketConnection::write_text(const std::string& message)
{
    if (state != State::WebSocketOpen)
    {
        return;
    }

    write_raw_frame(WebSocketFrameBuilder::build_text(message, true, false));
}

void HttpWebsocketConnection::write_ping(const std::string& payload)
{
    if (state != State::WebSocketOpen)
    {
        return;
    }

    write_raw_frame(WebSocketFrameBuilder::build_ping(payload, false));
}

void HttpWebsocketConnection::write_pong(const std::string& payload)
{
    if (state != State::WebSocketOpen)
    {
        return;
    }

    write_raw_frame(WebSocketFrameBuilder::build_pong(payload, false));
}

void HttpWebsocketConnection::write_close()
{
    write_close(1000, "");
}

void HttpWebsocketConnection::write_close(std::uint16_t close_code, const std::string& reason)
{
    if (state == State::Closing)
    {
        return;
    }

    state = State::Closing;
    write_raw_frame(WebSocketFrameBuilder::build_close(close_code, reason, false));
}

int HttpWebsocketConnection::handle_http_upgrade_bytes(const char* data, std::size_t size)
{
    save_buffer.append(data, size);

    std::string request_text;
    std::string leftover_data;
    if (!try_extract_http_request(request_text, leftover_data))
    {
        return 0;
    }

    if (!is_websocket_upgrade_request(request_text))
    {
        const std::string bad_request =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";

        write_to_socket_io(bad_request.data(), static_cast<std::uint32_t>(bad_request.size()));
        return close_connection();
    }

    const std::string sec_websocket_key = get_header_value(request_text, "Sec-WebSocket-Key");
    if (sec_websocket_key.empty())
    {
        const std::string bad_request =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";

        write_to_socket_io(bad_request.data(), static_cast<std::uint32_t>(bad_request.size()));
        return close_connection();
    }

    const std::string response = build_websocket_upgrade_response(sec_websocket_key);
    if (write_to_socket_io(response.data(), static_cast<std::uint32_t>(response.size())) == -1)
    {
        spdlog::error("HttpWebsocketConnection::handle_http_upgrade_bytes - failed to send upgrade response, fd = {}", fd);
        return close_connection();
    }

    state = State::WebSocketOpen;
    save_buffer.clear();

    run_on_connect().start_running_on((EventBase*)epoll_base);

    if (!leftover_data.empty())
    {
        return handle_websocket_bytes(leftover_data.data(), leftover_data.size());
    }

    return 0;
}

int HttpWebsocketConnection::handle_websocket_bytes(const char* data, std::size_t size)
{
    frame_parser.feed(data, size);
    std::vector<WebSocketFrameParser::Frame> frames = frame_parser.parse_frames();

    for (const auto& frame : frames)
    {
        if (frame.opcode == WebSocketFrameParser::Opcode::Text)
        {
            run_on_message(frame.payload_as_string()).start_running_on((EventBase*)epoll_base);
        }
        else if (frame.opcode == WebSocketFrameParser::Opcode::Binary)
        {
            spdlog::debug("HttpWebsocketConnection::handle_websocket_bytes - binary frame received, fd = {}, size = {}", fd, frame.payload.size());
        }
        else if (frame.opcode == WebSocketFrameParser::Opcode::Ping)
        {
            write_pong(frame.payload_as_string());
        }
        else if (frame.opcode == WebSocketFrameParser::Opcode::Pong)
        {
            spdlog::debug("HttpWebsocketConnection::handle_websocket_bytes - pong received, fd = {}, size = {}", fd, frame.payload.size());
        }
        else if (frame.opcode == WebSocketFrameParser::Opcode::Close)
        {
            if (state != State::Closing)
            {
                if (frame.payload.size() >= 2)
                {
                    const std::uint16_t close_code =
                        (static_cast<std::uint16_t>(static_cast<unsigned char>(frame.payload[0])) << 8) |
                        static_cast<std::uint16_t>(static_cast<unsigned char>(frame.payload[1]));
                    const std::string reason(frame.payload.begin() + 2, frame.payload.end());
                    write_close(close_code, reason);
                }
                else
                {
                    write_close();
                }
            }

            return close_connection();
        }
    }

    return 0;
}

int HttpWebsocketConnection::close_connection()
{
    state = State::Closing;
    run_on_disconnect().start_running_on((EventBase*)epoll_base);
    epoll_base->del_fd(fd, this);
    return -1;
}

bool HttpWebsocketConnection::try_extract_http_request(std::string& request_text, std::string& leftover_data)
{
    const std::size_t header_end = save_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
        return false;
    }

    const std::size_t request_end = header_end + 4;
    request_text = save_buffer.substr(0, request_end);
    leftover_data = save_buffer.substr(request_end);
    return true;
}

bool HttpWebsocketConnection::is_websocket_upgrade_request(const std::string& request_text) const
{
    std::istringstream stream(request_text);
    std::string request_line;
    if (!std::getline(stream, request_line))
    {
        return false;
    }

    if (request_line.find("GET ") != 0)
    {
        return false;
    }

    const std::string upgrade = to_lower(get_header_value(request_text, "Upgrade"));
    const std::string connection = to_lower(get_header_value(request_text, "Connection"));
    const std::string version = trim(get_header_value(request_text, "Sec-WebSocket-Version"));

    if (upgrade != "websocket")
    {
        return false;
    }

    if (connection.find("upgrade") == std::string::npos)
    {
        return false;
    }

    if (!version.empty() && version != "13")
    {
        return false;
    }

    return true;
}

std::string HttpWebsocketConnection::get_header_value(const std::string& request_text, const std::string& header_name) const
{
    const std::string lower_header_name = to_lower(header_name);
    std::istringstream stream(request_text);
    std::string line;

    std::getline(stream, line);
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            break;
        }

        const std::size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos)
        {
            continue;
        }

        const std::string current_name = to_lower(trim(line.substr(0, colon_pos)));
        if (current_name == lower_header_name)
        {
            return trim(line.substr(colon_pos + 1));
        }
    }

    return "";
}

std::string HttpWebsocketConnection::trim(const std::string& value) const
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

std::string HttpWebsocketConnection::to_lower(std::string value) const
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
    {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string HttpWebsocketConnection::build_websocket_upgrade_response(const std::string& sec_websocket_key) const
{
    const std::string sec_websocket_accept = compute_websocket_accept_key(sec_websocket_key);

    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << sec_websocket_accept << "\r\n\r\n";
    return response.str();
}

std::string HttpWebsocketConnection::compute_websocket_accept_key(const std::string& sec_websocket_key) const
{
    const std::string input = trim(sec_websocket_key) + WEBSOCKET_GUID;

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(), sha1_result);

    return base64_encode(sha1_result, sizeof(sha1_result));
}

std::string HttpWebsocketConnection::base64_encode(const unsigned char* data, std::size_t size) const
{
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string out;
    out.reserve(((size + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < size)
    {
        const std::uint32_t triple =
            (static_cast<std::uint32_t>(data[i]) << 16) |
            (static_cast<std::uint32_t>(data[i + 1]) << 8) |
            static_cast<std::uint32_t>(data[i + 2]);

        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back(table[(triple >> 6) & 0x3F]);
        out.push_back(table[triple & 0x3F]);
        i += 3;
    }

    if (i < size)
    {
        const std::uint32_t b0 = data[i];
        const std::uint32_t b1 = (i + 1 < size) ? data[i + 1] : 0;
        const std::uint32_t triple = (b0 << 16) | (b1 << 8);

        out.push_back(table[(triple >> 18) & 0x3F]);
        out.push_back(table[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < size) ? table[(triple >> 6) & 0x3F] : '=');
        out.push_back('=');
    }

    return out;
}

Task<void> HttpWebsocketConnection::run_on_connect()
{
    if (on_connect != nullptr)
    {
        co_await on_connect(fd);
    }

    co_return;
}

Task<void> HttpWebsocketConnection::run_on_message(std::string message)
{
    if (on_message != nullptr)
    {
        co_await on_message(fd, std::move(message));
    }

    co_return;
}

Task<void> HttpWebsocketConnection::run_on_disconnect()
{
    if (on_disconnect != nullptr)
    {
        co_await on_disconnect(fd);
    }

    co_return;
}

void HttpWebsocketConnection::write_raw_frame(const std::vector<char>& frame)
{
    if (frame.empty())
    {
        return;
    }

    const int result = write_to_socket_io(frame.data(), static_cast<std::uint32_t>(frame.size()));
    if (result == -1)
    {
        spdlog::error("HttpWebsocketConnection::write_raw_frame - write failed, fd = {}", fd);
        close_connection();
    }
}

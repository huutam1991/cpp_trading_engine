#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

class WebSocketFrameParser
{
public:
    enum class Opcode : std::uint8_t
    {
        Continuation = 0x0,
        Text         = 0x1,
        Binary       = 0x2,
        Close        = 0x8,
        Ping         = 0x9,
        Pong         = 0xA
    };

    struct Frame
    {
        bool fin = false;
        bool rsv1 = false;
        bool rsv2 = false;
        bool rsv3 = false;
        bool masked = false;
        Opcode opcode = Opcode::Continuation;
        std::vector<char> payload;

        bool is_control_frame() const
        {
            const auto op = static_cast<std::uint8_t>(opcode);
            return op >= 0x8;
        }

        std::string payload_as_string() const
        {
            return std::string(payload.begin(), payload.end());
        }
    };

public:
    WebSocketFrameParser() = default;
    ~WebSocketFrameParser() = default;

    void feed(const char* data, std::size_t size);

    // Parse as many frames as possible from internal buffer.
    std::vector<Frame> parse_frames();

    void clear();

    std::size_t buffered_size() const
    {
        return m_buffer.size() - m_parse_offset;
    }

private:
    std::optional<Frame> try_parse_one_frame();

private:
    std::vector<char> m_buffer;
    std::size_t m_parse_offset = 0;
};
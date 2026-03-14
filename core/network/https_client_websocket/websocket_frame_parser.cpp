#include "websocket_frame_parser.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace
{
    static std::uint16_t read_u16_be(const char* p)
    {
        return (static_cast<std::uint16_t>(static_cast<unsigned char>(p[0])) << 8) |
               (static_cast<std::uint16_t>(static_cast<unsigned char>(p[1])));
    }

    static std::uint64_t read_u64_be(const char* p)
    {
        return (static_cast<std::uint64_t>(static_cast<unsigned char>(p[0])) << 56) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[1])) << 48) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[2])) << 40) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[3])) << 32) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[4])) << 24) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[5])) << 16) |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[6])) << 8)  |
               (static_cast<std::uint64_t>(static_cast<unsigned char>(p[7])));
    }
}

void WebSocketFrameParser::feed(const char* data, std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return;
    }

    m_buffer.insert(m_buffer.end(), data, data + size);
}

std::vector<WebSocketFrameParser::Frame> WebSocketFrameParser::parse_frames()
{
    std::vector<Frame> frames;

    while (true)
    {
        auto frame = try_parse_one_frame();
        if (!frame.has_value())
        {
            break;
        }

        frames.push_back(std::move(frame.value()));
    }

    // Compact buffer if parsed prefix is large enough.
    if (m_parse_offset > 0)
    {
        if (m_parse_offset == m_buffer.size())
        {
            m_buffer.clear();
            m_parse_offset = 0;
        }
        else if (m_parse_offset >= 4096 || m_parse_offset > (m_buffer.size() / 2))
        {
            std::vector<char> new_buffer;
            new_buffer.reserve(m_buffer.size() - m_parse_offset);
            new_buffer.insert(new_buffer.end(), m_buffer.begin() + static_cast<std::ptrdiff_t>(m_parse_offset), m_buffer.end());
            m_buffer.swap(new_buffer);
            m_parse_offset = 0;
        }
    }

    return frames;
}

void WebSocketFrameParser::clear()
{
    m_buffer.clear();
    m_parse_offset = 0;
}

std::optional<WebSocketFrameParser::Frame> WebSocketFrameParser::try_parse_one_frame()
{
    const std::size_t available = m_buffer.size() - m_parse_offset;
    if (available < 2)
    {
        return std::nullopt;
    }

    const char* p = m_buffer.data() + m_parse_offset;

    const std::uint8_t b0 = static_cast<std::uint8_t>(static_cast<unsigned char>(p[0]));
    const std::uint8_t b1 = static_cast<std::uint8_t>(static_cast<unsigned char>(p[1]));

    Frame frame;
    frame.fin    = (b0 & 0x80) != 0;
    frame.rsv1   = (b0 & 0x40) != 0;
    frame.rsv2   = (b0 & 0x20) != 0;
    frame.rsv3   = (b0 & 0x10) != 0;
    frame.opcode = static_cast<Opcode>(b0 & 0x0F);
    frame.masked = (b1 & 0x80) != 0;

    std::uint64_t payload_len = (b1 & 0x7F);
    std::size_t header_size = 2;

    if (payload_len == 126)
    {
        if (available < header_size + 2)
        {
            return std::nullopt;
        }

        payload_len = read_u16_be(p + header_size);
        header_size += 2;
    }
    else if (payload_len == 127)
    {
        if (available < header_size + 8)
        {
            return std::nullopt;
        }

        payload_len = read_u64_be(p + header_size);
        header_size += 8;
    }

    char masking_key[4] = {0, 0, 0, 0};
    if (frame.masked)
    {
        if (available < header_size + 4)
        {
            return std::nullopt;
        }

        std::memcpy(masking_key, p + header_size, 4);
        header_size += 4;
    }

    if (payload_len > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        throw std::runtime_error("WebSocketFrameParser payload too large");
    }

    const std::size_t total_size = header_size + static_cast<std::size_t>(payload_len);
    if (available < total_size)
    {
        return std::nullopt;
    }

    frame.payload.resize(static_cast<std::size_t>(payload_len));
    if (payload_len > 0)
    {
        const char* payload_ptr = p + header_size;

        if (!frame.masked)
        {
            std::memcpy(frame.payload.data(), payload_ptr, static_cast<std::size_t>(payload_len));
        }
        else
        {
            for (std::size_t i = 0; i < static_cast<std::size_t>(payload_len); ++i)
            {
                frame.payload[i] = static_cast<char>(
                    payload_ptr[i] ^ masking_key[i & 0x3]
                );
            }
        }
    }

    m_parse_offset += total_size;
    return frame;
}
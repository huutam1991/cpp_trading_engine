#include "websocket_frame_builder.h"

#include <random>
#include <stdexcept>

namespace
{
    static void append_u16_be(std::vector<char>& out, std::uint16_t value)
    {
        out.push_back(static_cast<char>((value >> 8) & 0xFF));
        out.push_back(static_cast<char>(value & 0xFF));
    }

    static void append_u64_be(std::vector<char>& out, std::uint64_t value)
    {
        out.push_back(static_cast<char>((value >> 56) & 0xFF));
        out.push_back(static_cast<char>((value >> 48) & 0xFF));
        out.push_back(static_cast<char>((value >> 40) & 0xFF));
        out.push_back(static_cast<char>((value >> 32) & 0xFF));
        out.push_back(static_cast<char>((value >> 24) & 0xFF));
        out.push_back(static_cast<char>((value >> 16) & 0xFF));
        out.push_back(static_cast<char>((value >> 8) & 0xFF));
        out.push_back(static_cast<char>(value & 0xFF));
    }

    static bool is_control_opcode(WebSocketFrameBuilder::Opcode opcode)
    {
        const auto op = static_cast<std::uint8_t>(opcode);
        return op >= 0x8;
    }
}

std::vector<char> WebSocketFrameBuilder::build_frame(
    Opcode opcode,
    const char* payload,
    std::uint64_t payload_size,
    bool fin,
    bool mask)
{
    if (payload == nullptr && payload_size > 0)
    {
        throw std::runtime_error("WebSocketFrameBuilder payload is null");
    }

    if (is_control_opcode(opcode))
    {
        if (!fin)
        {
            throw std::runtime_error("Control frame must not be fragmented");
        }

        if (payload_size > 125)
        {
            throw std::runtime_error("Control frame payload must be <= 125");
        }
    }

    std::vector<char> out;
    out.reserve(static_cast<std::size_t>(2 + (payload_size <= 125 ? 0 : (payload_size <= 65535 ? 2 : 8)) + (mask ? 4 : 0) + payload_size));

    const std::uint8_t first_byte =
        static_cast<std::uint8_t>((fin ? 0x80 : 0x00) |
                                  static_cast<std::uint8_t>(opcode));

    out.push_back(static_cast<char>(first_byte));

    append_payload_length(out, payload_size, mask);

    std::uint8_t mask_key[4] = {0, 0, 0, 0};

    if (mask)
    {
        generate_masking_key(mask_key);
        append_masking_key(out, mask_key);
        append_payload_masked(out, payload, payload_size, mask_key);
    }
    else
    {
        append_payload_unmasked(out, payload, payload_size);
    }

    return out;
}

std::vector<char> WebSocketFrameBuilder::build_frame(
    Opcode opcode,
    const std::string& payload,
    bool fin,
    bool mask)
{
    return build_frame(opcode, payload.data(), static_cast<std::uint64_t>(payload.size()), fin, mask);
}

std::vector<char> WebSocketFrameBuilder::build_frame(
    Opcode opcode,
    const std::vector<char>& payload,
    bool fin,
    bool mask)
{
    if (payload.empty())
    {
        return build_frame(opcode, nullptr, 0, fin, mask);
    }

    return build_frame(opcode, payload.data(), static_cast<std::uint64_t>(payload.size()), fin, mask);
}

std::vector<char> WebSocketFrameBuilder::build_text(
    const std::string& text,
    bool fin,
    bool mask)
{
    return build_frame(Opcode::Text, text, fin, mask);
}

std::vector<char> WebSocketFrameBuilder::build_binary(
    const std::vector<char>& payload,
    bool fin,
    bool mask)
{
    return build_frame(Opcode::Binary, payload, fin, mask);
}

std::vector<char> WebSocketFrameBuilder::build_ping(
    const std::string& payload,
    bool mask)
{
    return build_frame(Opcode::Ping, payload, true, mask);
}

std::vector<char> WebSocketFrameBuilder::build_pong(
    const std::string& payload,
    bool mask)
{
    return build_frame(Opcode::Pong, payload, true, mask);
}

std::vector<char> WebSocketFrameBuilder::build_close(
    bool mask)
{
    return build_frame(Opcode::Close, nullptr, 0, true, mask);
}

std::vector<char> WebSocketFrameBuilder::build_close(
    std::uint16_t close_code,
    const std::string& reason,
    bool mask)
{
    std::vector<char> payload;
    payload.reserve(2 + reason.size());

    payload.push_back(static_cast<char>((close_code >> 8) & 0xFF));
    payload.push_back(static_cast<char>(close_code & 0xFF));
    payload.insert(payload.end(), reason.begin(), reason.end());

    return build_frame(Opcode::Close, payload, true, mask);
}

void WebSocketFrameBuilder::append_payload_length(
    std::vector<char>& out,
    std::uint64_t payload_size,
    bool mask)
{
    const std::uint8_t mask_bit = mask ? 0x80 : 0x00;

    if (payload_size <= 125)
    {
        out.push_back(static_cast<char>(mask_bit | static_cast<std::uint8_t>(payload_size)));
        return;
    }

    if (payload_size <= 65535)
    {
        out.push_back(static_cast<char>(mask_bit | 126));
        append_u16_be(out, static_cast<std::uint16_t>(payload_size));
        return;
    }

    out.push_back(static_cast<char>(mask_bit | 127));
    append_u64_be(out, payload_size);
}

void WebSocketFrameBuilder::append_masking_key(
    std::vector<char>& out,
    std::uint8_t mask_key[4])
{
    out.push_back(static_cast<char>(mask_key[0]));
    out.push_back(static_cast<char>(mask_key[1]));
    out.push_back(static_cast<char>(mask_key[2]));
    out.push_back(static_cast<char>(mask_key[3]));
}

void WebSocketFrameBuilder::append_payload_masked(
    std::vector<char>& out,
    const char* payload,
    std::uint64_t payload_size,
    const std::uint8_t mask_key[4])
{
    for (std::uint64_t i = 0; i < payload_size; ++i)
    {
        const std::uint8_t c = static_cast<std::uint8_t>(static_cast<unsigned char>(payload[i]));
        out.push_back(static_cast<char>(c ^ mask_key[i & 0x3]));
    }
}

void WebSocketFrameBuilder::append_payload_unmasked(
    std::vector<char>& out,
    const char* payload,
    std::uint64_t payload_size)
{
    if (payload == nullptr || payload_size == 0)
    {
        return;
    }

    out.insert(out.end(), payload, payload + payload_size);
}

void WebSocketFrameBuilder::generate_masking_key(std::uint8_t mask_key[4])
{
    static thread_local std::mt19937 rng{std::random_device{}()};
    static thread_local std::uniform_int_distribution<int> dist(0, 255);

    mask_key[0] = static_cast<std::uint8_t>(dist(rng));
    mask_key[1] = static_cast<std::uint8_t>(dist(rng));
    mask_key[2] = static_cast<std::uint8_t>(dist(rng));
    mask_key[3] = static_cast<std::uint8_t>(dist(rng));
}
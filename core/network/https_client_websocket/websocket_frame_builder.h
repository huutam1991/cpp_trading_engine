#pragma once

#include <cstdint>
#include <string>
#include <vector>

class WebSocketFrameBuilder
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

public:
    // Generic builder
    static std::vector<char> build_frame(
        Opcode opcode,
        const char* payload,
        std::uint64_t payload_size,
        bool fin = true,
        bool mask = true);

    static std::vector<char> build_frame(
        Opcode opcode,
        const std::string& payload,
        bool fin = true,
        bool mask = true);

    static std::vector<char> build_frame(
        Opcode opcode,
        const std::vector<char>& payload,
        bool fin = true,
        bool mask = true);

    // Convenience helpers
    static std::vector<char> build_text(
        const std::string& text,
        bool fin = true,
        bool mask = true);

    static std::vector<char> build_binary(
        const std::vector<char>& payload,
        bool fin = true,
        bool mask = true);

    static std::vector<char> build_ping(
        const std::string& payload = "",
        bool mask = true);

    static std::vector<char> build_pong(
        const std::string& payload = "",
        bool mask = true);

    static std::vector<char> build_close(
        bool mask = true);

    static std::vector<char> build_close(
        std::uint16_t close_code,
        const std::string& reason = "",
        bool mask = true);

private:
    static void append_payload_length(
        std::vector<char>& out,
        std::uint64_t payload_size,
        bool mask);

    static void append_masking_key(
        std::vector<char>& out,
        std::uint8_t mask_key[4]);

    static void append_payload_masked(
        std::vector<char>& out,
        const char* payload,
        std::uint64_t payload_size,
        const std::uint8_t mask_key[4]);

    static void append_payload_unmasked(
        std::vector<char>& out,
        const char* payload,
        std::uint64_t payload_size);

    static void generate_masking_key(std::uint8_t mask_key[4]);
};
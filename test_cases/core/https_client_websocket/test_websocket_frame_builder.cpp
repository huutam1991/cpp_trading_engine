#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

#include <network/https_client_websocket/websocket_frame_builder.h>
#include <network/https_client_websocket/websocket_frame_parser.h>

namespace
{

using BuilderOpcode = WebSocketFrameBuilder::Opcode;
using ParserOpcode  = WebSocketFrameParser::Opcode;

static std::uint16_t ReadU16BE(const std::vector<char>& data, std::size_t offset)
{
    return (static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset])) << 8) |
           (static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset + 1])));
}

static std::uint64_t ReadU64BE(const std::vector<char>& data, std::size_t offset)
{
    return (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 0])) << 56) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 1])) << 48) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 2])) << 40) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 3])) << 32) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 4])) << 24) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 5])) << 16) |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 6])) << 8)  |
           (static_cast<std::uint64_t>(static_cast<unsigned char>(data[offset + 7])));
}

static WebSocketFrameParser::Frame ParseSingleFrame(const std::vector<char>& raw)
{
    WebSocketFrameParser parser;
    parser.feed(raw.data(), raw.size());
    auto frames = parser.parse_frames();
    EXPECT_EQ(frames.size(), 1u);
    return frames[0];
}

static std::size_t ExpectedHeaderSize(bool masked, std::size_t payload_size)
{
    std::size_t header = 2;

    if (payload_size <= 125)
    {
    }
    else if (payload_size <= 65535)
    {
        header += 2;
    }
    else
    {
        header += 8;
    }

    if (masked)
    {
        header += 4;
    }

    return header;
}

} // namespace

TEST(WebSocketFrameBuilderTest, BuildUnmaskedTextFrameHello)
{
    auto raw = WebSocketFrameBuilder::build_text("hello", true, false);

    ASSERT_GE(raw.size(), 7u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81); // FIN + text
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x05); // unmasked len=5

    std::string payload(raw.begin() + 2, raw.end());
    EXPECT_EQ(payload, "hello");
}

TEST(WebSocketFrameBuilderTest, BuildMaskedTextFrameHelloHasMaskBitAndRoundTrips)
{
    auto raw = WebSocketFrameBuilder::build_text("hello", true, true);

    ASSERT_GE(raw.size(), 11u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);      // FIN + text
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x80, 0x80); // mask bit set
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x7F, 5u);   // len=5

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Text);
    EXPECT_TRUE(frame.fin);
    EXPECT_TRUE(frame.masked);
    EXPECT_EQ(frame.payload_as_string(), "hello");
}

TEST(WebSocketFrameBuilderTest, BuildUnmaskedBinaryFrameRoundTrips)
{
    std::vector<char> payload = {'a', '\x00', '\x7F', '\x01'};

    auto raw = WebSocketFrameBuilder::build_binary(payload, true, false);
    auto frame = ParseSingleFrame(raw);

    EXPECT_EQ(frame.opcode, ParserOpcode::Binary);
    EXPECT_TRUE(frame.fin);
    EXPECT_FALSE(frame.masked);
    ASSERT_EQ(frame.payload.size(), payload.size());
    EXPECT_EQ(frame.payload, payload);
}

TEST(WebSocketFrameBuilderTest, BuildMaskedBinaryFrameRoundTrips)
{
    std::vector<char> payload = {'x', 'y', 'z', '\x00', '\x01'};

    auto raw = WebSocketFrameBuilder::build_binary(payload, true, true);
    auto frame = ParseSingleFrame(raw);

    EXPECT_EQ(frame.opcode, ParserOpcode::Binary);
    EXPECT_TRUE(frame.fin);
    EXPECT_TRUE(frame.masked);
    EXPECT_EQ(frame.payload, payload);
}

TEST(WebSocketFrameBuilderTest, BuildPingEmptyUnmasked)
{
    auto raw = WebSocketFrameBuilder::build_ping("", false);

    ASSERT_EQ(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x89); // FIN + ping
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x00); // len=0

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Ping);
    EXPECT_TRUE(frame.payload.empty());
}

TEST(WebSocketFrameBuilderTest, BuildPongWithPayloadMaskedRoundTrips)
{
    auto raw = WebSocketFrameBuilder::build_pong("pong", true);

    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x8A);        // FIN + pong
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x80, 0x80); // masked

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Pong);
    EXPECT_TRUE(frame.masked);
    EXPECT_EQ(frame.payload_as_string(), "pong");
}

TEST(WebSocketFrameBuilderTest, BuildCloseWithoutPayloadUnmasked)
{
    auto raw = WebSocketFrameBuilder::build_close(false);

    ASSERT_EQ(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x88); // FIN + close
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x00); // len=0

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Close);
    EXPECT_TRUE(frame.payload.empty());
}

TEST(WebSocketFrameBuilderTest, BuildCloseWithCodeAndReasonUnmasked)
{
    auto raw = WebSocketFrameBuilder::build_close(1000, "normal", false);

    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x88);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 8u); // 2 bytes code + 6 bytes "normal"

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Close);
    ASSERT_EQ(frame.payload.size(), 8u);
    EXPECT_EQ(static_cast<unsigned char>(frame.payload[0]), 0x03);
    EXPECT_EQ(static_cast<unsigned char>(frame.payload[1]), 0xE8);
    EXPECT_EQ(std::string(frame.payload.begin() + 2, frame.payload.end()), "normal");
}

TEST(WebSocketFrameBuilderTest, BuildCloseWithCodeAndReasonMaskedRoundTrips)
{
    auto raw = WebSocketFrameBuilder::build_close(1001, "bye", true);

    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x88);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x80, 0x80);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Close);
    EXPECT_TRUE(frame.masked);
    ASSERT_EQ(frame.payload.size(), 5u);
    EXPECT_EQ(static_cast<unsigned char>(frame.payload[0]), 0x03);
    EXPECT_EQ(static_cast<unsigned char>(frame.payload[1]), 0xE9);
    EXPECT_EQ(std::string(frame.payload.begin() + 2, frame.payload.end()), "bye");
}

TEST(WebSocketFrameBuilderTest, BuildZeroLengthTextFrameUnmasked)
{
    auto raw = WebSocketFrameBuilder::build_text("", true, false);

    ASSERT_EQ(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x00);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Text);
    EXPECT_TRUE(frame.payload.empty());
}

TEST(WebSocketFrameBuilderTest, BuildPayloadLengthExactly125Unmasked)
{
    std::string payload(125, 'a');

    auto raw = WebSocketFrameBuilder::build_text(payload, true, false);

    ASSERT_EQ(raw.size(), ExpectedHeaderSize(false, payload.size()) + payload.size());
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 125u);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.payload_as_string(), payload);
}

TEST(WebSocketFrameBuilderTest, BuildPayloadLength126UsesExtended16BitUnmasked)
{
    std::string payload(126, 'b');

    auto raw = WebSocketFrameBuilder::build_text(payload, true, false);

    ASSERT_EQ(raw.size(), ExpectedHeaderSize(false, payload.size()) + payload.size());
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 126u);
    EXPECT_EQ(ReadU16BE(raw, 2), 126u);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.payload_as_string(), payload);
}

TEST(WebSocketFrameBuilderTest, BuildPayloadLength65535UsesExtended16BitUnmasked)
{
    std::string payload(65535, 'c');

    auto raw = WebSocketFrameBuilder::build_text(payload, true, false);

    ASSERT_EQ(raw.size(), ExpectedHeaderSize(false, payload.size()) + payload.size());
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 126u);
    EXPECT_EQ(ReadU16BE(raw, 2), 65535u);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.payload_as_string(), payload);
}

TEST(WebSocketFrameBuilderTest, BuildPayloadLength65536UsesExtended64BitUnmasked)
{
    std::string payload(65536, 'd');

    auto raw = WebSocketFrameBuilder::build_text(payload, true, false);

    ASSERT_EQ(raw.size(), ExpectedHeaderSize(false, payload.size()) + payload.size());
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 127u);
    EXPECT_EQ(ReadU64BE(raw, 2), 65536u);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.payload_as_string(), payload);
}

TEST(WebSocketFrameBuilderTest, BuildMaskedFrameLength126HasCorrectHeaderLayout)
{
    std::string payload(126, 'x');

    auto raw = WebSocketFrameBuilder::build_text(payload, true, true);

    ASSERT_EQ(raw.size(), ExpectedHeaderSize(true, payload.size()) + payload.size());
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x81);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x80, 0x80);
    EXPECT_EQ(static_cast<unsigned char>(raw[1]) & 0x7F, 126u);
    EXPECT_EQ(ReadU16BE(raw, 2), 126u);

    auto frame = ParseSingleFrame(raw);
    EXPECT_TRUE(frame.masked);
    EXPECT_EQ(frame.payload_as_string(), payload);
}

TEST(WebSocketFrameBuilderTest, BuildTextFrameWithFinFalse)
{
    auto raw = WebSocketFrameBuilder::build_text("hello", false, false);

    ASSERT_GE(raw.size(), 7u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x01); // FIN=0, opcode=text
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x05);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Text);
    EXPECT_FALSE(frame.fin);
    EXPECT_EQ(frame.payload_as_string(), "hello");
}

TEST(WebSocketFrameBuilderTest, BuildBinaryFrameWithFinFalse)
{
    std::vector<char> payload = {'a', 'b', 'c'};

    auto raw = WebSocketFrameBuilder::build_binary(payload, false, false);

    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x02); // FIN=0, opcode=binary

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Binary);
    EXPECT_FALSE(frame.fin);
    EXPECT_EQ(frame.payload, payload);
}

TEST(WebSocketFrameBuilderTest, BuildFrameFromVectorEmptyPayload)
{
    std::vector<char> payload;

    auto raw = WebSocketFrameBuilder::build_frame(BuilderOpcode::Binary, payload, true, false);

    ASSERT_EQ(raw.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(raw[0]), 0x82); // FIN + binary
    EXPECT_EQ(static_cast<unsigned char>(raw[1]), 0x00);

    auto frame = ParseSingleFrame(raw);
    EXPECT_EQ(frame.opcode, ParserOpcode::Binary);
    EXPECT_TRUE(frame.payload.empty());
}

TEST(WebSocketFrameBuilderTest, BuildControlFrameWithFinFalseShouldThrow)
{
    EXPECT_THROW(
        (void)WebSocketFrameBuilder::build_frame(
            BuilderOpcode::Ping,
            "x",
            1,
            false,
            false),
        std::runtime_error);
}

TEST(WebSocketFrameBuilderTest, BuildControlFramePayloadGreaterThan125ShouldThrow)
{
    std::string payload(126, 'p');

    EXPECT_THROW(
        (void)WebSocketFrameBuilder::build_ping(payload, false),
        std::runtime_error);
}

TEST(WebSocketFrameBuilderTest, BuildFrameWithNullPayloadAndPositiveSizeShouldThrow)
{
    EXPECT_THROW(
        (void)WebSocketFrameBuilder::build_frame(
            BuilderOpcode::Text,
            static_cast<const char*>(nullptr),
            3,
            true,
            false),
        std::runtime_error);
}
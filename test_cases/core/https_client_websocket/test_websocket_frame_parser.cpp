#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <network/https_client_websocket/websocket_frame_parser.h>

namespace
{

using Opcode = WebSocketFrameParser::Opcode;
using Frame  = WebSocketFrameParser::Frame;

static void Append(WebSocketFrameParser& p, const std::vector<char>& chunk)
{
    if (!chunk.empty())
    {
        p.feed(chunk.data(), chunk.size());
    }
}

static void Append(WebSocketFrameParser& p, const std::string& chunk)
{
    if (!chunk.empty())
    {
        p.feed(chunk.data(), chunk.size());
    }
}

static std::vector<char> MakeFrame(
    bool fin,
    Opcode opcode,
    const std::string& payload,
    bool masked = false,
    std::uint8_t mask0 = 0x12,
    std::uint8_t mask1 = 0x34,
    std::uint8_t mask2 = 0x56,
    std::uint8_t mask3 = 0x78)
{
    std::vector<char> out;

    const std::uint8_t b0 =
        (fin ? 0x80 : 0x00) |
        static_cast<std::uint8_t>(opcode);

    out.push_back(static_cast<char>(b0));

    const std::size_t payload_len = payload.size();
    std::uint8_t b1 = masked ? 0x80 : 0x00;

    if (payload_len <= 125)
    {
        b1 |= static_cast<std::uint8_t>(payload_len);
        out.push_back(static_cast<char>(b1));
    }
    else if (payload_len <= 0xFFFF)
    {
        b1 |= 126;
        out.push_back(static_cast<char>(b1));
        out.push_back(static_cast<char>((payload_len >> 8) & 0xFF));
        out.push_back(static_cast<char>(payload_len & 0xFF));
    }
    else
    {
        b1 |= 127;
        out.push_back(static_cast<char>(b1));

        const std::uint64_t n = static_cast<std::uint64_t>(payload_len);
        for (int i = 7; i >= 0; --i)
        {
            out.push_back(static_cast<char>((n >> (i * 8)) & 0xFF));
        }
    }

    std::uint8_t mask_key[4] = {mask0, mask1, mask2, mask3};

    if (masked)
    {
        out.push_back(static_cast<char>(mask_key[0]));
        out.push_back(static_cast<char>(mask_key[1]));
        out.push_back(static_cast<char>(mask_key[2]));
        out.push_back(static_cast<char>(mask_key[3]));
    }

    for (std::size_t i = 0; i < payload.size(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(payload[i]);
        if (masked)
        {
            ch ^= mask_key[i & 0x3];
        }
        out.push_back(static_cast<char>(ch));
    }

    return out;
}

static std::vector<char> MakeRawFrame(
    const std::vector<char>& bytes)
{
    return bytes;
}

static void ExpectSingleTextFrame(
    WebSocketFrameParser& p,
    const std::string& expected_payload,
    bool expected_masked = false,
    bool expected_fin = true)
{
    auto frames = p.parse_frames();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_EQ(frames[0].payload_as_string(), expected_payload);
    EXPECT_EQ(frames[0].masked, expected_masked);
    EXPECT_EQ(frames[0].fin, expected_fin);
}

} // namespace

TEST(WebSocketFrameParserTest, ParseSingleUnmaskedTextFrame)
{
    WebSocketFrameParser p;
    Append(p, MakeFrame(true, Opcode::Text, "hello", false));

    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_TRUE(frames[0].fin);
    EXPECT_FALSE(frames[0].masked);
    EXPECT_EQ(frames[0].payload_as_string(), "hello");
}

TEST(WebSocketFrameParserTest, ParseSingleMaskedTextFrame)
{
    WebSocketFrameParser p;
    Append(p, MakeFrame(true, Opcode::Text, "hello", true, 0x01, 0x02, 0x03, 0x04));

    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_TRUE(frames[0].fin);
    EXPECT_TRUE(frames[0].masked);
    EXPECT_EQ(frames[0].payload_as_string(), "hello");
}

TEST(WebSocketFrameParserTest, ParseSingleBinaryFrame)
{
    WebSocketFrameParser p;

    std::string payload;
    payload.push_back('\x01');
    payload.push_back('\x02');
    payload.push_back('\x7F');
    payload.push_back('\x00');

    Append(p, MakeFrame(true, Opcode::Binary, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Binary);
    ASSERT_EQ(frames[0].payload.size(), 4u);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[0]), 0x01);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[1]), 0x02);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[2]), 0x7F);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[3]), 0x00);
}

TEST(WebSocketFrameParserTest, ParseTwoFramesInOneBuffer)
{
    WebSocketFrameParser p;

    auto f1 = MakeFrame(true, Opcode::Text, "hello", false);
    auto f2 = MakeFrame(true, Opcode::Text, "world", false);

    std::vector<char> all;
    all.insert(all.end(), f1.begin(), f1.end());
    all.insert(all.end(), f2.begin(), f2.end());

    Append(p, all);
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[0].payload_as_string(), "hello");
    EXPECT_EQ(frames[1].payload_as_string(), "world");
}

TEST(WebSocketFrameParserTest, ParseFrameSplitAcrossTwoFeeds)
{
    WebSocketFrameParser p;

    auto frame = MakeFrame(true, Opcode::Text, "hello", false);

    Append(p, std::vector<char>(frame.begin(), frame.begin() + 2));
    {
        auto frames = p.parse_frames();
        EXPECT_TRUE(frames.empty());
        EXPECT_EQ(p.buffered_size(), 2u);
    }

    Append(p, std::vector<char>(frame.begin() + 2, frame.end()));
    ExpectSingleTextFrame(p, "hello");
}

TEST(WebSocketFrameParserTest, ParseFrameSplitByteByByte)
{
    WebSocketFrameParser p;
    auto frame = MakeFrame(true, Opcode::Text, "hello", false);

    for (std::size_t i = 0; i + 1 < frame.size(); ++i)
    {
        p.feed(&frame[i], 1);
        auto frames = p.parse_frames();
        EXPECT_TRUE(frames.empty());
    }

    p.feed(&frame.back(), 1);
    ExpectSingleTextFrame(p, "hello");
}

TEST(WebSocketFrameParserTest, ParseZeroLengthTextFrame)
{
    WebSocketFrameParser p;
    Append(p, MakeFrame(true, Opcode::Text, "", false));

    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_TRUE(frames[0].payload.empty());
    EXPECT_EQ(frames[0].payload_as_string(), "");
}

TEST(WebSocketFrameParserTest, ParseZeroLengthPingFrame)
{
    WebSocketFrameParser p;
    Append(p, MakeFrame(true, Opcode::Ping, "", false));

    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Ping);
    EXPECT_TRUE(frames[0].payload.empty());
    EXPECT_TRUE(frames[0].is_control_frame());
}

TEST(WebSocketFrameParserTest, ParsePayloadLengthExactly125)
{
    WebSocketFrameParser p;
    std::string payload(125, 'a');

    Append(p, MakeFrame(true, Opcode::Text, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].payload_as_string(), payload);
    EXPECT_EQ(frames[0].payload.size(), 125u);
}

TEST(WebSocketFrameParserTest, ParsePayloadLength126UsingExtended16Bit)
{
    WebSocketFrameParser p;
    std::string payload(126, 'b');

    Append(p, MakeFrame(true, Opcode::Text, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_EQ(frames[0].payload.size(), 126u);
    EXPECT_EQ(frames[0].payload_as_string(), payload);
}

TEST(WebSocketFrameParserTest, ParsePayloadLength65535UsingExtended16Bit)
{
    WebSocketFrameParser p;
    std::string payload(65535, 'c');

    Append(p, MakeFrame(true, Opcode::Binary, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Binary);
    EXPECT_EQ(frames[0].payload.size(), 65535u);
    EXPECT_EQ(frames[0].payload[0], 'c');
    EXPECT_EQ(frames[0].payload.back(), 'c');
}

TEST(WebSocketFrameParserTest, ParsePayloadLength65536UsingExtended64Bit)
{
    WebSocketFrameParser p;
    std::string payload(65536, 'd');

    Append(p, MakeFrame(true, Opcode::Binary, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Binary);
    EXPECT_EQ(frames[0].payload.size(), 65536u);
    EXPECT_EQ(frames[0].payload.front(), 'd');
    EXPECT_EQ(frames[0].payload.back(), 'd');
}

TEST(WebSocketFrameParserTest, ParseMaskedFrameWithExtended16BitPayload)
{
    WebSocketFrameParser p;
    std::string payload(300, 'x');

    Append(p, MakeFrame(true, Opcode::Text, payload, true, 0xAA, 0xBB, 0xCC, 0xDD));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(frames[0].masked);
    EXPECT_EQ(frames[0].opcode, Opcode::Text);
    EXPECT_EQ(frames[0].payload_as_string(), payload);
}

TEST(WebSocketFrameParserTest, ParsePingPongAndCloseFrames)
{
    WebSocketFrameParser p;

    auto ping  = MakeFrame(true, Opcode::Ping,  "pi", false);
    auto pong  = MakeFrame(true, Opcode::Pong,  "po", false);
    auto close = MakeFrame(true, Opcode::Close, "",   false);

    std::vector<char> all;
    all.insert(all.end(), ping.begin(), ping.end());
    all.insert(all.end(), pong.begin(), pong.end());
    all.insert(all.end(), close.begin(), close.end());

    Append(p, all);
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 3u);
    EXPECT_EQ(frames[0].opcode, Opcode::Ping);
    EXPECT_EQ(frames[0].payload_as_string(), "pi");

    EXPECT_EQ(frames[1].opcode, Opcode::Pong);
    EXPECT_EQ(frames[1].payload_as_string(), "po");

    EXPECT_EQ(frames[2].opcode, Opcode::Close);
    EXPECT_TRUE(frames[2].payload.empty());
}

TEST(WebSocketFrameParserTest, IncompleteExtended16BitHeaderShouldWaitForMoreData)
{
    WebSocketFrameParser p;

    std::vector<char> partial;
    partial.push_back(static_cast<char>(0x81)); // FIN + text
    partial.push_back(static_cast<char>(126));  // extended 16-bit length follows
    partial.push_back(static_cast<char>(0x00)); // only one byte of extended len

    Append(p, partial);

    auto frames = p.parse_frames();
    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(p.buffered_size(), partial.size());
}

TEST(WebSocketFrameParserTest, IncompleteExtended64BitHeaderShouldWaitForMoreData)
{
    WebSocketFrameParser p;

    std::vector<char> partial;
    partial.push_back(static_cast<char>(0x81)); // FIN + text
    partial.push_back(static_cast<char>(127));  // extended 64-bit length follows
    partial.push_back(static_cast<char>(0x00));
    partial.push_back(static_cast<char>(0x00));
    partial.push_back(static_cast<char>(0x00)); // still incomplete

    Append(p, partial);

    auto frames = p.parse_frames();
    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(p.buffered_size(), partial.size());
}

TEST(WebSocketFrameParserTest, IncompleteMaskedKeyShouldWaitForMoreData)
{
    WebSocketFrameParser p;

    std::vector<char> partial;
    partial.push_back(static_cast<char>(0x81)); // FIN + text
    partial.push_back(static_cast<char>(0x80 | 5)); // masked, len=5
    partial.push_back(static_cast<char>(0x11));
    partial.push_back(static_cast<char>(0x22)); // only 2 bytes of mask key

    Append(p, partial);

    auto frames = p.parse_frames();
    EXPECT_TRUE(frames.empty());
    EXPECT_EQ(p.buffered_size(), partial.size());
}

TEST(WebSocketFrameParserTest, IncompletePayloadShouldWaitForMoreData)
{
    WebSocketFrameParser p;

    auto frame = MakeFrame(true, Opcode::Text, "hello", false);

    Append(p, std::vector<char>(frame.begin(), frame.begin() + 4));
    {
        auto frames = p.parse_frames();
        EXPECT_TRUE(frames.empty());
    }

    Append(p, std::vector<char>(frame.begin() + 4, frame.end()));
    ExpectSingleTextFrame(p, "hello");
}

TEST(WebSocketFrameParserTest, BufferShouldBeCompactedAfterParsingFrames)
{
    WebSocketFrameParser p;

    for (int i = 0; i < 200; ++i)
    {
        auto f = MakeFrame(true, Opcode::Text, "abc", false);
        Append(p, f);
    }

    auto frames = p.parse_frames();
    ASSERT_EQ(frames.size(), 200u);
    EXPECT_EQ(p.buffered_size(), 0u);

    // Feed one more frame after compaction/clear and ensure parser still works.
    Append(p, MakeFrame(true, Opcode::Text, "tail", false));
    ExpectSingleTextFrame(p, "tail");
}

TEST(WebSocketFrameParserTest, ClearShouldDiscardBufferedPartialFrame)
{
    WebSocketFrameParser p;

    auto frame = MakeFrame(true, Opcode::Text, "hello", false);
    Append(p, std::vector<char>(frame.begin(), frame.begin() + 3));

    EXPECT_GT(p.buffered_size(), 0u);

    p.clear();
    EXPECT_EQ(p.buffered_size(), 0u);

    auto frames = p.parse_frames();
    EXPECT_TRUE(frames.empty());

    Append(p, frame);
    ExpectSingleTextFrame(p, "hello");
}

TEST(WebSocketFrameParserTest, ParseCloseFrameWithStatusCodeAndReasonPayload)
{
    WebSocketFrameParser p;

    std::string payload;
    payload.push_back(static_cast<char>(0x03)); // 1000 = 0x03E8
    payload.push_back(static_cast<char>(0xE8));
    payload += "normal";

    Append(p, MakeFrame(true, Opcode::Close, payload, false));
    auto frames = p.parse_frames();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, Opcode::Close);
    ASSERT_EQ(frames[0].payload.size(), 8u);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[0]), 0x03);
    EXPECT_EQ(static_cast<unsigned char>(frames[0].payload[1]), 0xE8);
    EXPECT_EQ(std::string(frames[0].payload.begin() + 2, frames[0].payload.end()), "normal");
}
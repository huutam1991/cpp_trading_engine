#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <algorithm>

#include <network/https_client_request/https_client_response_parser.h>

namespace {

static std::string HexSize(std::size_t n, bool upper = false)
{
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (n == 0) return "0";
    std::string s;
    while (n > 0) {
        int d = static_cast<int>(n & 0xF);
        s.push_back(digits[d]);
        n >>= 4;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

static std::string MakeChunkedEncoding(
    const std::vector<std::string>& chunks,
    bool upper_hex = false,
    bool include_extensions = false,
    const std::vector<std::pair<std::string, std::string>>& trailers = {})
{
    std::string body;
    for (std::size_t i = 0; i < chunks.size(); ++i) {
        std::string sz = HexSize(chunks[i].size(), upper_hex);
        if (include_extensions) {
            // Chunk extensions are allowed by RFC; parser should ignore after ';'
            sz += ";ext=1";
        }
        body += sz;
        body += "\r\n";
        body += chunks[i];
        body += "\r\n";
    }

    body += "0\r\n";
    for (auto& kv : trailers) {
        body += kv.first;
        body += ": ";
        body += kv.second;
        body += "\r\n";
    }
    body += "\r\n";
    return body;
}

static std::string MakeChunkedResponse(
    int status_code,
    const std::string& reason,
    const std::vector<std::pair<std::string, std::string>>& headers,
    const std::vector<std::string>& chunks,
    const std::vector<std::pair<std::string, std::string>>& trailers = {},
    const std::string& http_version = "HTTP/1.1",
    bool upper_hex = false,
    bool include_extensions = false)
{
    std::string s;
    s += http_version + " " + std::to_string(status_code) + " " + reason + "\r\n";

    bool has_te = false;
    for (auto& kv : headers) {
        if (kv.first == "Transfer-Encoding" || kv.first == "transfer-encoding") {
            has_te = true;
        }
        s += kv.first + ": " + kv.second + "\r\n";
    }

    if (!has_te) {
        s += "Transfer-Encoding: chunked\r\n";
    }

    s += "\r\n";
    s += MakeChunkedEncoding(chunks, upper_hex, include_extensions, trailers);
    return s;
}

static void Append(HttpsClientResponseParser& p, const std::string& chunk)
{
    p.append_data(chunk.data(), static_cast<std::uint32_t>(chunk.size()));
}

static std::string Join(const std::vector<std::string>& parts)
{
    std::string out;
    for (auto& s : parts) out += s;
    return out;
}

} // namespace

TEST(HttpsClientResponseParserChunkedTest, SingleFullChunkedResponseInOneBuffer)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"A", "BB", "CCC"};
    const std::string expected = Join(chunks);

    const std::string raw = MakeChunkedResponse(
        200, "OK",
        {{"Content-Type", "text/plain"}},
        chunks
    );

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].status_code, 200);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, HeaderSplitAcrossTwoBuffers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"hello", "world"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X-A", "1"}}, chunks);

    const std::size_t cut = raw.find("\r\n\r\n") - 1; // inside header
    ASSERT_NE(cut, std::string::npos);

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, HeaderDelimiterSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"abc", "def"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X", "Y"}}, chunks);

    const std::size_t hdr_end = raw.find("\r\n\r\n");
    ASSERT_NE(hdr_end, std::string::npos);

    // split inside the delimiter
    const std::size_t cut = hdr_end + 3;

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, ChunkSizeLineSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    // first chunk size is "5\r\n"
    const std::vector<std::string> chunks = {"HELLO", "XYZ"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X", "1"}}, chunks);

    const std::size_t body_start = raw.find("\r\n\r\n");
    ASSERT_NE(body_start, std::string::npos);
    const std::size_t first_size_pos = body_start + 4;

    // Cut after the '5' but before "\r\n"
    const std::size_t cut = first_size_pos + 1;

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, ChunkDataSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"0123456789", "ABC"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X", "1"}}, chunks);

    const std::size_t hdr_end = raw.find("\r\n\r\n");
    ASSERT_NE(hdr_end, std::string::npos);

    // Body begins: "a\r\n0123456789\r\n..."; split inside the first chunk payload.
    const std::size_t payload_pos = raw.find("0123456789");
    ASSERT_NE(payload_pos, std::string::npos);
    const std::size_t cut = payload_pos + 4;

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, ChunkDataCRLFSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"DATA", "MORE"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X", "1"}}, chunks);

    // find the "DATA\r\n" ending and split between '\r' and '\n'
    const std::string marker = "DATA\r\n";
    const std::size_t m = raw.find(marker);
    ASSERT_NE(m, std::string::npos);
    const std::size_t cut = m + 4 + 1; // after 'DATA' and '\r'

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, FinalZeroChunkSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"A", "B"};
    const std::string expected = Join(chunks);
    const std::string raw = MakeChunkedResponse(200, "OK", {{"X", "1"}}, chunks);

    // Find the terminating "0\r\n\r\n" and split between '0' and "\r\n"
    const std::size_t term = raw.rfind("0\r\n\r\n");
    ASSERT_NE(term, std::string::npos);

    const std::size_t cut = term + 1; // after '0'

    Append(p, raw.substr(0, cut));
    EXPECT_TRUE(p.parse_all().empty());

    Append(p, raw.substr(cut));
    auto resps = p.parse_all();
    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, ChunkedWithTrailers)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"abc", "def"};
    const std::string expected = Join(chunks);

    const std::string raw = MakeChunkedResponse(
        200, "OK",
        {{"X", "1"}},
        chunks,
        {{"X-Trailer", "t"}, {"Another", "v"}}
    );

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, TransferEncodingMultipleTokensAndUpperHexAndExtensions)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"HELLO", "WORLD"};
    const std::string expected = Join(chunks);

    const std::string raw = MakeChunkedResponse(
        200, "OK",
        {
            {"Transfer-Encoding", "gzip, Chunked"},
            {"Content-Type", "text/plain"}
        },
        chunks,
        /*trailers*/{},
        /*http_version*/"HTTP/1.1",
        /*upper_hex*/true,
        /*include_extensions*/true
    );

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, ChunkedShouldIgnoreContentLengthIfPresent)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"12345", "678"};
    const std::string expected = Join(chunks);

    // Intentionally lie about Content-Length: 0
    const std::string raw = MakeChunkedResponse(
        200, "OK",
        {
            {"Content-Length", "0"},
            {"Transfer-Encoding", "chunked"},
            {"X", "1"}
        },
        chunks
    );

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, expected);
}

TEST(HttpsClientResponseParserChunkedTest, TwoResponsesChunkedThenNormalInOneBuffer)
{
    HttpsClientResponseParser p;

    const std::vector<std::string> chunks = {"AA", "BB"};
    const std::string expected1 = Join(chunks);

    const std::string r1 = MakeChunkedResponse(200, "OK", {{"X-Id", "1"}}, chunks);

    // Normal response right after
    const std::string body2 = "ZZZ";
    std::string r2;
    r2 += "HTTP/1.1 201 Created\r\n";
    r2 += "Content-Length: 3\r\n";
    r2 += "X-Id: 2\r\n";
    r2 += "\r\n";
    r2 += body2;

    Append(p, r1 + r2);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 2u);
    EXPECT_EQ(resps[0].status_code, 200);
    EXPECT_EQ(resps[0].body, expected1);

    EXPECT_EQ(resps[1].status_code, 201);
    EXPECT_EQ(resps[1].body, body2);
}

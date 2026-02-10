#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdint>

// Include your parser + response types
#include <network/https_client_request/https_client_response_parser.h>   // adjust include path
// #include "https_client_response.h"       // if needed

namespace {

static std::string MakeHttpResponse(
    int status_code,
    const std::string& reason,
    const std::vector<std::pair<std::string, std::string>>& headers,
    const std::string& body,
    bool include_content_length = true,
    const std::string& http_version = "HTTP/1.1")
{
    std::string s;
    s += http_version + " " + std::to_string(status_code) + " " + reason + "\r\n";

    bool has_content_length = false;
    for (auto& kv : headers) {
        if (kv.first == "Content-Length" || kv.first == "content-length") {
            has_content_length = true;
        }
        s += kv.first + ": " + kv.second + "\r\n";
    }

    if (include_content_length && !has_content_length) {
        s += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }

    s += "\r\n";
    s += body;
    return s;
}

static void Append(HttpsClientResponseParser& p, const std::string& chunk)
{
    p.append_data(chunk.data(), static_cast<std::uint32_t>(chunk.size()));
}

} // namespace

TEST(HttpsClientResponseParserTest, SingleFullResponseInOneBuffer)
{
    HttpsClientResponseParser p;

    const std::string body = "hello";
    const std::string raw = MakeHttpResponse(
        200, "OK",
        {{"Content-Type", "text/plain"}},
        body
    );

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].status_code, 200);
    EXPECT_EQ(resps[0].body, body);
    // Header checks (adapt to your header storage)
    EXPECT_TRUE(resps[0].headers.find("Content-Type") != resps[0].headers.end());
}

TEST(HttpsClientResponseParserTest, HeaderSplitAcrossTwoBuffers)
{
    HttpsClientResponseParser p;

    const std::string body = "ABCDE";
    const std::string raw = MakeHttpResponse(200, "OK", {{"X-A", "1"}}, body);

    // Split somewhere in header
    const std::size_t cut = raw.find("\r\n\r\n") - 2; // within header
    ASSERT_NE(cut, std::string::npos);

    Append(p, raw.substr(0, cut));
    auto r0 = p.parse_all();
    EXPECT_TRUE(r0.empty()); // not enough to parse

    Append(p, raw.substr(cut));
    auto r1 = p.parse_all();

    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0].status_code, 200);
    EXPECT_EQ(r1[0].body, body);
}

TEST(HttpsClientResponseParserTest, HeaderDelimiterSplitAcrossBuffers)
{
    HttpsClientResponseParser p;

    const std::string body = "XYZ";
    const std::string raw = MakeHttpResponse(200, "OK", {{"X-Test", "yes"}}, body);

    // Split exactly inside "\r\n\r\n"
    const std::size_t hdr_end = raw.find("\r\n\r\n");
    ASSERT_NE(hdr_end, std::string::npos);

    // Put "\r\n\r" in first, "\n" in second
    const std::size_t cut = hdr_end + 3;
    Append(p, raw.substr(0, cut));
    auto r0 = p.parse_all();
    EXPECT_TRUE(r0.empty());

    Append(p, raw.substr(cut));
    auto r1 = p.parse_all();

    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0].body, body);
}

TEST(HttpsClientResponseParserTest, BodySplitAcrossTwoBuffers)
{
    HttpsClientResponseParser p;

    const std::string body = "0123456789";
    const std::string raw = MakeHttpResponse(200, "OK", {{"X", "Y"}}, body);

    // Split after headers + a few body bytes
    const std::size_t hdr_bytes = raw.find("\r\n\r\n") + 4;
    ASSERT_TRUE(hdr_bytes >= 4);
    const std::size_t cut = hdr_bytes + 3; // partial body
    ASSERT_LT(cut, raw.size());

    Append(p, raw.substr(0, cut));
    auto r0 = p.parse_all();
    EXPECT_TRUE(r0.empty()); // body not enough yet

    Append(p, raw.substr(cut));
    auto r1 = p.parse_all();
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0].body, body);
}

TEST(HttpsClientResponseParserTest, TwoFullResponsesInOneBuffer)
{
    HttpsClientResponseParser p;

    const std::string r1 = MakeHttpResponse(200, "OK", {{"X-Id", "1"}}, "AAA");
    const std::string r2 = MakeHttpResponse(201, "Created", {{"X-Id", "2"}}, "BBB");
    const std::string raw = r1 + r2;

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 2u);
    EXPECT_EQ(resps[0].status_code, 200);
    EXPECT_EQ(resps[0].body, "AAA");
    EXPECT_EQ(resps[1].status_code, 201);
    EXPECT_EQ(resps[1].body, "BBB");
}

TEST(HttpsClientResponseParserTest, SecondResponsePartialShouldBeKeptForNextBuffer)
{
    HttpsClientResponseParser p;

    const std::string r1 = MakeHttpResponse(200, "OK", {{"X-Id", "1"}}, "AAA");
    const std::string r2 = MakeHttpResponse(200, "OK", {{"X-Id", "2"}}, "BBBBBBBB"); // longer body

    // Feed full r1 + only part of r2
    const std::size_t partial = r2.size() - 3;
    Append(p, r1 + r2.substr(0, partial));

    auto a = p.parse_all();
    ASSERT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0].body, "AAA");

    // Now feed the rest of r2
    Append(p, r2.substr(partial));
    auto b = p.parse_all();

    ASSERT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0].body, "BBBBBBBB");
}

TEST(HttpsClientResponseParserTest, ContentLengthZeroShouldParseImmediately)
{
    HttpsClientResponseParser p;

    const std::string raw =
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "X-A: 1\r\n"
        "\r\n";

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].status_code, 204);
    EXPECT_TRUE(resps[0].body.empty());
}

TEST(HttpsClientResponseParserTest, HeaderCaseVariationsForContentLength)
{
    HttpsClientResponseParser p;

    const std::string body = "abc";
    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "content-length: 3\r\n"
        "X-Test: v\r\n"
        "\r\n"
        "abc";

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, body);
}

TEST(HttpsClientResponseParserTest, HeaderWhitespaceAroundColon)
{
    HttpsClientResponseParser p;

    const std::string body = "abcde";
    std::string raw =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length:    5\r\n"
        "X-Test:    v\r\n"
        "\r\n"
        "abcde";

    Append(p, raw);
    auto resps = p.parse_all();

    ASSERT_EQ(resps.size(), 1u);
    EXPECT_EQ(resps[0].body, body);
}

TEST(HttpsClientResponseParserTest, MalformedHeaderShouldNotCrashAndShouldNotEmitResponse)
{
    HttpsClientResponseParser p;

    // Missing status line format or missing \r\n\r\n
    const std::string raw = "NOT_HTTP\r\nX: 1\r\n";
    Append(p, raw);

    // Expected: should not crash; most parsers return empty and keep buffer.
    // If your parser instead clears buffer on error, adapt this test.
    auto resps = p.parse_all();
    EXPECT_TRUE(resps.empty());
}

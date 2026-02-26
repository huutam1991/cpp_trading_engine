#pragma once

#include <string>
#include <sstream>
#include <strings.h>
#include <unordered_map>
#include <vector>
#include <cctype>

struct HttpsClientResponse
{
    int status_code = 0;
    std::string status_message;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    bool is_complete = false;

    void reset()
    {
        status_code = 0;
        status_message.clear();
        headers.clear();
        body.clear();
        is_complete = false;
    }

    static HttpsClientResponse create_error_response()
    {
        return HttpsClientResponse {-1, "Disconnected", {}, "", false};
    }
};

class HttpsClientResponseParser
{
    HttpsClientResponse m_current_response;

public:
    HttpsClientResponseParser() = default;

    // append new received data
    void append_data(const char* data, size_t len);

    // parse ALL available responses inside m_buffer
    // return vector of complete responses
    std::vector<HttpsClientResponse> parse_all();

    // Helper to reset state for the next response
    void reset_state();

    bool is_header_parsed() const { return m_header_parsed; }
    int  get_content_length() const { return m_content_length; }
    bool is_chunked() const { return m_is_chunked; }

private:
    std::string m_buffer;
    bool m_header_parsed = false;
    int  m_content_length = 0;

    bool   m_is_chunked = false;

    enum class ChunkState
    {
        ReadSizeLine,      // read "<hex>\r\n"
        ReadData,          // read chunk payload
        ReadDataCRLF,      // read "\r\n" after payload
        ReadTrailers,      // read trailers until "\r\n\r\n" (or at least "\r\n")
        Done
    };

    enum class ChunkParseResult
    {
        Done,
        NeedMoreData,
        Error
    };

    ChunkState m_chunk_state = ChunkState::ReadSizeLine;
    size_t     m_chunk_bytes_remaining = 0;

private:
    bool parse_header(const std::string& header_block, HttpsClientResponse& resp);
    bool parse_status_line(const std::string& line, HttpsClientResponse& resp);

    // -------- Chunked parsing --------
    ChunkParseResult parse_chunked_body();

    // String support methods
    static bool parse_hex_size(const std::string& s, size_t& out);
    static std::string trim(const std::string& s);
    static std::string to_lower(std::string s);
};

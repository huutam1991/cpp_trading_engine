#pragma once

#include <string>
#include <sstream>
#include <strings.h>
#include <unordered_map>
#include <vector>
#include <cctype>

#include "https_client_response.h"
#include "https_client_chunk_parser.h"

class HttpsClientResponseParser
{
    HttpsClientResponse m_current_response;
    HttpsClientChunkParser m_chunk_parser;

public:
    HttpsClientResponseParser() : m_chunk_parser(m_buffer, m_current_response)
    {}

    // append new received data
    void append_data(const char* data, size_t len);
    void append_data(std::string data);

    // get leftover data that has not been parsed into a complete response (e.g. partial header/body for the next response)
    std::string get_leftover_data() const
    {
        return m_buffer;
    }

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

private:
    bool parse_header(const std::string& header_block, HttpsClientResponse& resp);
    bool parse_status_line(const std::string& line, HttpsClientResponse& resp);
};

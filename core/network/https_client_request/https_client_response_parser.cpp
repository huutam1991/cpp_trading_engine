#include "https_client_response_parser.h"

void HttpsClientResponseParser::append_data(const char* data, size_t len)
{
    m_buffer.append(data, len);
}

std::vector<HttpsClientResponse> HttpsClientResponseParser::parse_all()
{
    std::vector<HttpsClientResponse> results;

    while (true)
    {
        // 1) Parse header (once)
        if (!m_header_parsed)
        {
            size_t pos = m_buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
            {
                // Not enough header data
                break;
            }

            std::string header_block = m_buffer.substr(0, pos);

            if (!parse_header(header_block, m_current_response))
            {
                // Parse failed -> do not remove buffer
                break;
            }

            // remove header
            m_buffer.erase(0, pos + 4);
            m_header_parsed = true;

            // If chunked -> body parsing handled by parse_chunked_body()
            if (m_is_chunked)
            {
                // continue to body parsing section below
            }
            else
            {
                // If Content-Length = 0 -> complete response immediately
                if (m_content_length == 0)
                {
                    m_current_response.is_complete = true;
                    results.push_back(m_current_response);
                    reset_state();
                    continue;
                }
            }
        }

        // 2) Parse body
        if (m_header_parsed)
        {
            if (m_is_chunked)
            {
                ChunkParseResult r = parse_chunked_body();
                if (r == ChunkParseResult::NeedMoreData)
                {
                    break;
                }
                if (r == ChunkParseResult::Error)
                {
                    // parsing error: stop (you can also choose to reset_state() + push error)
                    break;
                }

                // Done
                m_current_response.is_complete = true;
                results.push_back(m_current_response);
                reset_state();
                continue;
            }
            else
            {
                // Content-Length path (original logic)
                if (m_buffer.size() < (size_t)m_content_length)
                {
                    break; // need more data
                }

                m_current_response.body = m_buffer.substr(0, m_content_length);
                m_current_response.is_complete = true;

                m_buffer.erase(0, m_content_length);

                results.push_back(m_current_response);
                reset_state();
                continue;
            }
        }

        break; // default exit
    }

    return results;
}

void HttpsClientResponseParser::reset_state()
{
    m_header_parsed = false;
    m_content_length = 0;
    m_is_chunked = false;

    // reset chunk parser state
    m_chunk_state = ChunkState::ReadSizeLine;
    m_chunk_bytes_remaining = 0;

    m_current_response.reset();
}

bool HttpsClientResponseParser::parse_header(const std::string& header_block, HttpsClientResponse& resp)
{
    std::stringstream ss(header_block);
    std::string line;

    // status line
    if (!std::getline(ss, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!parse_status_line(line, resp)) return false;

    // headers
    while (std::getline(ss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        size_t pos = line.find(':');
        if (pos == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        resp.headers[key] = value;

        if (strcasecmp(key.c_str(), "Content-Length") == 0)
        {
            // Only meaningful if not chunked, but harmless to parse.
            m_content_length = std::stoi(value);
        }

        if (strcasecmp(key.c_str(), "Transfer-Encoding") == 0)
        {
            // detect "chunked" token (case-insensitive)
            // value can be: "chunked" or "gzip, chunked" etc.
            std::string v = to_lower(value);
            if (v.find("chunked") != std::string::npos)
            {
                m_is_chunked = true;
                // In chunked mode, Content-Length MUST be ignored.
                m_content_length = 0;
            }
        }
    }

    return true;
}

bool HttpsClientResponseParser::parse_status_line(const std::string& line, HttpsClientResponse& resp)
{
    // Expected: HTTP/1.1 200 OK
    std::stringstream ss(line);
    std::string http_ver;
    ss >> http_ver;
    ss >> resp.status_code;
    std::getline(ss, resp.status_message);
    if (!resp.status_message.empty() && resp.status_message[0] == ' ')
        resp.status_message.erase(0, 1);
    return true;
}

HttpsClientResponseParser::ChunkParseResult HttpsClientResponseParser::parse_chunked_body()
{
    while (true)
    {
        if (m_chunk_state == ChunkState::ReadSizeLine)
        {
            size_t rn = m_buffer.find("\r\n");
            if (rn == std::string::npos)
                return ChunkParseResult::NeedMoreData;

            std::string size_line = m_buffer.substr(0, rn);
            m_buffer.erase(0, rn + 2);

            // size_line may contain extensions: "1e8;ext=1"
            size_t semi = size_line.find(';');
            if (semi != std::string::npos)
                size_line = size_line.substr(0, semi);

            size_line = trim(size_line);
            if (size_line.empty())
                return ChunkParseResult::Error;

            // parse hex
            size_t chunk_sz = 0;
            if (!parse_hex_size(size_line, chunk_sz))
                return ChunkParseResult::Error;

            m_chunk_bytes_remaining = chunk_sz;

            if (m_chunk_bytes_remaining == 0)
            {
                m_chunk_state = ChunkState::ReadTrailers;
            }
            else
            {
                m_chunk_state = ChunkState::ReadData;
            }
        }
        else if (m_chunk_state == ChunkState::ReadData)
        {
            if (m_buffer.size() < m_chunk_bytes_remaining)
                return ChunkParseResult::NeedMoreData;

            // append chunk payload to body
            m_current_response.body.append(m_buffer.data(), m_chunk_bytes_remaining);
            m_buffer.erase(0, m_chunk_bytes_remaining);

            m_chunk_state = ChunkState::ReadDataCRLF;
        }
        else if (m_chunk_state == ChunkState::ReadDataCRLF)
        {
            if (m_buffer.size() < 2)
                return ChunkParseResult::NeedMoreData;

            if (!(m_buffer[0] == '\r' && m_buffer[1] == '\n'))
                return ChunkParseResult::Error;

            m_buffer.erase(0, 2);

            // next chunk
            m_chunk_state = ChunkState::ReadSizeLine;
            m_chunk_bytes_remaining = 0;
        }
        else if (m_chunk_state == ChunkState::ReadTrailers)
        {
            // RFC7230: after the 0-size chunk line, there is:
            //   trailer-part = *( header-field CRLF )
            //   CRLF
            //
            // Most common: empty trailers => immediate "\r\n"
            if (m_buffer.size() < 2)
                return ChunkParseResult::NeedMoreData;

            // Empty trailers: MUST consume exactly 1 CRLF and finish.
            // IMPORTANT: do NOT search for "\r\n\r\n" here, or you may
            // accidentally consume the next response's header delimiter.
            if (m_buffer[0] == '\r' && m_buffer[1] == '\n')
            {
                m_buffer.erase(0, 2);
                m_chunk_state = ChunkState::Done;
                return ChunkParseResult::Done;
            }

            // Non-empty trailers: end with "\r\n\r\n"
            size_t end = m_buffer.find("\r\n\r\n");
            if (end == std::string::npos)
                return ChunkParseResult::NeedMoreData;

            m_buffer.erase(0, end + 4);
            m_chunk_state = ChunkState::Done;
            return ChunkParseResult::Done;
        }
        else if (m_chunk_state == ChunkState::Done)
        {
            return ChunkParseResult::Done;
        }
    }
}
bool HttpsClientResponseParser::parse_hex_size(const std::string& s, size_t& out)
{
    // strict-ish hex parse
    size_t v = 0;
    for (char c : s)
    {
        int d = -1;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
        else return false;

        v = (v << 4) + (size_t)d;
    }
    out = v;
    return true;
}

std::string HttpsClientResponseParser::trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t");
    size_t end   = s.find_last_not_of(" \t");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::string HttpsClientResponseParser::to_lower(std::string s)
{
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
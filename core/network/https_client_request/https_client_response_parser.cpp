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
                HttpsClientChunkParser::ChunkParseResult r = m_chunk_parser.parse_chunked_body();
                if (r == HttpsClientChunkParser::ChunkParseResult::NeedMoreData)
                {
                    break;
                }
                if (r == HttpsClientChunkParser::ChunkParseResult::Error)
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
    m_chunk_parser.reset();
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

        std::string key = HttpsClientResponse::trim(line.substr(0, pos));
        std::string value = HttpsClientResponse::trim(line.substr(pos + 1));
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
            std::string v = HttpsClientResponse::to_lower(value);
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

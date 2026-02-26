#include "https_client_chunk_parser.h"

#include <spdlog/spdlog.h>

HttpsClientChunkParser::HttpsClientChunkParser(std::string& buffer, HttpsClientResponse& current_response) : m_buffer(buffer), m_current_response(current_response)
{}

HttpsClientChunkParser::ChunkParseResult HttpsClientChunkParser::parse_chunked_body()
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

            size_line = HttpsClientResponse::trim(size_line);
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

void HttpsClientChunkParser::reset()
{
    m_chunk_state = ChunkState::ReadSizeLine;
    m_chunk_bytes_remaining = 0;
}

bool HttpsClientChunkParser::parse_hex_size(const std::string& s, size_t& out)
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


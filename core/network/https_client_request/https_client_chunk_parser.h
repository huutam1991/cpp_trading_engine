#pragma once

#include <cstddef>

#include "https_client_response.h"

class HttpsClientChunkParser
{
public:
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

private:
    std::string& m_buffer;
    HttpsClientResponse& m_current_response;

    ChunkState m_chunk_state = ChunkState::ReadSizeLine;
    size_t     m_chunk_bytes_remaining = 0;

    // String support methods
    static inline bool parse_hex_size(const std::string& s, size_t& out);

public:
    HttpsClientChunkParser(std::string& buffer, HttpsClientResponse& current_response);
    HttpsClientChunkParser() = delete;

    // -------- Chunked parsing --------
    ChunkParseResult parse_chunked_body();

    void reset();
};
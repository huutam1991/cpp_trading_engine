#pragma once

#include "https_client_response.h"

class HttpsClientResponseParser
{
public:
    HttpsClientResponseParser() {}

    // append new received data
    void append_data(const char* data, size_t len)
    {
        buffer.append(data, len);
    }

    // parse ALL available responses inside buffer
    // return vector of complete responses
    std::vector<HttpsClientResponse> parse_all()
    {
        std::vector<HttpsClientResponse> results;

        while (true)
        {
            HttpsClientResponse resp;

            // 1. Parse header
            if (!header_parsed)
            {
                size_t pos = buffer.find("\r\n\r\n");
                if (pos == std::string::npos)
                {
                    // Not enough header data
                    break;
                }

                // Extract header block
                std::string header_block = buffer.substr(0, pos);

                // Parse header
                if (!parse_header(header_block, resp))
                {
                    // Parse failed → break (do not remove buffer)
                    break;
                }

                // Remove header from buffer
                buffer.erase(0, pos + 4);
                header_parsed = true;

                // If Content-Length = 0 → complete response
                if (content_length == 0)
                {
                    resp.is_complete = true;
                    results.push_back(resp);

                    // Reset for next response
                    reset_state();
                    continue;   // Try parse next response
                }
            }

            // 2. Parse body
            if (header_parsed)
            {
                if (buffer.size() < (size_t)content_length)
                {
                    // Body not complete → need more data
                    break;
                }

                // Extract body
                resp.body = buffer.substr(0, content_length);
                resp.is_complete = true;

                // Remove body from buffer
                buffer.erase(0, content_length);

                // Save response
                results.push_back(resp);

                // Reset for next response
                reset_state();

                // Continue loop → try parse next response in remaining buffer
                continue;
            }

            break; // default exit
        }

        return results;
    }

    // Helper to reset state for the next response
    void reset_state()
    {
        header_parsed = false;
        content_length = 0;
    }

    bool is_header_parsed() const { return header_parsed; }
    int get_content_length() const { return content_length; }

private:
    std::string buffer;
    bool header_parsed = false;
    int content_length = 0;

private:
    bool parse_header(const std::string& header_block, HttpsClientResponse& resp)
    {
        std::stringstream ss(header_block);
        std::string line;

        // Parse status line
        if (!std::getline(ss, line)) return false;
        if (line.back() == '\r') line.pop_back();

        if (!parse_status_line(line, resp)) return false;

        // Parse header lines
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (line.empty())
            {
                continue;
            }

            size_t pos = line.find(':');
            if (pos == std::string::npos)
            {
                continue;
            }

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            resp.headers[key] = value;

            if (strcasecmp(key.c_str(), "Content-Length") == 0)
            {
                content_length = std::stoi(value);
            }
        }

        return true;
    }

    bool parse_status_line(const std::string& line, HttpsClientResponse& resp)
    {
        // Expected: HTTP/1.1 200 OK
        std::stringstream ss(line);
        std::string http_ver;
        ss >> http_ver;    // HTTP/1.1
        ss >> resp.status_code;
        std::getline(ss, resp.status_message);
        if (!resp.status_message.empty() && resp.status_message[0] == ' ')
        {
            resp.status_message.erase(0, 1);
        }
        return true;
    }

    static std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        size_t end   = s.find_last_not_of(" \t");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
};
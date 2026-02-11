#pragma once

#include <string>
#include <sstream>
#include <strings.h>
#include <unordered_map>

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
    HttpsClientResponseParser() {}

    // append new received data
    void append_data(const char* data, size_t len)
    {
        m_buffer.append(data, len);
    }

    // parse ALL available responses inside m_buffer
    // return vector of complete responses
    std::vector<HttpsClientResponse> parse_all()
    {
        std::vector<HttpsClientResponse> results;

        while (true)
        {
            // 1. Parse header
            if (!m_header_parsed)
            {
                size_t pos = m_buffer.find("\r\n\r\n");
                if (pos == std::string::npos)
                {
                    // Not enough header data
                    break;
                }

                // Extract header block
                std::string header_block = m_buffer.substr(0, pos);

                // Parse header
                if (!parse_header(header_block, m_current_response))
                {
                    // Parse failed -> break (do not remove m_buffer)
                    break;
                }

                // Remove header from m_buffer
                m_buffer.erase(0, pos + 4);
                m_header_parsed = true;

                // If Content-Length = 0 -> complete response
                if (m_content_length == 0)
                {
                    m_current_response.is_complete = true;
                    results.push_back(m_current_response);

                    // Reset for next response
                    reset_state();
                    continue;   // Try parse next response
                }
            }

            // 2. Parse body
            if (m_header_parsed)
            {
                if (m_buffer.size() < (size_t)m_content_length)
                {
                    // Body not complete -> need more data
                    break;
                }

                // Extract body
                m_current_response.body = m_buffer.substr(0, m_content_length);
                m_current_response.is_complete = true;

                // Remove body from m_buffer
                m_buffer.erase(0, m_content_length);

                // Save response
                results.push_back(m_current_response);

                // Reset for next response
                reset_state();

                // Continue loop -> try parse next response in remaining m_buffer
                continue;
            }

            break; // default exit
        }

        return results;
    }

    // Helper to reset state for the next response
    void reset_state()
    {
        m_header_parsed = false;
        m_content_length = 0;
        m_current_response.reset();
    }

    bool is_header_parsed() const { return m_header_parsed; }
    int get_content_length() const { return m_content_length; }

private:
    std::string m_buffer;
    bool m_header_parsed = false;
    int m_content_length = 0;

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

            if (strcasecmp(key.c_str(), "Content-Length") == 0 || strcasecmp(key.c_str(), "content-length") == 0)
            {
                m_content_length = std::stoi(value);
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
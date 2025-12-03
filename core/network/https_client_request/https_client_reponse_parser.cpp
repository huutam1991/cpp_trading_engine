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

    // parse available data
    // return:
    // - false: error
    // - true: still processing or complete
    bool parse(HttpsClientResponse& resp)
    {
        // If header not parsed yet
        if (!header_parsed)
        {
            size_t pos = buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
            {
                return true;
            }

            std::string header_block = buffer.substr(0, pos);
            if (!parse_header(header_block, resp))
            {
                return false;
            }

            buffer.erase(0, pos + 4);
            header_parsed = true;

            // If Content-Length = 0 -> complete
            if (content_length == 0)
            {
                resp.body = "";
                resp.is_complete = true;
                return true;
            }
        }

        // Body parse
        if (header_parsed)
        {
            if (buffer.size() >= (size_t)content_length)
            {
                resp.body = buffer.substr(0, content_length);
                resp.is_complete = true;
                return true;
            }
        }

        return true; // body not complete, continue waiting
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
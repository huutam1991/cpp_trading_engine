#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <iostream>

#include <external_request/external_request.h>
#include <util_macros.h>

ExternalRequest::ExternalRequest(const std::string& url, int port, const std::string& path, RequestMethod request_method):
    m_url(url), m_port(port), m_path(path), m_request_method(request_method)
{
}

std::string ExternalRequest::send_request(bool wait_response)
{
    char buffer[BUFFER_SIZE];
    std::string res;
    std::string request = m_request_method == RequestMethod::POST ? "POST" : "GET";
    request += " " + m_path + " HTTP/1.1\r\nHost: " + m_url + "\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";

    ADD_LOG("request = " << request);

    try
    {
        int socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct hostent *host = gethostbyname(m_url.c_str());

        sockaddr_in addr;
        addr.sin_port = htons(m_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

        if (connect(socket_id, (sockaddr*)(&addr), sizeof(addr)) != 0)
        {
            ADD_LOG("connect: " << std::strerror(errno));
        }

        // send request
        send(socket_id, request.c_str(), strlen(request.c_str()), 0);

        // recieve data
        if (wait_response == true)
        {
            int n_data_length;
            while ((n_data_length = recv(socket_id, buffer, sizeof(buffer) - 1, 0)) > 0)
            {
                buffer[n_data_length] = '\0';
                res += std::string(buffer);
            }
        }

        close(socket_id);
    }
    catch(std::exception const& e)
    {
        ADD_LOG("Error: " << e.what());
        return "";
    }

    return res;
}


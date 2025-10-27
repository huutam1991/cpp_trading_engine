#include <iostream>
#include <string>
#include <cstring>
#include <signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "https_client_socket.h"

#define BUFFER_SIZE 2048

void HttpsClientSocket::clear()
{
    server_fd = -1;
    save_buffer = "";
}

int HttpsClientSocket::generate_fd()
{
    fd = HttpClientSocket::generate_fd();


    return fd;
}

int HttpsClientSocket::handle_io_data()
{
    char buffer[BUFFER_SIZE];
    char temp_buffer[BUFFER_SIZE];
    int read_bytes = 0;
    int buffer_length = 0;

    if ((read_bytes = read_buffer(temp_buffer)) > 0)
    {
        memcpy(buffer + buffer_length, temp_buffer, read_bytes);
        buffer_length += read_bytes;

        while ((read_bytes = read_buffer(temp_buffer)) > 0)
        {
            memcpy(buffer + buffer_length, temp_buffer, read_bytes);
            buffer_length += read_bytes;
        }

        if (read_bytes == 0)
        {
            spdlog::debug("Connection lost, fd = {}", fd);
            // Clean save buffer
            save_buffer = "";
            return -1;
        }

        buffer[buffer_length] = '\0';
    }
    else
    {
        spdlog::debug("Connection lost, fd = {}", fd);
        // Clean save buffer
        save_buffer = "";
        return -1;
    }

    std::string message = save_buffer + std::string(buffer);
    HttpRequest* request = HttpRequest::CreateNewHttpRequest(message.c_str(), "web_data"); // Temporarily hard code path: web_data

    // Check if request is nullptr (wrong format), return error 404
    if (request == nullptr)
    {
        // Execute on a single thread
        auto task = send_404_response(request);
        task.start_running_on(EventBaseManager::get_event_base_by_id(0)); // Temporarily hard code 0 here, should update later

        return 0;
    }

    // Post request is invalid format when it's is separated into 2 buffer, that's why we need to save the first buffer
    if (request->is_valid_format() == false)
    {
        save_buffer += std::string(buffer);

        delete request;
        return 0;
    }

    // Otherwise, clean save buffer + and execute request
    save_buffer = "";

    // Execute request on a single thread
    auto task = execute_request(request);
    task.start_running_on(EventBaseManager::get_event_base_by_id(0)); // Temporarily hard code 0 here, should update later

    return 0;
}

void HttpsClientSocket::release()
{
    HttpsClientSocketPool::release(this);
}

int HttpsClientSocket::read_buffer(char* const buffer)
{
    return read(fd, buffer, BUFFER_SIZE);
}

void HttpsClientSocket::write_to_socket_io(const char* buffer, std::uint32_t size)
{
    write(fd, buffer, size);
}
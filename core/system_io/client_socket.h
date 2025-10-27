#pragma once

#include <coroutine/task.h>
#include "system_io_object.h"

struct ClientSocket : public SystemIOObject
{
    int server_fd;
    std::string save_buffer;

    ClientSocket(int server_fd_value) : server_fd{server_fd_value} {}

    // SystemIOObject's methods
    virtual void generate_fd();
    virtual int handle_io_data();

    // Handle data methods
    virtual int read_buffer(char* const buffer);
    virtual void write_to_socket_io(const char* buffer, std::uint32_t size);

    Task<void> send_404_response(HttpRequest* request);
    Task<void> execute_request(HttpRequest* request);
};
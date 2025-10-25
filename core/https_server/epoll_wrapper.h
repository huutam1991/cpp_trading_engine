#pragma once

#include <functional>

class EPollWrapper
{
private:
    int m_epoll_fd = -1;
    int m_server_fd = -1;

public:
    EPollWrapper(int server_fd);
    ~EPollWrapper();

    void init_epoll();
    void start_waitting(std::function<int()> accept_new_connection, std::function<void(int)> handle_client_request);

    int add_client_fd(int client_fd);
    int del_client_fd(int client_fd);
};


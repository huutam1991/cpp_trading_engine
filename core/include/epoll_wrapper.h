#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#include <functional>

class EPollWrapper
{
private:
    int m_epoll_fd = -1;
    int m_server_fd = -1;
    bool m_is_waiting = true;

public:
    EPollWrapper(int server_fd);
    ~EPollWrapper();

    void init_epoll();
    void start_waitting(std::function<int()> accept_new_connection, std::function<void(int)> handle_client_request);
    void stop_waiting();

    int add_client_fd(int client_fd);
    int del_client_fd(int client_fd);
};

#endif //EPOLL_WRAPPER_H

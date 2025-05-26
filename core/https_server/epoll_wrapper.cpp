#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>

#include <utils/constants.h>
#include <https_server/epoll_wrapper.h>
#include <utils/util_macros.h>

typedef struct sockaddr_in sockaddr_in;

EPollWrapper::EPollWrapper(int server_fd) : m_server_fd(server_fd)
{
    init_epoll();
}

EPollWrapper::~EPollWrapper()
{
    if (m_epoll_fd != -1)
    {
        close(m_epoll_fd);
    }
}

void EPollWrapper::init_epoll()
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        // LOG(INFO) << "epoll_create1: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = m_server_fd;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &ev) == -1)
    {
        // LOG(INFO) << "epoll_ctl: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void EPollWrapper::stop_waiting()
{
    m_is_waiting = false;
}

void EPollWrapper::start_waitting(std::function<int()> accept_new_connection, std::function<void(int)> handle_client_request)
{
    epoll_event events[BACKLOG_EPOLL];

    int nfds;
    while (m_is_waiting)
    {
        if ((nfds = epoll_wait(m_epoll_fd, events, BACKLOG_EPOLL, -1)) == -1)
        {
            if (errno == EINTR)
            {
                // LOG(INFO) << "Exiting main-loop..." << std::strerror(errno) << std::endl;
                continue; // temporarily put continue here for debuging
                // exit(EXIT_FAILURE);
            }
            else
            {
                // LOG(INFO) << "epoll_wait: " << std::strerror(errno) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < nfds; i++)
        {
            // if fd is server, accept the new connection
            if (events[i].data.fd == m_server_fd)
            {
                int client_fd;
                if ((client_fd = accept_new_connection()) < 0)
                {
                    // LOG(INFO) << "accept socket error: " << std::strerror(errno) << std::endl;
                    continue;
                }

                if (add_client_fd(client_fd) == -1)
                {
                    // LOG(INFO) << "epoll_ctl (EPOLL_CTL_ADD): " << std::strerror(errno) << std::endl;
                    continue;
                }
            }
            // if fd is client, handle the request
            else
            {
                handle_client_request(events[i].data.fd);
            }
        }
    }
}

int EPollWrapper::add_client_fd(int client_fd)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
}

int EPollWrapper::del_client_fd(int client_fd)
{
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
}

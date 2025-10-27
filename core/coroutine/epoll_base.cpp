#include <sys/epoll.h>
#include <netinet/in.h>

#include "epoll_base.h"

#define MAX_EPOLL_EVENTS 1000

EpollBase::EpollBase()
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        spdlog::info("EPollWrapper - [epoll_create1] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }

    m_system_io_object_list.resize(MAX_EPOLL_EVENTS, nullptr);
}

int EpollBase::add_fd(int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    spdlog::info("EPollWrapper - [add_fd] fd: {}", fd);

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int EpollBase::del_fd(int fd)
{
    spdlog::info("EPollWrapper - [del_fd] fd: {}", fd);
    m_system_io_object_list[fd] = nullptr;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void EpollBase::start_running_system_io_object(SystemIOObject* object)
{
    object->epoll_base = this;
    object->generate_fd();
    m_system_io_object_list[object->fd] = object;
    add_fd(object->fd);
}

void EpollBase::loop()
{
    epoll_event events[MAX_EPOLL_EVENTS];

    int nfds;
    while (true)
    {
        if ((nfds = epoll_wait(m_epoll_fd, events, MAX_EPOLL_EVENTS, -1)) == -1)
        {
            if (errno == EINTR)
            {
                // continue; // temporarily put continue here for debuging

                spdlog::info("EPollWrapper - Exiting main-loop ... , error: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
            else
            {
                spdlog::info("EPollWrapper - [epoll_wait] error: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < nfds; i++)
        {
            // if fd is server, accept the new connection
            int fd = events[i].data.fd;

            SystemIOObject* io_object = m_system_io_object_list[fd];
            int res = io_object->handle_io_data();

            // [-1] means there's error with handle io data and need to close this fd
            if (res == -1)
            {
                del_fd(fd);
            }
        }
    }
}
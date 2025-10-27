#include <sys/epoll.h>
#include <netinet/in.h>

#include "epoll_base.h"

EpollBase::EpollBase()
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        spdlog::info("EPollWrapper - [epoll_create1] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int EpollBase::add_fd(int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int EpollBase::del_fd(int fd)
{
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void EpollBase::start_running_system_io_object(SystemIOObject* object)
{

}


void EpollBase::loop()
{}
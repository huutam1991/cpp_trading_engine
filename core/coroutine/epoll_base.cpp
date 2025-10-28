#include <sys/epoll.h>
#include <netinet/in.h>

#include "epoll_base.h"

#define MAX_EPOLL_EVENTS 10000

EpollBase::EpollBase()
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        spdlog::info("EPollWrapper - [epoll_create1] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int EpollBase::add_fd(int fd, void* ptr)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = ptr;
    spdlog::info("EPollWrapper - [add_fd] fd: {}", fd);

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int EpollBase::del_fd(int fd)
{
    spdlog::info("EPollWrapper - [del_fd] fd: {}", fd);

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

void EpollBase::start_living_on(SystemIOObject* object)
{
    object->epoll_base = this;

    int fd = object->generate_fd();
    if (fd < 0)
    {
        spdlog::error("EpollBase - [start_living_on] generate_fd error for fd: {}", fd);
        return;
    }

    add_fd(fd, object);
}

void EpollBase::set_ready_task(void* task_info)
{
    TaskInfo* ti = static_cast<TaskInfo*>(task_info);
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
            void* ptr = events[i].data.ptr;
            SystemIOObject* io_object = static_cast<SystemIOObject*>(ptr);
            int fd = io_object->fd;

            int res = io_object->handle_io_data();

            // [-1] means there's error with handle io data and need to close this fd
            if (res == -1)
            {
                del_fd(fd);
            }
        }
    }
}
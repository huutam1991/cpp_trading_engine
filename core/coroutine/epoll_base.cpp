#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <netinet/in.h>

#include "epoll_base.h"

#define MAX_EPOLL_EVENTS 10000

EpollBase::EpollBase(size_t id) : EventBase(id)
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        spdlog::error("EpollBase - [epoll_create1] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }

    spdlog::info("EpollBase - Created EpollBase with id: {}", m_event_base_id);
}

void EpollBase::add_fd(int fd, SystemIOObject* ptr)
{
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = ptr;

    int res = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (res == -1)
    {
        spdlog::error("EpollBase - [add_fd] epoll_ctl ADD error for fd: {}, error: {}", fd, std::strerror(errno));
    }
    // spdlog::debug("EpollBase - [add_fd] fd: {}", fd);
}

void EpollBase::del_fd(int fd, SystemIOObject* ptr)
{
    int res = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    if (res == -1)
    {
        spdlog::error("EpollBase - [del_fd] epoll_ctl DEL error for fd: {}, error: {}", fd, std::strerror(errno));
    }
    // spdlog::debug("EpollBase - [del_fd] fd: {}", fd);

    if (ptr != nullptr)
    {
        close(ptr->fd);
        ptr->release();
    }
}

void EpollBase::start_living_system_io_object(SystemIOObject* object)
{
    object->epoll_base = this;

    int fd = object->generate_fd();
    if (fd < 0)
    {
        spdlog::error("EpollBase - [start_living_system_io_object] generate_fd error for fd: {}", fd);
        return;
    }

    add_fd(fd, object);
}

void EpollBase::set_ready_task(void* task_info)
{
    SystemIOObject* task = static_cast<SystemIOObject*>(task_info);
    int fd = task->generate_fd();
    if (fd < 0)
    {
        spdlog::error("EpollBase - [set_ready_task], TaskInfo generate_fd error for fd: {}", fd);
        return;
    }

    // Add to epoll
    add_fd(fd, task);

    // Mark this task is ready
    eventfd_write(fd, 1);
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

                spdlog::error("EpollBase - Exiting main-loop ... , error: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
            else
            {
                spdlog::error("EpollBase - [epoll_wait] error: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < nfds; i++)
        {
            SystemIOObject* io_object = static_cast<SystemIOObject*>(events[i].data.ptr);
            int fd = io_object->fd;

            // Handle IO data
            int res = io_object->handle_io_data();

            // [-1] means there's error with handle io data and need to close this fd
            if (res == -1)
            {
                del_fd(fd, io_object);
            }
        }
    }
}
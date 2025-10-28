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

    m_system_io_object_list.resize(MAX_EPOLL_EVENTS, nullptr);
}

int EpollBase::add_fd(int fd)
{
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = fd;
    spdlog::info("EPollWrapper - [add_fd] fd: {}", fd);

    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int EpollBase::del_fd(int fd)
{
    spdlog::info("EPollWrapper - [del_fd] fd: {}", fd);

    if (std::holds_alternative<SystemIOObject*>(m_system_io_object_list[fd]))
    {
        SystemIOObject* object = std::get<SystemIOObject*>(m_system_io_object_list[fd]);
        object->release();
    }
    else if (std::holds_alternative<TaskInfo*>(m_system_io_object_list[fd]))
    {
        TaskInfo* task_info =  std::get<TaskInfo*>(m_system_io_object_list[fd]);
        if (task_info != nullptr)
        {
            check_to_remove_task(task_info);
        }
    }
    m_system_io_object_list[fd] = nullptr;

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

    m_system_io_object_list[fd] = object;
    add_fd(fd);
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
            int fd = events[i].data.fd;

            if (std::holds_alternative<SystemIOObject*>(m_system_io_object_list[fd]))
            {
                SystemIOObject* io_object = std::get<SystemIOObject*>(m_system_io_object_list[fd]);
                int res = io_object->handle_io_data();

                // [-1] means there's error with handle io data and need to close this fd
                if (res == -1)
                {
                    del_fd(fd);
                }
            }
            else if (std::holds_alternative<TaskInfo*>(m_system_io_object_list[fd]))
            {
                TaskInfo* task_info = std::get<TaskInfo*>(m_system_io_object_list[fd]);

                if (task_info != nullptr && task_info->handle != nullptr && task_info->handle.done() == false)
                {
                    if (task_info->is_first_time == true)
                    {
                        task_info->is_first_time = false;
                        auto duration = std::chrono::high_resolution_clock::now() - task_info->start;
                        auto duration_count = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

                        // spdlog::debug("EventBase: {}, Task first wait time: {} microsecond", m_event_base_id, duration_count / 1000.0);
                    }

                    task_info->handle.resume();

                    if (task_info->handle.done() == true)
                    {
                        del_fd(fd);
                    }
                }
            }
        }
    }
}
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <netinet/in.h>

#include "epoll_base.h"
#include "base_promise_type.h"

#define MAX_EPOLL_EVENTS 10000

int EpollBase::TaskInfoEventEpoll::generate_fd()
{
#ifdef TEST_MODE_ONLY
    task_event_generate_fd_count.fetch_add(1, std::memory_order_relaxed);

    if (EpollBase::TestInjection::consume_failure(EpollBase::TestInjection::fail_task_eventfd_count))
    {
        errno = EpollBase::TestInjection::task_eventfd_errno.load(std::memory_order_relaxed);
        fd = -1;

        return -1;
    }
#endif

    fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return fd;
}

int EpollBase::TaskInfoEventEpoll::activate()
{
#ifdef TEST_MODE_ONLY
    task_event_activate_count.fetch_add(1, std::memory_order_relaxed);
#endif

    // Nothing to do for TaskInfoEventEpoll
    return 0;
}

int EpollBase::TaskInfoEventEpoll::handle_read()
{
#ifdef TEST_MODE_ONLY
    task_event_handle_read_count.fetch_add(1, std::memory_order_relaxed);
#endif

    eventfd_t value;
    if (eventfd_read(fd, &value) == -1)
    {
        if (errno != EAGAIN)
        {
            spdlog::error("TaskInfoEventEpoll - eventfd_read failed, fd: {}, error: {}", fd, std::strerror(errno));
        }
    }

    while (true)
    {
        // Check if there's any task ready to process
        TaskInfoEvent task_event = m_task_event_queue->pop();

        // Continue process this task
        if (task_event != nullptr)
        {
            task_event.check_handle();
        }
        else
        {
            break;
        }
    }

    // Always return -1 to indicate this task is done
    return 0;
}

int EpollBase::TaskInfoEventEpoll::handle_write()
{
#ifdef TEST_MODE_ONLY
    task_event_handle_write_count.fetch_add(1, std::memory_order_relaxed);
#endif

    // Nothing to do for write event
    return 0;
}

void EpollBase::TaskInfoEventEpoll::release()
{
#ifdef TEST_MODE_ONLY
    task_event_release_count.fetch_add(1, std::memory_order_relaxed);
#endif

    // TaskInfoEventPool::release(this);
}

EpollBase::EpollBase(EventBaseID id) : EventBase(id)
{
    if ((m_epoll_fd = epoll_create1(0)) == -1)
    {
        spdlog::error("EpollBase - [epoll_create1] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((m_shutdown_fd = create_shutdown_event()) == -1)
    {
        spdlog::error("EpollBase - Failed to create shutdown event, error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ((m_task_event_fd = create_task_event_fd()) == -1)
    {
        spdlog::error("EpollBase - [eventfd] error: {}", std::strerror(errno));
        exit(EXIT_FAILURE);
    }

    spdlog::info("EpollBase - Created EpollBase with id: {}", m_event_base_id);
}

EpollBase::~EpollBase()
{
    if (m_epoll_fd != -1)
    {
        close(m_epoll_fd);
    }

    if (m_shutdown_fd != -1)
    {
        close(m_shutdown_fd);
    }

    if (m_task_event_fd != -1)
    {
        close(m_task_event_fd);
    }
}

int EpollBase::create_shutdown_event()
{
    int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fd == -1)
    {
        spdlog::error("EpollBase - [create_shutdown_event] eventfd error: {}", std::strerror(errno));
        return -1;
    }

    epoll_event ev;
    ev.events = EPOLLIN;

    // Reserved sentinel:
    // nullptr means shutdown event.
    ev.data.ptr = nullptr;

    int res = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (res == -1)
    {
        spdlog::error("EpollBase - [create_shutdown_event] epoll_ctl ADD error: {}", std::strerror(errno));

        close(fd);
        return -1;
    }

    return fd;
}

int EpollBase::create_task_event_fd()
{
    // Create 1 TaskInfoEventEpoll object for all tasks to use
    m_task_info_event = new TaskInfoEventEpoll(&m_task_event_queue);

    int fd = m_task_info_event->generate_fd();
    if (fd == -1)
    {
        spdlog::error("EpollBase - [create_task_event_fd] TaskInfoEventEpoll generate_fd error: {}", std::strerror(errno));
        return -1;
    }

    add_fd(fd, m_task_info_event);

    return fd;
}

void EpollBase::add_fd(int fd, SystemIOObject* ptr)
{
    epoll_event ev;
    ev.events = ptr->get_io_events();
    ev.data.ptr = ptr;

    int res;

#ifdef TEST_MODE_ONLY
    if (TestInjection::consume_failure(TestInjection::fail_epoll_add_count))
    {
        errno = TestInjection::epoll_add_errno.load(std::memory_order_relaxed);
        res = -1;
    }
    else
#endif
    {
        res = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    if (res == -1)
    {
        spdlog::error("EpollBase - [add_fd] epoll_ctl ADD error for fd: {}, error: {}", fd, std::strerror(errno));
    }
}

void EpollBase::mod_fd_events(int fd, SystemIOObject* ptr, uint32_t events)
{
    epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;

    int res = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    if (res == -1)
    {
        spdlog::error("EpollBase - [mod_fd] epoll_ctl MOD error for fd: {}, error: {}", fd, std::strerror(errno));
    }
}

void EpollBase::del_fd(int fd, SystemIOObject* ptr)
{
    if (ptr != nullptr && fd != -1)
    {
        int res;

#ifdef TEST_MODE_ONLY
        if (TestInjection::consume_failure(TestInjection::fail_epoll_del_count))
        {
            errno = TestInjection::epoll_del_errno.load(std::memory_order_relaxed);
            res = -1;
        }
        else
#endif
        {
            res = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        }

        if (res == -1)
        {
            spdlog::error("EpollBase - [del_fd], object name: {}, EPOLL_CTL_DEL error for fd: {}, error: {}", ptr->name(), fd, std::strerror(errno));
        }
        // spdlog::debug("EpollBase - [del_fd] fd: {}", fd);

        close(fd);
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

    int activate_res = object->activate();
    if (activate_res < 0)
    {
        spdlog::error("EpollBase - [start_living_system_io_object] activate error for fd: {}", fd);
        del_fd(fd, object);
        return;
    }
}

void EpollBase::set_ready_task()
{
    // Mark this task as ready.
    int write_res;

#ifdef TEST_MODE_ONLY
    if (TestInjection::consume_failure(TestInjection::fail_eventfd_write_count))
    {
        errno = TestInjection::eventfd_write_errno.load(std::memory_order_relaxed);
        write_res = -1;
    }
    else
#endif
    {
        write_res = eventfd_write(m_task_event_fd, 1);
    }

    // The current production implementation intentionally keeps its existing
    // behavior here: the return value is not handled yet. Failure-path tests
    // can now inject write_res == -1 and expose the missing rollback logic.
    (void)write_res;
}

void EpollBase::stop()
{
    if (m_shutdown_fd != -1)
    {
        eventfd_write(m_shutdown_fd, 1);
    }
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
            // This is the shutdown event, exit the loop and stop the event base
            if (events[i].data.ptr == nullptr)
            {
                eventfd_t value;
                eventfd_read(m_shutdown_fd, &value);

                return;
            }

            SystemIOObject* io_object = static_cast<SystemIOObject*>(events[i].data.ptr);
            int fd = io_object->fd;

            uint32_t event = events[i].events;

            if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                // An error has occured on this fd
                spdlog::error("EpollBase - [loop] EPOLLERR or EPOLLHUP or EPOLLRDHUP on fd: {}", fd);
                del_fd(fd, io_object);
                continue;
            }

            // Handle read event first
            if (event & EPOLLIN)
            {
                int res = io_object->handle_read();
                if (res == -1)
                {
                    del_fd(fd, io_object);
                    continue;
                }
            }

            // Handle write event (if needed)
            if (event & EPOLLOUT)
            {
                int res = io_object->handle_write();
                if (res == -1)
                {
                    del_fd(fd, io_object);
                    continue;
                }
            }
        }
    }
}
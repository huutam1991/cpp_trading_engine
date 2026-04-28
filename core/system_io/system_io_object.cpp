#include "system_io_object.h"

#include <coroutine/epoll_base.h>

void SystemIOObject::write(std::string data)
{
    if (data.empty())
    {
        return;
    }

    // If there is pending data in the queue, push new data to the queue and wait for the turn to write
    if (!m_write_queue.empty() || current_state != State::READING_AND_WRITING)
    {
        m_write_queue.push_back(std::move(data));
        enable_write_event();
        return;
    }

    const int n = write_to_socket_io(data.data(), static_cast<std::uint32_t>(data.size()));

    if (n == static_cast<int>(data.size()))
    {
        // Already write full data, return
        return;
    }

    if (n > 0)
    {
        // Partial write: queue the remaining data
        data.erase(0, static_cast<std::size_t>(n));
        m_write_queue.push_back(std::move(data));
        m_write_offset = 0;
        enable_write_event();
        return;
    }

    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        // Socket buffer full: queue the whole data and wait for the turn to write
        m_write_queue.push_back(std::move(data));
        m_write_offset = 0;
        enable_write_event();
        return;
    }

    spdlog::error("SystemIOObject::write - write failed fd = {}, err = {}, data = {}", fd, std::strerror(errno), data);
}

int SystemIOObject::check_to_write()
{
    while (!m_write_queue.empty())
    {
        std::string& data = m_write_queue.front();
        const int n = write_to_socket_io(data.data() + m_write_offset, data.size() - m_write_offset);

        if (n > 0)
        {
            m_write_offset += static_cast<std::size_t>(n);

            if (m_write_offset == data.size())
            {
                m_write_queue.pop_front();
                m_write_offset = 0;
                continue;
            }

            // Partial again. Wait for next EPOLLOUT.
            return 0;
        }

        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            // Still not writable enough.
            return 0;
        }

        spdlog::error("SystemIOObject::check_to_write - write failed fd = {}, err = {}", fd, std::strerror(errno));
        return -1;
    }

    // Queue empty => no need to receive EPOLLOUT anymore.
    disable_write_event();
    return 0;
}

void SystemIOObject::enable_write_event()
{
    static constexpr uint32_t READ_WRITE_EVENTS = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if (m_write_event_enabled == true)
    {
        return;
    }

    m_write_event_enabled = true;
    epoll_base->mod_fd_events(fd, this, READ_WRITE_EVENTS);
}

void SystemIOObject::disable_write_event()
{
    static constexpr uint32_t READ_EVENTS = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    if (m_write_event_enabled == false)
    {
        return;
    }

    m_write_event_enabled = false;
    epoll_base->mod_fd_events(fd, this, READ_EVENTS);
}
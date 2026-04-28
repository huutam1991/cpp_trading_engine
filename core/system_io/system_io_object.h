#pragma once

#include <cstdint>
#include <sys/epoll.h>
#include <deque>

#include <spdlog/spdlog.h>
#include <utils/type_name.h>

class EpollBase;

struct SystemIOObject
{
    int fd; // File descriptor
    EpollBase *epoll_base = nullptr;

    virtual std::string name() = 0;
    virtual int generate_fd() = 0;
    virtual int get_io_events() { return EPOLLIN | EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP; }
    virtual int activate() = 0;
    virtual int handle_read() = 0;
    virtual int handle_write() = 0;
    virtual void release() = 0;

    // For non-blocking writer
    enum State
    {
        CONNECTING_AND_HANDSHAKING,
        READING_AND_WRITING,
        NONE
    };
    State current_state = State::NONE;

    std::deque<std::string> m_write_queue;
    std::size_t m_write_offset = 0;
    bool m_write_event_enabled = false;

    void write(std::string data);
    int check_to_write();
    void enable_write_event();
    void disable_write_event();

    virtual int write_to_socket_io(const char* buffer, std::uint32_t size)
    {
        spdlog::error("SystemIOObject::write_to_socket_io - write_to_socket_io not implemented for {}, fd = {}", name(), fd);
        return -1;
    }
};

template <class T>
struct NamedIOObject : public SystemIOObject
{
    virtual std::string name() override
    {
        return type_name::TypeName<T>::name();
    }
};
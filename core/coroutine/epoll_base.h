#pragma once

#include <variant>
#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;
    int m_shutdown_fd;

    void add_fd(int fd, SystemIOObject* ptr);

public:
    EpollBase(size_t id);
    virtual ~EpollBase() override;

    int create_shutdown_event();
    void mod_fd_events(int fd, SystemIOObject* ptr, uint32_t events);
    void del_fd(int fd, SystemIOObject* ptr);
    void start_living_system_io_object(SystemIOObject* object);
    virtual void stop() override;
    virtual void loop() override;
};
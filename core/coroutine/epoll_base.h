#pragma once

#include <variant>
#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;

    int add_fd(int fd, void* ptr);
    int del_fd(int fd);

public:
    EpollBase();

    void start_living_on(SystemIOObject* object);
    virtual void set_ready_task(void* task_info);
    virtual void loop();
};
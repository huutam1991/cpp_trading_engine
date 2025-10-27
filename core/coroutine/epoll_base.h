#pragma once

#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;

    int add_fd(int client_fd);
    int del_fd(int client_fd);

public:
    EpollBase();

    void start_running_system_io_object(SystemIOObject* object);
    void loop();
};
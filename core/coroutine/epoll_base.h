#pragma once

#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;
    std::vector<SystemIOObject*> m_system_io_object_list;

    int add_fd(int client_fd);
    int del_fd(int client_fd);

public:
    EpollBase();

    void start_living_on(SystemIOObject* object);
    void loop();
};
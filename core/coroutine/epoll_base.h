#pragma once

#include <variant>
#include <system_io/system_io_object.h>

#include "event_base.h"

class EpollBase : public EventBase
{
    int m_epoll_fd;
    std::vector<std::variant<SystemIOObject*, TaskInfo*, std::nullptr_t>> m_system_io_object_list;

    int add_fd(int client_fd);
    int del_fd(int client_fd);

public:
    EpollBase();

    void start_living_on(SystemIOObject* object);
    virtual void set_ready_task(void* task_info);
    virtual void loop();
};
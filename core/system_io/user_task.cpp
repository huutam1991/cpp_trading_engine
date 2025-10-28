#include <sys/eventfd.h>
#include "user_task.h"

int UserTask::generate_fd()
{
    fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return fd;
}

int UserTask::handle_io_data()
{
    return 0;
}

void UserTask::release()
{
    UserTaskPool::release(this);
}
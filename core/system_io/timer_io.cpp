#include "timer_io.h"


void TimerIO::clear()
{
    callback = nullptr;
    interval_ns = 0;
}

int TimerIO::generate_fd()
{
    fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (fd < 0)
    {
        perror("timerfd_create");
        return -1;
    }

    itimerspec ts{};
    ts.it_value.tv_sec  = interval_ns / 1000000000;
    ts.it_value.tv_nsec = interval_ns % 1000000000;
    if (timerfd_settime(fd, 0, &ts, nullptr) < 0)
    {
        perror("timerfd_settime");
        close(fd);
        return -1;
    }

    return fd;
}

int TimerIO::handle_io_data()
{
    return 0;
}

void TimerIO::release()
{
    if (callback != nullptr)
    {
        callback();
    }

    TimerIOPool::release(this);
}
#include "timer_io.h"

void TimerIO::set_callback(size_t interval_ns_value, std::function<void()> callback_value)
{
    interval_ns = interval_ns_value;
    callback = std::move(callback_value);
}

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

int TimerIO::activate()
{
    // Nothing to do for TimerIO
    return 0;
}

int TimerIO::handle_read()
{
    if (callback != nullptr)
    {
        callback();
    }

    // [-1] mean always release after handling
    return -1;
}

int TimerIO::handle_write()
{
    // Nothing to do for write event
    return 0;
}

void TimerIO::release()
{
    TimerIOPool::release(this);
}
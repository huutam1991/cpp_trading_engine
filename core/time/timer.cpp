#include <time/timer.h>

EpollBase* Timer::get_epoll_base()
{
    static EpollBase* epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::SYSTEM_IO_TASK);
    return epoll_base;
}

void Timer::add_schedule_task(std::function<void()> callback, size_t tick_interval, TimerUnit unit)
{
    TimerIO* timer_io = TimerIOPool::acquire();
    timer_io->set_callback(tick_interval * unit, std::move(callback));
    get_epoll_base()->start_living_system_io_object(timer_io);
}

Future<size_t> Timer::sleep_for(size_t tick_interval, TimerUnit unit)
{
    size_t tick = tick_interval * unit; // Tick in nanoseconds

    return Future<size_t>([tick](Future<size_t>::FutureValue value)
    {
        add_schedule_task([tick, value]() mutable
        {
            size_t tick_none_const = tick;
            value.set_value(tick_none_const);
        }, tick, TimerUnit::NANOSECOND);
    });
}
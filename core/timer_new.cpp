#include <timer_new.h>

void TimerNew::init(boost::asio::io_context& io_context)
{
    get_io_context() = &io_context;
}

boost::asio::io_context*& TimerNew::get_io_context()
{
    static boost::asio::io_context* io_context = nullptr;
    return io_context;
}

void TimerNew::check_valid_io_context()
{
    if (get_io_context() == nullptr)
    {
        throw std::runtime_error("Schedule task with [m_io_context] is nullptr ");
    }
}

void TimerNew::add_schedule_task(std::function<void()> callback, size_t tick_interval, TimerUnit unit)
{
    check_valid_io_context();

    size_t tick = tick_interval * unit; // Tick in nanoseconds
    auto task = std::make_shared<TimerNew::Task>(*get_io_context(), std::move(callback), tick);
    task->start();
}

Future<size_t> TimerNew::sleep_for(size_t tick_interval, TimerUnit unit)
{
    size_t tick = tick_interval * unit; // Tick in nanoseconds

    return Future<size_t>([tick](Future<size_t>::FutureValue value)
    {
        add_schedule_task([tick, value]() mutable
        {
            value.set_value(tick);
        }, tick, TimerUnit::NANOSECOND);
    });
}

TimerNew::Task::Task(boost::asio::io_context& io_context, std::function<void()> callback, size_t tick_in_nanoseconds)
    : m_callback{std::move(callback)},
      m_timer{std::make_unique<boost::asio::steady_timer>(io_context, std::chrono::nanoseconds(tick_in_nanoseconds))}
{
}

void TimerNew::Task::start()
{
    m_timer->async_wait(boost::bind(&TimerNew::Task::on_tick, shared_from_this()));
}

void TimerNew::Task::on_tick()
{
    if (m_callback != nullptr)
    {
        m_callback();
    }
}
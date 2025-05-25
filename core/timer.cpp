#include <timer.h>

void Timer::init(boost::asio::io_context& io_context)
{
    get_io_context() = &io_context;
}

boost::asio::io_context*& Timer::get_io_context()
{
    static boost::asio::io_context* io_context = nullptr;
    return io_context;
}

void Timer::check_valid_io_context()
{
    if (get_io_context() == nullptr)
    {
        throw std::runtime_error("Schedule task with [m_io_context] is nullptr ");
    }
}

void Timer::add_schedule_task(std::function<void()> callback, size_t tick_interval, TimerUnit unit)
{
    check_valid_io_context();

    size_t tick = tick_interval * unit; // Tick in nanoseconds
    auto task = std::make_shared<Timer::Task>(*get_io_context(), std::move(callback), tick);
    task->start();
}

void Timer::add_schedule_task_on_ioc(std::function<void()> callback, boost::asio::io_context& ioc, size_t tick_interval, TimerUnit unit)
{
    size_t tick = tick_interval * unit; // Tick in nanoseconds
    auto task = std::make_shared<Timer::Task>(ioc, std::move(callback), tick);
    task->start();
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

Timer::Task::Task(boost::asio::io_context& io_context, std::function<void()> callback, size_t tick_in_nanoseconds)
    : m_callback{std::move(callback)},
      m_timer{std::make_unique<boost::asio::steady_timer>(io_context, std::chrono::nanoseconds(tick_in_nanoseconds))}
{
}

void Timer::Task::start()
{
    m_timer->async_wait(boost::bind(&Timer::Task::on_tick, shared_from_this()));
}

void Timer::Task::on_tick()
{
    if (m_callback != nullptr)
    {
        m_callback();
    }
}
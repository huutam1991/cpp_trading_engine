#include <timer_new.h>

void TimerNew::init(boost::asio::io_context& ioc_context)
{
    get_ioc_context() = &ioc_context;
}

boost::asio::io_context*& TimerNew::get_ioc_context()
{
    static boost::asio::io_context* io_context = nullptr;
    return io_context;
}

void TimerNew::add_schedule_task(std::function<void(void)> callback, size_t tick_interval, TimerUnit unit)
{
    boost::asio::io_context*& io_context = get_ioc_context();

    if (io_context == nullptr)
    {
        throw std::runtime_error("Schedule task with [m_io_context] is nullptr ");
    }

    size_t tick_in_nanoseconds = tick_interval * unit;
    auto task = std::make_shared<TimerNew::Task>(*io_context, std::move(callback), tick_in_nanoseconds);
    task->start();
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
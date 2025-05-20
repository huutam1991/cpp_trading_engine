#include <timer.h>
#include <utils.h>

Timer::Task::Task(std::function<void(void)> w, size_t p, bool is_one_shot) :
    m_work(w), m_period(p), m_is_one_shot(is_one_shot)
{
}

Timer::Task::~Task()
{
}

Timer::Timer()
{
    init();
}

void Timer::init()
{
    m_thread = std::thread([this]() 
    {
        m_timer = std::make_unique<boost::asio::steady_timer>(m_io_service, std::chrono::milliseconds(1));
        m_timer->async_wait(boost::bind(&Timer::on_tick, this));

        m_io_service.run();
    });
}

void Timer::on_tick()
{
    reschedule_next_tick();

    std::unique_lock lock(m_list_mutex);
    std::vector<size_t> remove_task_list;

    for (auto it = m_taks_list.begin(); it != m_taks_list.end(); it++)
    {
        Task& task = *it->second;

        task.m_count++;
        if (task.m_count == task.m_period)
        {
            task.m_work();
            task.m_count = 0;

            if (task.m_is_one_shot == true)
            {
                remove_task_list.push_back(it->first);
            }
        }
    }

    // Remove tasks that m_is_one_shot == true
    for (size_t task_id : remove_task_list)
    {
        m_taks_list.erase(task_id);
    }
}

void Timer::reschedule_next_tick()
{
    // Reschedule the timer for 1 milisecond second in the future:
    m_timer->expires_at(m_timer->expiry() + std::chrono::milliseconds(1));
    // Posts the timer event
    m_timer->async_wait(boost::bind(&Timer::on_tick, this));
}

size_t Timer::get_new_task_id()
{
    static size_t task_id = 0;
    return ++task_id;
}

size_t Timer::add_schedule_task(std::function<void(void)> work, size_t period)
{
    std::unique_lock lock(m_list_mutex);
    size_t task_id = get_new_task_id();
    m_taks_list[task_id] = std::make_unique<Task>(work, period);
    return task_id;
}

size_t Timer::add_time_out(std::function<void(void)> work, size_t period)
{
    std::unique_lock lock(m_list_mutex);
    size_t task_id = get_new_task_id();
    m_taks_list[task_id] = std::make_unique<Task>(work, period, true);
    return task_id;
}

void Timer::delete_schedule_task(size_t task_id)
{
    std::unique_lock lock(m_list_mutex);

    if (m_taks_list.find(task_id) != m_taks_list.end())
    {
        ADD_LOG("Delete schedule task id = " << task_id);
        m_taks_list.erase(task_id);
    }
}

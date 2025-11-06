#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/asio.hpp>

#include <utils/thread_pinning.h>

namespace net = boost::asio;

enum IOCId
{
    MARKET_DATA = 4,
    ORDER_ENTRY,
};

class IOCPool
{
public:
    static net::io_context& get_ioc_by_id(size_t id)
    {
        static std::mutex mutex;
        static std::vector<std::thread> threads;
        static std::unordered_map<size_t, std::shared_ptr<net::io_context>> ioc_list;
        static std::unordered_map<size_t, std::shared_ptr<net::executor_work_guard<net::io_context::executor_type>>> guards;

        std::lock_guard<std::mutex> lock(mutex);

        if (ioc_list.find(id) == ioc_list.end())
        {
            // ioc
            auto ioc = std::make_shared<net::io_context>(id);
            ioc_list.insert(std::make_pair(id, ioc));

            // guard
            auto guard = std::make_shared<net::executor_work_guard<net::io_context::executor_type>>(net::make_work_guard(*ioc));
            guards[id] = guard;

            threads.emplace_back([ioc, id]()
            {
                // Pin each IOC thread to a specific core
                ThreadPinning::pin_thread_to_core(static_cast<int>(id));
                ioc->run();
            });
        }

        return *ioc_list[id].get();
    }
};
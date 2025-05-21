#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <boost/asio.hpp>

namespace net = boost::asio;

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

            threads.emplace_back([ioc]()
            {
                ioc->run();
            });
        }

        return *ioc_list[id].get();
    }
};
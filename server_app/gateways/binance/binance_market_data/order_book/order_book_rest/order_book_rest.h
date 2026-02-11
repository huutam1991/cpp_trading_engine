#pragma once

#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>

class OrderBookRest
{
public:
    OrderBookRest();
    Task<std::string> get_order_book(const std::string& symbol, size_t depth);

private:
    EpollBase* m_epoll_base = (EpollBase*)EventBaseManager::get_event_base_by_id(EpollBaseID::GATEWAY);
};
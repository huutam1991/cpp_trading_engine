#pragma once

#include <mutex>
#include <functional>
#include <unordered_map>
#include <queue>
#include <spdlog/spdlog.h>

#include <utils/util_macros.h>
#include <coroutine/task.h>
#include <coroutine/future.h>

#include <order/order.h>
#include <data_model/savable_object.h>

namespace std {
    template<>
    struct hash<std::pair<size_t, Order::Status>> {
        std::size_t operator()(const std::pair<OrderId, Order::Status>& p) const noexcept {
            return std::hash<size_t>{}(p.first) ^ (std::hash<int>{}(static_cast<int>(p.second)) << 1);
        }
    };
}

class OrderManager
{
    Singleton(OrderManager);

private:
    std::unordered_map<OrderId, Order> m_order_list;
    std::vector<std::function<void(Order)>> m_order_update_callbacks;
    EventBase* m_order_event_base = nullptr;

    // For handling order create / update
    Order& get_order_by_id(OrderId order_id);
    Task<void> update_order_in_db(Order order);
    Task<void> handle_update_order(Order order);

public:
    // Helper method
    static OrderId generate_order_id();
    std::vector<Order> get_all_orders();
    std::vector<OrderId> get_open_orders();

    inline bool is_valid_order(OrderId order_id)
    {
        return m_order_list.find(order_id) != m_order_list.end();
    }

    inline Order::Status get_order_status(OrderId order_id)
    {
        auto it = m_order_list.find(order_id);
        return it != m_order_list.end() ? it->second.status : Order::Status::NOT_AVAILABLE;
    }

    void init();
    void register_order_update(std::function<void(Order)> callback);
    void set_cancel_order(OrderId order_id);
    void update_order(Order order);

};
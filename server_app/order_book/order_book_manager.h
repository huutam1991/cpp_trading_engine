#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstddef>

#include <coroutine/task.h>
#include <coroutine/event_base_manager.h>
#include <utils/util_macros.h>
#include <order_book/order_book.h>
#include <order_book/order_book_snapshot.h>

class OrderBookManager
{
    Singleton(OrderBookManager);

private:
    EventBase* m_event_base = EventBaseManager::get_event_base_by_id(EventBaseID::ORDER_BOOK);

    std::vector<std::function<void(OrderBookSnapShotObject)>> m_update_callbacks;
    std::unordered_map<const Instrument*, std::unique_ptr<OrderBook>> m_order_books;

    std::size_t m_depth = 10000;
    std::size_t m_publish_levels = 20;
    double m_tick_size = 0.01;

private:
    Task<void> run_update_order_book_data(OrderBookSnapShotObject snapshot);
    Task<void> run_update_order_book_data(OrderBookUpdateObject update);
    Task<void> run_update_order_book_data(std::vector<OrderBookUpdate> updates);

    OrderBook& get_or_create_order_book(const OrderBookSnapShotObject& snapshot);
    OrderBook& get_or_create_order_book(const OrderBookUpdateObject& update);
    OrderBook& get_or_create_order_book(const OrderBookUpdate& update);

    double get_snapshot_reference_price(const OrderBookSnapShotObject& snapshot) const noexcept;

public:
    void register_update(std::function<void(OrderBookSnapShotObject)> callback);

    template<class T>
    void publish_order_book_data(T update)
    {
        auto task = run_update_order_book_data(update);
        task.start_running_on(m_event_base);
    }

    void set_config(
        double tick_size,
        std::size_t depth,
        double rebase_delta,
        std::size_t publish_levels
    );

    OrderBook* get_order_book(const Instrument* instrument) noexcept;
};
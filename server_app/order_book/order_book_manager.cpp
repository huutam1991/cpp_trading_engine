#include <order_book/order_book_manager.h>

void OrderBookManager::register_update(std::function<void(OrderBookSnapShotObject)> callback)
{
    m_update_callbacks.push_back(std::move(callback));
}

void OrderBookManager::set_config(
    double tick_size,
    std::size_t depth,
    double rebase_delta,
    std::size_t publish_levels
)
{
    m_tick_size = tick_size;
    m_depth = depth;
    m_publish_levels = publish_levels;
}

OrderBook& OrderBookManager::get_or_create_order_book(const OrderBookSnapShotObject& snapshot)
{
    const Instrument* instrument = snapshot->instrument;

    auto it = m_order_books.find(instrument);

    if (it != m_order_books.end())
    {
        return *(it->second);
    }

    const double base_price = get_snapshot_reference_price(snapshot);

    auto order_book = std::make_unique<OrderBook>(
        instrument,
        base_price,
        instrument->price_precision,
        m_depth
    );

    auto [inserted_it, inserted] = m_order_books.emplace(instrument, std::move(order_book));

    return *(inserted_it->second);
}

OrderBook& OrderBookManager::get_or_create_order_book(const OrderBookUpdate& update)
{
    const Instrument* instrument = update.instrument;

    auto it = m_order_books.find(instrument);

    if (it != m_order_books.end())
    {
        return *(it->second);
    }

    auto order_book = std::make_unique<OrderBook>(
        instrument,
        update.price,
        instrument->price_precision,
        m_depth
    );

    auto [inserted_it, inserted] = m_order_books.emplace(instrument, std::move(order_book));

    return *(inserted_it->second);
}


double OrderBookManager::get_snapshot_reference_price(const OrderBookSnapShotObject& snapshot) const noexcept
{
    const bool has_bid = snapshot->bids_size > 0;
    const bool has_ask = snapshot->asks_size > 0;

    if (has_bid)
    {
        return snapshot->bids[0].price;
    }

    if (has_ask)
    {
        return snapshot->asks[0].price;
    }

    return 0.0;
}

OrderBook* OrderBookManager::get_order_book(const Instrument* instrument) noexcept
{
    auto it = m_order_books.find(instrument);

    if (it == m_order_books.end())
    {
        return nullptr;
    }

    return it->second.get();
}

Task<void> OrderBookManager::run_update_order_book_snapshot(OrderBookSnapShotObject snapshot)
{
    if (snapshot == nullptr || snapshot->instrument == nullptr)
    {
        co_return;
    }

    OrderBook& order_book = get_or_create_order_book(snapshot);
    order_book.apply_update(*snapshot);

    OrderBookSnapShotObject output_snapshot = order_book.get_order_book_snapshot(m_publish_levels);

    for (auto& callback : m_update_callbacks)
    {
        callback(output_snapshot);
    }

    co_return;
}

Task<void> OrderBookManager::run_update_order_book_snapshot(OrderBookUpdate update)
{
    if (update.instrument == nullptr)
    {
        co_return;
    }

    OrderBook& order_book = get_or_create_order_book(update);
    order_book.apply_update(update);

    OrderBookSnapShotObject output_snapshot = order_book.get_order_book_snapshot(m_publish_levels);

    for (auto& callback : m_update_callbacks)
    {
        callback(output_snapshot);
    }

    co_return;
}
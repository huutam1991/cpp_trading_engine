#pragma once

#include <cstddef>

#include "order_book_side.h"
#include "order_book_snapshot.h"

enum class OrderBookUpdateType
{
    Add,
    Update,
    Remove
};

struct OrderBookUpdate
{
    OrderBookSideType side;
    OrderBookUpdateType type;
    double price;
    double quantity;
};

class OrderBook
{
public:
    OrderBook(
        const Instrument* instrument,
        double base_price,
        double tick_size,
        std::size_t depth
    )
        : OrderBook(
              instrument,
              base_price,
              tick_size,
              depth,
              tick_size * static_cast<double>(depth) * DEFAULT_REBASE_DELTA_RATIO
          )
    {
    }

    OrderBook(
        const Instrument* instrument,
        double base_price,
        double tick_size,
        std::size_t depth,
        double rebase_delta
    )
        : m_bids(OrderBookSideType::Bid, base_price, tick_size, depth),
          m_asks(OrderBookSideType::Ask, base_price, tick_size, depth),
          m_instrument(instrument),
          m_rebase_delta(rebase_delta)
    {
    }

    inline void reset() noexcept
    {
        m_bids.clear();
        m_asks.clear();
    }

    inline void apply_update(const OrderBookSnapShot& snapshot)
    {
        reset();

        for (std::size_t i = 0; i < snapshot.bids_size; ++i)
        {
            apply_update_without_rebase({
                OrderBookSideType::Bid,
                OrderBookUpdateType::Update,
                snapshot.bids[i].price,
                snapshot.bids[i].quantity
            });
        }

        for (std::size_t i = 0; i < snapshot.asks_size; ++i)
        {
            apply_update_without_rebase({
                OrderBookSideType::Ask,
                OrderBookUpdateType::Update,
                snapshot.asks[i].price,
                snapshot.asks[i].quantity
            });
        }

        maybe_rebase();
    }

    inline void apply_update(const OrderBookUpdate& update)
    {
        apply_update_without_rebase(update);
        maybe_rebase();
    }

    inline void apply_update_without_rebase(const OrderBookUpdate& update) noexcept
    {
        OrderBookSide& side = get_mutable_side(update.side);

        switch (update.type)
        {
            case OrderBookUpdateType::Add:
            case OrderBookUpdateType::Update:
            {
                side.set_level(update.price, update.quantity);
                break;
            }

            case OrderBookUpdateType::Remove:
            {
                side.remove_level(update.price);
                break;
            }
        }
    }

    inline void set_bid(double price, double quantity)
    {
        apply_update({
            OrderBookSideType::Bid,
            OrderBookUpdateType::Update,
            price,
            quantity
        });
    }

    inline void set_ask(double price, double quantity)
    {
        apply_update({
            OrderBookSideType::Ask,
            OrderBookUpdateType::Update,
            price,
            quantity
        });
    }

    inline void remove_bid(double price)
    {
        apply_update({
            OrderBookSideType::Bid,
            OrderBookUpdateType::Remove,
            price,
            0.0
        });
    }

    inline void remove_ask(double price)
    {
        apply_update({
            OrderBookSideType::Ask,
            OrderBookUpdateType::Remove,
            price,
            0.0
        });
    }

    inline bool should_rebase() const noexcept
    {
        if (!has_spread())
        {
            return false;
        }

        return m_bids.in_rebase_trigger_zone(
                best_bid_price(),
                m_rebase_delta
            )
            ||
            m_asks.in_rebase_trigger_zone(
                best_ask_price(),
                m_rebase_delta
            );
    }

    inline void maybe_rebase()
    {
        if (!should_rebase())
        {
            return;
        }

        move_to_new_base_price(best_bid_price());
    }

    inline void move_to_new_base_price(double new_base_price)
    {
        m_bids.move_to_new_base_price(new_base_price);
        m_asks.move_to_new_base_price(new_base_price);
    }

    inline double get_bid_quantity(double price) const noexcept
    {
        return m_bids.get_quantity(price);
    }

    inline double get_ask_quantity(double price) const noexcept
    {
        return m_asks.get_quantity(price);
    }

    inline bool has_best_bid() const noexcept
    {
        return m_bids.has_top();
    }

    inline bool has_best_ask() const noexcept
    {
        return m_asks.has_top();
    }

    inline double best_bid_price() const noexcept
    {
        return m_bids.get_top_price();
    }

    inline double best_ask_price() const noexcept
    {
        return m_asks.get_top_price();
    }

    inline double best_bid_quantity() const noexcept
    {
        return m_bids.get_top_quantity();
    }

    inline double best_ask_quantity() const noexcept
    {
        return m_asks.get_top_quantity();
    }

    inline bool has_spread() const noexcept
    {
        return has_best_bid() && has_best_ask();
    }

    inline double spread() const noexcept
    {
        if (!has_spread())
        {
            return 0.0;
        }

        return best_ask_price() - best_bid_price();
    }

    inline double mid_price() const noexcept
    {
        if (!has_spread())
        {
            return 0.0;
        }

        return (best_bid_price() + best_ask_price()) * 0.5;
    }

    inline bool crossed() const noexcept
    {
        if (!has_spread())
        {
            return false;
        }

        return best_bid_price() >= best_ask_price();
    }

    inline void reset_base_price(double new_base_price)
    {
        m_bids.reset_base_price(new_base_price);
        m_asks.reset_base_price(new_base_price);
    }

    inline double base_price() const noexcept
    {
        return m_bids.base_price();
    }

    inline double tick_size() const noexcept
    {
        return m_bids.tick_size();
    }

    inline std::size_t depth() const noexcept
    {
        return m_bids.size();
    }

    inline double rebase_delta() const noexcept
    {
        return m_rebase_delta;
    }

    inline void set_rebase_delta(double rebase_delta) noexcept
    {
        m_rebase_delta = rebase_delta;
    }

    inline OrderBookSnapShotObject get_order_book_snapshot(std::size_t levels) const
    {
        OrderBookSnapShotObject snapshot = OrderBookSnapShotPool::acquire();

        snapshot->update_instrument(m_instrument);
        snapshot->resize(levels);
        snapshot->refresh();

        std::size_t bid_count = 0;
        std::size_t ask_count = 0;

        if (m_bids.has_top())
        {
            for (std::size_t i = m_bids.top_index() + 1; i > 0 && bid_count < levels; --i)
            {
                const std::size_t index = i - 1;
                const double quantity = m_bids.quantity_at_index(index);

                if (quantity <= 0.0)
                {
                    continue;
                }

                snapshot->add_bid(
                    m_bids.index_to_price(index),
                    quantity
                );

                ++bid_count;
            }
        }

        if (m_asks.has_top())
        {
            for (std::size_t index = m_asks.top_index(); index < m_asks.size() && ask_count < levels; ++index)
            {
                const double quantity = m_asks.quantity_at_index(index);

                if (quantity <= 0.0)
                {
                    continue;
                }

                snapshot->add_ask(
                    m_asks.index_to_price(index),
                    quantity
                );

                ++ask_count;
            }
        }

        return snapshot;
    }

    inline const OrderBookSide& bids() const noexcept
    {
        return m_bids;
    }

    inline const OrderBookSide& asks() const noexcept
    {
        return m_asks;
    }

    inline OrderBookSide& mutable_bids() noexcept
    {
        return m_bids;
    }

    inline OrderBookSide& mutable_asks() noexcept
    {
        return m_asks;
    }

private:
    inline OrderBookSide& get_mutable_side(OrderBookSideType side) noexcept
    {
        return side == OrderBookSideType::Bid ? m_bids : m_asks;
    }

    inline const OrderBookSide& get_side(OrderBookSideType side) const noexcept
    {
        return side == OrderBookSideType::Bid ? m_bids : m_asks;
    }

private:
    static constexpr double DEFAULT_REBASE_DELTA_RATIO = 0.10;

private:
    OrderBookSide m_bids;
    OrderBookSide m_asks;

    const Instrument* m_instrument = nullptr;
    double m_rebase_delta;
};
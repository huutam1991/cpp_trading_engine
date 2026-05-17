#pragma once

#include <cstddef>

#include "order_book_side.h"

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
        double base_price,
        double tick_size,
        std::size_t depth
    )
        : m_bids(OrderBookSideType::Bid, base_price, tick_size, depth),
          m_asks(OrderBookSideType::Ask, base_price, tick_size, depth)
    {
    }

    inline void apply_update(const OrderBookUpdate& update) noexcept
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

    inline void set_bid(double price, double quantity) noexcept
    {
        apply_update({
            OrderBookSideType::Bid,
            OrderBookUpdateType::Update,
            price,
            quantity
        });
    }

    inline void set_ask(double price, double quantity) noexcept
    {
        apply_update({
            OrderBookSideType::Ask,
            OrderBookUpdateType::Update,
            price,
            quantity
        });
    }

    inline void remove_bid(double price) noexcept
    {
        apply_update({
            OrderBookSideType::Bid,
            OrderBookUpdateType::Remove,
            price,
            0.0
        });
    }

    inline void remove_ask(double price) noexcept
    {
        apply_update({
            OrderBookSideType::Ask,
            OrderBookUpdateType::Remove,
            price,
            0.0
        });
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
    OrderBookSide m_bids;
    OrderBookSide m_asks;
};
#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <limits>

class OrderBookSide
{
public:
    OrderBookSide(double base_price, double tick_size, std::size_t depth)
        : m_base_price(base_price),
          m_tick_size(tick_size),
          m_inv_tick_size(1.0 / tick_size),
          m_center_index(depth / 2),
          m_levels(depth, 0.0)
    {
        if (tick_size <= 0.0)
        {
            throw std::invalid_argument("tick_size must be positive");
        }

        if (depth == 0)
        {
            throw std::invalid_argument("depth must be positive");
        }
    }

    inline std::size_t price_to_index(double price) const noexcept
    {
        const double diff = price - m_base_price;
        const auto tick_offset = static_cast<int64_t>(std::llround(diff * m_inv_tick_size));

        return static_cast<std::size_t>(
            static_cast<int64_t>(m_center_index) + tick_offset
        );
    }

    inline double index_to_price(std::size_t index) const noexcept
    {
        const auto tick_offset = static_cast<int64_t>(index) - static_cast<int64_t>(m_center_index);

        return m_base_price + static_cast<double>(tick_offset) * m_tick_size;
    }

    inline bool valid_index(std::size_t index) const noexcept
    {
        return index < m_levels.size();
    }

    inline bool valid_price(double price) const noexcept
    {
        const auto index = price_to_index(price);
        return valid_index(index);
    }

    inline void set_level(double price, double quantity) noexcept
    {
        const auto index = price_to_index(price);

        if (index < m_levels.size())
        {
            m_levels[index] = quantity;
        }
    }

    inline double get_quantity(double price) const noexcept
    {
        const auto index = price_to_index(price);

        if (index >= m_levels.size())
        {
            return 0.0;
        }

        return m_levels[index];
    }

    inline void remove_level(double price) noexcept
    {
        set_level(price, 0.0);
    }

    inline double quantity_at_index(std::size_t index) const noexcept
    {
        return index < m_levels.size() ? m_levels[index] : 0.0;
    }

    inline std::size_t size() const noexcept
    {
        return m_levels.size();
    }

    inline double base_price() const noexcept
    {
        return m_base_price;
    }

    inline double tick_size() const noexcept
    {
        return m_tick_size;
    }

    void reset_base_price(double new_base_price)
    {
        m_base_price = new_base_price;
        std::fill(m_levels.begin(), m_levels.end(), 0.0);
    }

private:
    double m_base_price;
    double m_tick_size;
    double m_inv_tick_size;

    std::size_t m_center_index;
    std::vector<double> m_levels;
};
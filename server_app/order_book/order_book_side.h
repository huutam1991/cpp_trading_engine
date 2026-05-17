#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <limits>

class OrderBookSide {
public:
    OrderBookSide(
        double base_price,
        double tick_size,
        std::size_t depth
    )
        : base_price_(base_price),
          tick_size_(tick_size),
          inv_tick_size_(1.0 / tick_size),
          center_index_(depth / 2),
          levels_(depth, 0.0)
    {
        if (tick_size <= 0.0) {
            throw std::invalid_argument("tick_size must be positive");
        }

        if (depth == 0) {
            throw std::invalid_argument("depth must be positive");
        }
    }

    inline std::size_t price_to_index(double price) const noexcept {
        const double diff = price - base_price_;
        const auto tick_offset = static_cast<int64_t>(std::llround(diff * inv_tick_size_));
        return static_cast<std::size_t>(
            static_cast<int64_t>(center_index_) + tick_offset
        );
    }

    inline double index_to_price(std::size_t index) const noexcept {
        const auto tick_offset =
            static_cast<int64_t>(index) - static_cast<int64_t>(center_index_);

        return base_price_ + static_cast<double>(tick_offset) * tick_size_;
    }

    inline bool valid_index(std::size_t index) const noexcept {
        return index < levels_.size();
    }

    inline bool valid_price(double price) const noexcept {
        const auto index = price_to_index(price);
        return valid_index(index);
    }

    inline void set_level(double price, double quantity) noexcept {
        const auto index = price_to_index(price);

        if (index < levels_.size()) {
            levels_[index] = quantity;
        }
    }

    inline double get_quantity(double price) const noexcept {
        const auto index = price_to_index(price);

        if (index >= levels_.size()) {
            return 0.0;
        }

        return levels_[index];
    }

    inline void remove_level(double price) noexcept {
        set_level(price, 0.0);
    }

    inline double quantity_at_index(std::size_t index) const noexcept {
        return index < levels_.size() ? levels_[index] : 0.0;
    }

    inline std::size_t size() const noexcept {
        return levels_.size();
    }

    inline double base_price() const noexcept {
        return base_price_;
    }

    inline double tick_size() const noexcept {
        return tick_size_;
    }

    void reset_base_price(double new_base_price) {
        base_price_ = new_base_price;
        std::fill(levels_.begin(), levels_.end(), 0.0);
    }

private:
    double base_price_;
    double tick_size_;
    double inv_tick_size_;

    std::size_t center_index_;
    std::vector<double> levels_;
};
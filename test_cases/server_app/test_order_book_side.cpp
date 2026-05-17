#include <gtest/gtest.h>
#include <order_book/order_book_side.h>

static constexpr double EPS = 1e-12;

TEST(OrderBookSideTest, ConstructorInitialization)
{
    OrderBookSide book(1000.0, 0.5, 100);

    ASSERT_DOUBLE_EQ(book.base_price(), 1000.0);
    ASSERT_DOUBLE_EQ(book.tick_size(), 0.5);
    ASSERT_EQ(book.size(), 100u);
}

TEST(OrderBookSideTest, PriceToIndexCenterPrice)
{
    OrderBookSide book(1000.0, 0.5, 100);

    const auto index = book.price_to_index(1000.0);

    ASSERT_EQ(index, 50u);
}

TEST(OrderBookSideTest, IndexToPriceCenterIndex)
{
    OrderBookSide book(1000.0, 0.5, 100);

    const double price = book.index_to_price(50);

    ASSERT_NEAR(price, 1000.0, EPS);
}

TEST(OrderBookSideTest, PriceToIndexPositiveOffset)
{
    OrderBookSide book(1000.0, 0.5, 100);

    const auto index = book.price_to_index(1001.0);

    // +2 ticks
    ASSERT_EQ(index, 52u);
}

TEST(OrderBookSideTest, PriceToIndexNegativeOffset)
{
    OrderBookSide book(1000.0, 0.5, 100);

    const auto index = book.price_to_index(999.0);

    // -2 ticks
    ASSERT_EQ(index, 48u);
}

TEST(OrderBookSideTest, SetAndGetLevel)
{
    OrderBookSide book(1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);

    const double qty = book.get_quantity(1001.0);

    ASSERT_NEAR(qty, 5.25, EPS);
}

TEST(OrderBookSideTest, OverwriteExistingLevel)
{
    OrderBookSide book(1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);
    book.set_level(1001.0, 9.75);

    const double qty = book.get_quantity(1001.0);

    ASSERT_NEAR(qty, 9.75, EPS);
}

TEST(OrderBookSideTest, RemoveLevel)
{
    OrderBookSide book(1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);

    ASSERT_NEAR(book.get_quantity(1001.0), 5.25, EPS);

    book.remove_level(1001.0);

    ASSERT_NEAR(book.get_quantity(1001.0), 0.0, EPS);
}

TEST(OrderBookSideTest, InvalidPriceReturnsZeroQuantity)
{
    OrderBookSide book(1000.0, 0.5, 100);

    // way outside range
    const double qty = book.get_quantity(2000.0);

    ASSERT_NEAR(qty, 0.0, EPS);
}

TEST(OrderBookSideTest, ValidPriceCheck)
{
    OrderBookSide book(1000.0, 0.5, 100);

    ASSERT_TRUE(book.valid_price(1000.0));
    ASSERT_TRUE(book.valid_price(1010.0));

    ASSERT_FALSE(book.valid_price(2000.0));
}

TEST(OrderBookSideTest, IndexToPriceRoundTrip)
{
    OrderBookSide book(1000.0, 0.25, 1000);

    const double original_price = 1003.75;

    const auto index = book.price_to_index(original_price);
    const double restored_price = book.index_to_price(index);

    ASSERT_NEAR(restored_price, original_price, EPS);
}

TEST(OrderBookSideTest, QuantityAtIndex)
{
    OrderBookSide book(1000.0, 0.5, 100);

    book.set_level(1001.0, 7.0);

    const auto index = book.price_to_index(1001.0);

    ASSERT_NEAR(book.quantity_at_index(index), 7.0, EPS);
}

TEST(OrderBookSideTest, ResetBasePriceClearsBook)
{
    OrderBookSide book(1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);

    ASSERT_NEAR(book.get_quantity(1001.0), 5.0, EPS);

    book.reset_base_price(2000.0);

    ASSERT_NEAR(book.get_quantity(1001.0), 0.0, EPS);

    ASSERT_DOUBLE_EQ(book.base_price(), 2000.0);
}

TEST(OrderBookSideTest, MultipleLevels)
{
    OrderBookSide book(1000.0, 0.1, 1000);

    book.set_level(999.9, 1.0);
    book.set_level(1000.0, 2.0);
    book.set_level(1000.1, 3.0);
    book.set_level(1000.2, 4.0);

    ASSERT_NEAR(book.get_quantity(999.9), 1.0, EPS);
    ASSERT_NEAR(book.get_quantity(1000.0), 2.0, EPS);
    ASSERT_NEAR(book.get_quantity(1000.1), 3.0, EPS);
    ASSERT_NEAR(book.get_quantity(1000.2), 4.0, EPS);
}

TEST(OrderBookSideTest, FloatingPointTickPrecision)
{
    OrderBookSide book(1000.0, 0.01, 10000);

    const double price = 1000.37;

    book.set_level(price, 8.5);

    ASSERT_NEAR(book.get_quantity(price), 8.5, EPS);

    const auto index = book.price_to_index(price);

    ASSERT_NEAR(book.index_to_price(index), price, EPS);
}
#include <gtest/gtest.h>
#include <order_book/order_book_side.h>

static constexpr double EPS = 1e-12;

TEST(OrderBookSideTest, ConstructorInitializationBid)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_EQ(book.side(), OrderBookSideType::Bid);
    ASSERT_DOUBLE_EQ(book.base_price(), 1000.0);
    ASSERT_DOUBLE_EQ(book.tick_size(), 0.5);
    ASSERT_EQ(book.size(), 100u);
}

TEST(OrderBookSideTest, ConstructorInitializationAsk)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    ASSERT_EQ(book.side(), OrderBookSideType::Ask);
    ASSERT_DOUBLE_EQ(book.base_price(), 1000.0);
    ASSERT_DOUBLE_EQ(book.tick_size(), 0.5);
    ASSERT_EQ(book.size(), 100u);
}

TEST(OrderBookSideTest, PriceToIndexCenterPrice)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_EQ(book.price_to_index(1000.0), 50u);
}

TEST(OrderBookSideTest, IndexToPriceCenterIndex)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_NEAR(book.index_to_price(50), 1000.0, EPS);
}

TEST(OrderBookSideTest, PriceToIndexPositiveOffset)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_EQ(book.price_to_index(1001.0), 52u);
}

TEST(OrderBookSideTest, PriceToIndexNegativeOffset)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_EQ(book.price_to_index(999.0), 48u);
}

TEST(OrderBookSideTest, SetAndGetLevel)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);

    ASSERT_NEAR(book.get_quantity(1001.0), 5.25, EPS);
}

TEST(OrderBookSideTest, OverwriteExistingLevel)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);
    book.set_level(1001.0, 9.75);

    ASSERT_NEAR(book.get_quantity(1001.0), 9.75, EPS);
}

TEST(OrderBookSideTest, RemoveLevel)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.25);
    ASSERT_NEAR(book.get_quantity(1001.0), 5.25, EPS);

    book.remove_level(1001.0);
    ASSERT_NEAR(book.get_quantity(1001.0), 0.0, EPS);
}

TEST(OrderBookSideTest, InvalidPriceReturnsZeroQuantity)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_NEAR(book.get_quantity(2000.0), 0.0, EPS);
}

TEST(OrderBookSideTest, ValidPriceCheck)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_TRUE(book.valid_price(1000.0));
    ASSERT_TRUE(book.valid_price(1010.0));
    ASSERT_FALSE(book.valid_price(2000.0));
}

TEST(OrderBookSideTest, IndexToPriceRoundTrip)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.25, 1000);

    const double original_price = 1003.75;
    const auto index = book.price_to_index(original_price);

    ASSERT_NEAR(book.index_to_price(index), original_price, EPS);
}

TEST(OrderBookSideTest, QuantityAtIndex)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 7.0);

    const auto index = book.price_to_index(1001.0);

    ASSERT_NEAR(book.quantity_at_index(index), 7.0, EPS);
}

TEST(OrderBookSideTest, ResetBasePriceClearsBook)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    ASSERT_NEAR(book.get_quantity(1001.0), 5.0, EPS);

    book.reset_base_price(2000.0);

    ASSERT_NEAR(book.get_quantity(1001.0), 0.0, EPS);
    ASSERT_DOUBLE_EQ(book.base_price(), 2000.0);
}

TEST(OrderBookSideTest, MultipleLevels)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.1, 1000);

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
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.01, 10000);

    const double price = 1000.37;

    book.set_level(price, 8.5);

    ASSERT_NEAR(book.get_quantity(price), 8.5, EPS);

    const auto index = book.price_to_index(price);

    ASSERT_NEAR(book.index_to_price(index), price, EPS);
}

TEST(OrderBookSideTest, EmptyBookHasNoTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    ASSERT_FALSE(book.has_top());
    ASSERT_DOUBLE_EQ(book.get_top_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.get_top_quantity(), 0.0);
}

TEST(OrderBookSideTest, SingleLevelBecomesTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, BidHigherPriceBecomesTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, BidLowerPriceDoesNotReplaceTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1002.0, 3.0);
    book.set_level(1001.0, 5.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, BidOverwriteTopQuantityKeepsSameTopPrice)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1002.0, 3.0);
    book.set_level(1002.0, 9.5);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 9.5, EPS);
}

TEST(OrderBookSideTest, BidRemoveNonTopDoesNotChangeTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);

    book.remove_level(1001.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, BidRemoveTopFallsBackToNextHighestPrice)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);
    book.set_level(1003.0, 7.0);

    book.remove_level(1003.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, BidSettingTopQuantityToZeroFallsBack)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);

    book.set_level(1002.0, 0.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, AskLowerPriceBecomesTop)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1002.0, 3.0);
    book.set_level(1001.0, 5.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, AskHigherPriceDoesNotReplaceTop)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, AskRemoveTopFallsBackToNextLowestPrice)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);
    book.set_level(1003.0, 7.0);

    book.remove_level(1001.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, AskRemoveNonTopDoesNotChangeTop)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);

    book.remove_level(1002.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, AskSettingTopQuantityToZeroFallsBack)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(1002.0, 3.0);
    book.set_level(1003.0, 7.0);

    book.set_level(1001.0, 0.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 3.0, EPS);
}

TEST(OrderBookSideTest, AskRemoveOnlyTopLeavesBookEmpty)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.remove_level(1001.0);

    ASSERT_FALSE(book.has_top());
    ASSERT_DOUBLE_EQ(book.get_top_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.get_top_quantity(), 0.0);
}

TEST(OrderBookSideTest, OutOfRangePriceDoesNotAffectTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 5.0);
    book.set_level(2000.0, 99.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 5.0, EPS);
}

TEST(OrderBookSideTest, ResetBasePriceClearsTop)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1002.0, 3.0);

    ASSERT_TRUE(book.has_top());

    book.reset_base_price(2000.0);

    ASSERT_FALSE(book.has_top());
    ASSERT_DOUBLE_EQ(book.get_top_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.get_top_quantity(), 0.0);
}

TEST(OrderBookSideTest, BidTopPriceWorksWithSmallTickSize)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.01, 10000);

    book.set_level(1000.01, 1.0);
    book.set_level(1000.37, 2.0);
    book.set_level(1000.12, 3.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1000.37, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 2.0, EPS);
}

TEST(OrderBookSideTest, AskTopPriceWorksWithSmallTickSize)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.01, 10000);

    book.set_level(1000.37, 2.0);
    book.set_level(1000.12, 3.0);
    book.set_level(1000.01, 1.0);

    ASSERT_TRUE(book.has_top());
    ASSERT_NEAR(book.get_top_price(), 1000.01, EPS);
    ASSERT_NEAR(book.get_top_quantity(), 1.0, EPS);
}

TEST(OrderBookSideTest, BidTopCanMoveDownAfterRemovingHigherPrices)
{
    OrderBookSide book(OrderBookSideType::Bid, 1000.0, 0.5, 100);

    book.set_level(1001.0, 1.0);
    book.set_level(1002.0, 2.0);
    book.set_level(1003.0, 3.0);

    ASSERT_NEAR(book.get_top_price(), 1003.0, EPS);

    book.remove_level(1003.0);
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);

    book.remove_level(1002.0);
    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);
}

TEST(OrderBookSideTest, AskTopCanMoveUpAfterRemovingLowerPrices)
{
    OrderBookSide book(OrderBookSideType::Ask, 1000.0, 0.5, 100);

    book.set_level(1001.0, 1.0);
    book.set_level(1002.0, 2.0);
    book.set_level(1003.0, 3.0);

    ASSERT_NEAR(book.get_top_price(), 1001.0, EPS);

    book.remove_level(1001.0);
    ASSERT_NEAR(book.get_top_price(), 1002.0, EPS);

    book.remove_level(1002.0);
    ASSERT_NEAR(book.get_top_price(), 1003.0, EPS);
}
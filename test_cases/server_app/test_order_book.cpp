#include <gtest/gtest.h>
#include <order_book/order_book.h>

static constexpr double EPS = 1e-12;

TEST(OrderBookTest, ConstructorInitialization)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    ASSERT_DOUBLE_EQ(book.base_price(), 1000.0);
    ASSERT_DOUBLE_EQ(book.tick_size(), 0.5);
    ASSERT_EQ(book.depth(), 100u);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());
    ASSERT_FALSE(book.has_spread());
}

TEST(OrderBookTest, SetBidUpdatesBestBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_bid(999.0, 20.0);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, SetAskUpdatesBestAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_ask(1001.0, 20.0);
    book.set_ask(1000.5, 10.0);

    ASSERT_TRUE(book.has_best_ask());
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, BidAndAskSpreadAndMidPrice)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    ASSERT_TRUE(book.has_spread());
    ASSERT_NEAR(book.spread(), 1.0, EPS);
    ASSERT_NEAR(book.mid_price(), 1000.0, EPS);
}

TEST(OrderBookTest, GetBidAndAskQuantity)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    ASSERT_NEAR(book.get_bid_quantity(999.5), 10.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(1000.5), 20.0, EPS);
}

TEST(OrderBookTest, RemoveBidFallsBackToNextBestBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.0, 10.0);
    book.set_bid(999.5, 20.0);

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);

    book.remove_bid(999.5);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_NEAR(book.best_bid_price(), 999.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, RemoveAskFallsBackToNextBestAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_ask(1000.5, 10.0);
    book.set_ask(1001.0, 20.0);

    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);

    book.remove_ask(1000.5);

    ASSERT_TRUE(book.has_best_ask());
    ASSERT_NEAR(book.best_ask_price(), 1001.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 20.0, EPS);
}

TEST(OrderBookTest, ApplyUpdateAddBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookUpdate update{
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    };

    book.apply_update(update);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, ApplyUpdateUpdateAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.5,
        10.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Update,
        1000.5,
        25.0
    });

    ASSERT_TRUE(book.has_best_ask());
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 25.0, EPS);
}

TEST(OrderBookTest, ApplyUpdateRemoveBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    });

    ASSERT_TRUE(book.has_best_bid());

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Remove,
        999.5,
        0.0
    });

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_DOUBLE_EQ(book.best_bid_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.best_bid_quantity(), 0.0);
}

TEST(OrderBookTest, CrossedBookDetection)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(1001.0, 10.0);
    book.set_ask(1000.5, 20.0);

    ASSERT_TRUE(book.has_spread());
    ASSERT_TRUE(book.crossed());
    ASSERT_NEAR(book.spread(), -0.5, EPS);
}

TEST(OrderBookTest, ResetBasePriceClearsBothSides)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    book.reset_base_price(2000.0);

    ASSERT_DOUBLE_EQ(book.base_price(), 2000.0);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());
    ASSERT_FALSE(book.has_spread());

    ASSERT_DOUBLE_EQ(book.best_bid_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.best_ask_price(), 0.0);
}

TEST(OrderBookTest, EmptyBookSpreadMidAndCrossed)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());

    ASSERT_FALSE(book.has_spread());

    ASSERT_DOUBLE_EQ(book.spread(), 0.0);
    ASSERT_DOUBLE_EQ(book.mid_price(), 0.0);

    ASSERT_FALSE(book.crossed());
}

TEST(OrderBookTest, OnlyBidNoAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());

    ASSERT_FALSE(book.has_spread());

    ASSERT_DOUBLE_EQ(book.spread(), 0.0);
    ASSERT_DOUBLE_EQ(book.mid_price(), 0.0);

    ASSERT_FALSE(book.crossed());
}

TEST(OrderBookTest, OnlyAskNoBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_ask(1000.5, 10.0);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    ASSERT_FALSE(book.has_spread());

    ASSERT_DOUBLE_EQ(book.spread(), 0.0);
    ASSERT_DOUBLE_EQ(book.mid_price(), 0.0);

    ASSERT_FALSE(book.crossed());
}

TEST(OrderBookTest, RemoveNonTopBidDoesNotChangeBestBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.0, 5.0);
    book.set_bid(999.5, 10.0);

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);

    book.remove_bid(999.0);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, RemoveNonTopAskDoesNotChangeBestAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_ask(1000.5, 10.0);
    book.set_ask(1001.0, 5.0);

    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);

    book.remove_ask(1001.0);

    ASSERT_TRUE(book.has_best_ask());
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 10.0, EPS);
}

TEST(OrderBookTest, OutOfRangeBidAndAskUpdatesDoNotAffectBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    const double best_bid_before = book.best_bid_price();
    const double best_ask_before = book.best_ask_price();

    book.set_bid(5000.0, 999.0);
    book.set_ask(-5000.0, 999.0);

    ASSERT_NEAR(book.best_bid_price(), best_bid_before, EPS);
    ASSERT_NEAR(book.best_ask_price(), best_ask_before, EPS);

    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 20.0, EPS);
}

TEST(OrderBookTest, UpdateQuantityZeroBehavesLikeRemove)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    });

    ASSERT_TRUE(book.has_best_bid());

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Update,
        999.5,
        0.0
    });

    ASSERT_FALSE(book.has_best_bid());

    ASSERT_DOUBLE_EQ(book.best_bid_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.best_bid_quantity(), 0.0);
}

TEST(OrderBookTest, MultipleMixedBidAskUpdates)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.0,
        5.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1001.0,
        8.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.5,
        12.0
    });

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Remove,
        999.5,
        0.0
    });

    ASSERT_NEAR(book.best_bid_price(), 999.0, EPS);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Remove,
        1000.5,
        0.0
    });

    ASSERT_NEAR(book.best_ask_price(), 1001.0, EPS);
}

TEST(OrderBookTest, BidAndAskAccessorsReturnCorrectSides)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    ASSERT_EQ(book.bids().side(), OrderBookSideType::Bid);
    ASSERT_EQ(book.asks().side(), OrderBookSideType::Ask);

    book.mutable_bids().set_level(999.5, 10.0);
    book.mutable_asks().set_level(1000.5, 20.0);

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
}

TEST(OrderBookTest, AddAndUpdateProduceSameFinalState)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.5,
        10.0
    });

    ASSERT_NEAR(book.best_ask_quantity(), 10.0, EPS);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Update,
        1000.5,
        25.0
    });

    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 25.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotBuildsBidAndAskBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(999.5, 10.0);
    snapshot.add_bid(999.0, 5.0);

    snapshot.add_ask(1000.5, 20.0);
    snapshot.add_ask(1001.0, 8.0);

    book.apply_update(snapshot);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);

    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 20.0, EPS);

    ASSERT_NEAR(book.spread(), 1.0, EPS);
    ASSERT_NEAR(book.mid_price(), 1000.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotClearsOldBookState)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    OrderBookSnapShot snapshot;

    snapshot.add_bid(998.5, 7.0);
    snapshot.add_ask(1002.0, 9.0);

    book.apply_update(snapshot);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    ASSERT_NEAR(book.best_bid_price(), 998.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 7.0, EPS);

    ASSERT_NEAR(book.best_ask_price(), 1002.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 9.0, EPS);

    ASSERT_NEAR(book.get_bid_quantity(999.5), 0.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(1000.5), 0.0, EPS);
}

TEST(OrderBookTest, ApplyEmptySnapshotClearsBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    OrderBookSnapShot snapshot;

    book.apply_update(snapshot);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());
    ASSERT_FALSE(book.has_spread());

    ASSERT_DOUBLE_EQ(book.best_bid_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.best_ask_price(), 0.0);
    ASSERT_DOUBLE_EQ(book.spread(), 0.0);
    ASSERT_DOUBLE_EQ(book.mid_price(), 0.0);
}

TEST(OrderBookTest, ApplySnapshotThenIncrementalUpdateBid)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(999.0, 5.0);
    snapshot.add_bid(999.5, 10.0);

    snapshot.add_ask(1000.5, 20.0);
    snapshot.add_ask(1001.0, 8.0);

    book.apply_update(snapshot);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Update,
        999.5,
        30.0
    });

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 30.0, EPS);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        1000.0,
        12.0
    });

    ASSERT_NEAR(book.best_bid_price(), 1000.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 12.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotThenIncrementalUpdateAsk)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(999.5, 10.0);
    snapshot.add_ask(1001.0, 8.0);
    snapshot.add_ask(1000.5, 20.0);

    book.apply_update(snapshot);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Update,
        1000.5,
        30.0
    });

    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 30.0, EPS);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.0,
        6.0
    });

    ASSERT_NEAR(book.best_ask_price(), 1000.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 6.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotThenIncrementalRemoveBidTop)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(999.5, 10.0);
    snapshot.add_bid(999.0, 5.0);
    snapshot.add_ask(1000.5, 20.0);

    book.apply_update(snapshot);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Remove,
        999.5,
        0.0
    });

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_NEAR(book.best_bid_price(), 999.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 5.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotThenIncrementalRemoveAskTop)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(999.5, 10.0);
    snapshot.add_ask(1000.5, 20.0);
    snapshot.add_ask(1001.0, 8.0);

    book.apply_update(snapshot);

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Remove,
        1000.5,
        0.0
    });

    ASSERT_TRUE(book.has_best_ask());
    ASSERT_NEAR(book.best_ask_price(), 1001.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 8.0, EPS);
}

TEST(OrderBookTest, IncrementalUpdatesThenApplySnapshotReplacesEverything)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.5,
        20.0
    });

    ASSERT_TRUE(book.has_spread());

    OrderBookSnapShot snapshot;

    snapshot.add_bid(998.5, 7.0);
    snapshot.add_ask(1002.0, 9.0);

    book.apply_update(snapshot);

    ASSERT_NEAR(book.best_bid_price(), 998.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 7.0, EPS);

    ASSERT_NEAR(book.best_ask_price(), 1002.0, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 9.0, EPS);

    ASSERT_NEAR(book.get_bid_quantity(999.5), 0.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(1000.5), 0.0, EPS);
}

TEST(OrderBookTest, RebaseTriggeredNearUpperBoundaryPreservesBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    book.set_bid(1023.0, 5.0);

    ASSERT_NEAR(book.base_price(), 1023.0, EPS);

    ASSERT_NEAR(book.get_bid_quantity(999.5), 10.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(1000.5), 20.0, EPS);
    ASSERT_NEAR(book.get_bid_quantity(1023.0), 5.0, EPS);

    ASSERT_NEAR(book.best_bid_price(), 1023.0, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
}

TEST(OrderBookTest, RebaseTriggeredNearLowerBoundaryPreservesBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    book.set_ask(977.0, 5.0);

    ASSERT_NEAR(book.base_price(), 999.5, EPS);

    ASSERT_NEAR(book.get_bid_quantity(999.5), 10.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(1000.5), 20.0, EPS);
    ASSERT_NEAR(book.get_ask_quantity(977.0), 5.0, EPS);

    ASSERT_NEAR(book.best_bid_price(), 999.5, EPS);
    ASSERT_NEAR(book.best_ask_price(), 977.0, EPS);
}

TEST(OrderBookTest, PriceInsideRangeButNotNearBoundaryDoesNotRebase)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    const double old_base = book.base_price();

    book.set_bid(1001.0, 5.0);

    ASSERT_NEAR(book.base_price(), old_base, EPS);

    ASSERT_NEAR(book.best_bid_price(), 1001.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 5.0, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
}

TEST(OrderBookTest, OutOfRangePriceDoesNotTriggerRebase)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    const double old_base = book.base_price();
    const double old_best_bid = book.best_bid_price();
    const double old_best_ask = book.best_ask_price();

    book.set_bid(5000.0, 99.0);
    book.set_ask(-5000.0, 99.0);

    ASSERT_NEAR(book.base_price(), old_base, EPS);

    ASSERT_NEAR(book.best_bid_price(), old_best_bid, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);

    ASSERT_NEAR(book.best_ask_price(), old_best_ask, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 20.0, EPS);
}

TEST(OrderBookTest, RebaseTriggeredByOrderBookUpdateAdd)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        999.5,
        10.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Ask,
        OrderBookUpdateType::Add,
        1000.5,
        20.0
    });

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Add,
        1023.0,
        5.0
    });

    ASSERT_NEAR(book.base_price(), 1023.0, EPS);

    ASSERT_NEAR(book.best_bid_price(), 1023.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 5.0, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
}

TEST(OrderBookTest, RebaseTriggeredByOrderBookUpdateRemovePreservesBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_bid(1000.0, 12.0);
    book.set_ask(1000.5, 20.0);

    book.apply_update({
        nullptr,
        OrderBookSideType::Bid,
        OrderBookUpdateType::Remove,
        1023.0,
        0.0
    });

    ASSERT_NEAR(book.base_price(), 1000.0, EPS);

    ASSERT_NEAR(book.best_bid_price(), 1000.0, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 12.0, EPS);
    ASSERT_NEAR(book.best_ask_price(), 1000.5, EPS);
}

TEST(OrderBookTest, ApplySnapshotNearBoundaryTriggersRebase)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(1022.5, 10.0);
    snapshot.add_ask(1023.5, 20.0);

    book.apply_update(snapshot);

    ASSERT_NEAR(book.base_price(), 1022.5, EPS);

    ASSERT_TRUE(book.has_best_bid());
    ASSERT_TRUE(book.has_best_ask());

    ASSERT_NEAR(book.best_bid_price(), 1022.5, EPS);
    ASSERT_NEAR(book.best_bid_quantity(), 10.0, EPS);

    ASSERT_NEAR(book.best_ask_price(), 1023.5, EPS);
    ASSERT_NEAR(book.best_ask_quantity(), 20.0, EPS);
}

TEST(OrderBookTest, ApplySnapshotOutOfRangeWithoutValidReferenceStillClearsAndDropsOutOfRange)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    OrderBookSnapShot snapshot;

    snapshot.add_bid(5000.0, 10.0);
    snapshot.add_ask(5000.5, 20.0);

    book.apply_update(snapshot);

    ASSERT_NEAR(book.base_price(), 1000.0, EPS);

    ASSERT_FALSE(book.has_best_bid());
    ASSERT_FALSE(book.has_best_ask());
}

TEST(OrderBookTest, GetOrderBookSnapshotEmptyBook)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 0u);
    ASSERT_EQ(snapshot->asks_size, 0u);

    ASSERT_DOUBLE_EQ(snapshot->get_best_bid(), 0.0);
    ASSERT_DOUBLE_EQ(snapshot->get_best_ask(), 0.0);
    ASSERT_DOUBLE_EQ(snapshot->get_mid_price(), 0.0);
}

TEST(OrderBookTest, GetOrderBookSnapshotReturnsBestBidAndAskFirst)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(998.5, 1.0);
    book.set_bid(999.5, 3.0);
    book.set_bid(999.0, 2.0);

    book.set_ask(1001.5, 3.0);
    book.set_ask(1000.5, 1.0);
    book.set_ask(1001.0, 2.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 3u);
    ASSERT_EQ(snapshot->asks_size, 3u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[0].quantity, 3.0, EPS);

    ASSERT_NEAR(snapshot->bids[1].price, 999.0, EPS);
    ASSERT_NEAR(snapshot->bids[1].quantity, 2.0, EPS);

    ASSERT_NEAR(snapshot->bids[2].price, 998.5, EPS);
    ASSERT_NEAR(snapshot->bids[2].quantity, 1.0, EPS);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[0].quantity, 1.0, EPS);

    ASSERT_NEAR(snapshot->asks[1].price, 1001.0, EPS);
    ASSERT_NEAR(snapshot->asks[1].quantity, 2.0, EPS);

    ASSERT_NEAR(snapshot->asks[2].price, 1001.5, EPS);
    ASSERT_NEAR(snapshot->asks[2].quantity, 3.0, EPS);
}

TEST(OrderBookTest, GetOrderBookSnapshotRespectsRequestedLevels)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(998.5, 1.0);
    book.set_bid(999.0, 2.0);
    book.set_bid(999.5, 3.0);

    book.set_ask(1000.5, 1.0);
    book.set_ask(1001.0, 2.0);
    book.set_ask(1001.5, 3.0);

    auto snapshot = book.get_order_book_snapshot(2);

    ASSERT_EQ(snapshot->bids_size, 2u);
    ASSERT_EQ(snapshot->asks_size, 2u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[1].price, 999.0, EPS);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[1].price, 1001.0, EPS);
}

TEST(OrderBookTest, GetOrderBookSnapshotWithZeroLevels)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    auto snapshot = book.get_order_book_snapshot(0);

    ASSERT_EQ(snapshot->bids_size, 0u);
    ASSERT_EQ(snapshot->asks_size, 0u);

    ASSERT_DOUBLE_EQ(snapshot->get_best_bid(), 0.0);
    ASSERT_DOUBLE_EQ(snapshot->get_best_ask(), 0.0);
}

TEST(OrderBookTest, GetOrderBookSnapshotOnlyBidSide)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_bid(999.0, 5.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 2u);
    ASSERT_EQ(snapshot->asks_size, 0u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[0].quantity, 10.0, EPS);

    ASSERT_NEAR(snapshot->bids[1].price, 999.0, EPS);
    ASSERT_NEAR(snapshot->bids[1].quantity, 5.0, EPS);

    ASSERT_DOUBLE_EQ(snapshot->get_best_ask(), 0.0);
    ASSERT_DOUBLE_EQ(snapshot->get_mid_price(), 0.0);
}

TEST(OrderBookTest, GetOrderBookSnapshotOnlyAskSide)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_ask(1000.5, 10.0);
    book.set_ask(1001.0, 5.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 0u);
    ASSERT_EQ(snapshot->asks_size, 2u);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[0].quantity, 10.0, EPS);

    ASSERT_NEAR(snapshot->asks[1].price, 1001.0, EPS);
    ASSERT_NEAR(snapshot->asks[1].quantity, 5.0, EPS);

    ASSERT_DOUBLE_EQ(snapshot->get_best_bid(), 0.0);
    ASSERT_DOUBLE_EQ(snapshot->get_mid_price(), 0.0);
}

TEST(OrderBookTest, GetOrderBookSnapshotSkipsRemovedLevels)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(998.5, 1.0);
    book.set_bid(999.0, 2.0);
    book.set_bid(999.5, 3.0);

    book.set_ask(1000.5, 1.0);
    book.set_ask(1001.0, 2.0);
    book.set_ask(1001.5, 3.0);

    book.remove_bid(999.0);
    book.remove_ask(1001.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 2u);
    ASSERT_EQ(snapshot->asks_size, 2u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[1].price, 998.5, EPS);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[1].price, 1001.5, EPS);
}

TEST(OrderBookTest, GetOrderBookSnapshotReflectsUpdatedQuantity)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    book.set_bid(999.5, 15.0);
    book.set_ask(1000.5, 25.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 1u);
    ASSERT_EQ(snapshot->asks_size, 1u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[0].quantity, 15.0, EPS);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[0].quantity, 25.0, EPS);
}

TEST(OrderBookTest, GetOrderBookSnapshotAfterRebasePreservesSortedOrder)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100, 2.0);

    book.set_bid(999.5, 10.0);
    book.set_bid(999.0, 5.0);

    book.set_ask(1000.5, 20.0);
    book.set_ask(1001.0, 8.0);

    book.set_bid(1023.0, 3.0);

    auto snapshot = book.get_order_book_snapshot(10);

    ASSERT_EQ(snapshot->bids_size, 3u);
    ASSERT_EQ(snapshot->asks_size, 2u);

    ASSERT_NEAR(snapshot->bids[0].price, 1023.0, EPS);
    ASSERT_NEAR(snapshot->bids[0].quantity, 3.0, EPS);

    ASSERT_NEAR(snapshot->bids[1].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->bids[1].quantity, 10.0, EPS);

    ASSERT_NEAR(snapshot->bids[2].price, 999.0, EPS);
    ASSERT_NEAR(snapshot->bids[2].quantity, 5.0, EPS);

    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
    ASSERT_NEAR(snapshot->asks[0].quantity, 20.0, EPS);

    ASSERT_NEAR(snapshot->asks[1].price, 1001.0, EPS);
    ASSERT_NEAR(snapshot->asks[1].quantity, 8.0, EPS);
}

TEST(OrderBookTest, GetOrderBookSnapshotLevelGreaterThanExistingLevels)
{
    OrderBook book(nullptr, 1000.0, 0.5, 100);

    book.set_bid(999.5, 10.0);
    book.set_ask(1000.5, 20.0);

    auto snapshot = book.get_order_book_snapshot(50);

    ASSERT_EQ(snapshot->bids_size, 1u);
    ASSERT_EQ(snapshot->asks_size, 1u);

    ASSERT_NEAR(snapshot->bids[0].price, 999.5, EPS);
    ASSERT_NEAR(snapshot->asks[0].price, 1000.5, EPS);
}
#include <gtest/gtest.h>
#include <pnl/pnl.h>

static constexpr double EPS = 1e-12;

TEST(PnLTest, NoTrade)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 0.0, 0.1);   // no-op
    pnl.update_current_price(123.0);
    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0); // 0 * (123 - anything) = 0
}

TEST(PnLTest, OpenLongPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);  // long 10 @ 100
    pnl.update_current_price(105.0);
    ASSERT_DOUBLE_EQ(pnl.volume, 10.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 100.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 10.0 * (105.0 - 100.0)); // = 50
}

TEST(PnLTest, OpenShortPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(200.0, -5.0, 0.0);  // short 5 @ 200
    pnl.update_current_price(190.0);
    ASSERT_DOUBLE_EQ(pnl.volume, -5.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 200.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, -5.0 * (190.0 - 200.0)); // = 50
}

TEST(PnLTest, IncreaseLongPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);
    pnl.update_trade(110.0, 5.0, 0.0);   // increase long
    pnl.update_current_price(120.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 15.0);
    double expected_avg = (100.0 * 10.0 + 110.0 * 5.0) / 15.0;
    ASSERT_NEAR(pnl.avg_price, expected_avg, EPS);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_NEAR(pnl.unrealized, 15.0 * (120.0 - expected_avg), EPS);
}

TEST(PnLTest, IncreaseShortPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(200.0, -5.0, 0.0);
    pnl.update_trade(190.0, -5.0, 0.0);  // increase short
    pnl.update_current_price(180.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -10.0);
    double expected_avg = (200.0 * 5.0 + 190.0 * 5.0) / 10.0; // = 195
    ASSERT_NEAR(pnl.avg_price, expected_avg, EPS);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_NEAR(pnl.unrealized, -10.0 * (180.0 - expected_avg), EPS); // = 150
}

TEST(PnLTest, CloseAllLongPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);
    pnl.update_trade(110.0, -10.0, 0.0); // close all long
    pnl.update_current_price(999.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 0.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 10.0 * (110.0 - 100.0)); // = 100
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0);
}

TEST(PnLTest, CloseAllShortPosition)
{
    PnL pnl(nullptr);
    pnl.update_trade(200.0, -5.0, 0.0);
    pnl.update_trade(190.0, 5.0, 0.0);   // close all short
    pnl.update_current_price(1.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 0.0);
    ASSERT_DOUBLE_EQ(pnl.realized, -5.0 * (190.0 - 200.0)); // = 50
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0);
}

TEST(PnLTest, ClosePartOfLong)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);
    pnl.update_trade(110.0, -4.0, 0.0);  // close 4/10
    pnl.update_current_price(108.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 6.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 100.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 4.0 * (110.0 - 100.0));   // = 40
    ASSERT_DOUBLE_EQ(pnl.unrealized, 6.0 * (108.0 - 100.0)); // = 48
}

TEST(PnLTest, FlipFromLongToShort)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);   // long 10 @100
    pnl.update_trade(90.0, -15.0, 0.0);   // close 10, open short 5 @90
    pnl.update_current_price(80.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -5.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 90.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 10.0 * (90.0 - 100.0)); // = -100
    ASSERT_DOUBLE_EQ(pnl.unrealized, -5.0 * (80.0 - 90.0)); // = 50
}

TEST(PnLTest, FlipFromShortToLong)
{
    PnL pnl(nullptr);
    pnl.update_trade(200.0, -8.0, 0.0);   // short 8 @200
    pnl.update_trade(210.0, 10.0, 0.0);   // close 8, open long 2 @210
    pnl.update_current_price(220.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 2.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 210.0);
    ASSERT_DOUBLE_EQ(pnl.realized, -8.0 * (210.0 - 200.0)); // = -80
    ASSERT_DOUBLE_EQ(pnl.unrealized, 2.0 * (220.0 - 210.0)); // = 20
}

TEST(PnLTest, UnrealizedReflectsLatestMark)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 3.0, 0.0);    // long 3 @100
    pnl.update_current_price(101.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 3.0 * (101.0 - 100.0)); // = 3

    pnl.update_current_price(98.5);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 3.0 * (98.5 - 100.0));  // = -4.5

    pnl.update_trade(100.0, -3.0, 0.0);   // close-all
    pnl.update_current_price(150.0);
    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0); // flat, unrealized = 0
}

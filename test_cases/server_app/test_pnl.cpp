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

TEST(PnLWithFee, OpenLong_WithFee_ChargesRealized)
{
    // Initial open uses the "flip" path when prior volume == 0
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 2.5); // open long 10 @100, fee=2.5
    pnl.update_current_price(105.0);

    // Fee goes to realized (negative), avg_price = price, unrealized mark-to-market
    ASSERT_DOUBLE_EQ(pnl.volume, 10.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 100.0);
    ASSERT_DOUBLE_EQ(pnl.realized, -2.5);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 10.0 * (105.0 - 100.0)); // = 50
}

TEST(PnLWithFee, OpenShort_WithFee_ChargesRealized)
{
    PnL pnl(nullptr);
    pnl.update_trade(200.0, -6.0, 3.0); // open short 6 @200, fee=3
    pnl.update_current_price(190.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -6.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 200.0);
    ASSERT_DOUBLE_EQ(pnl.realized, -3.0);
    ASSERT_DOUBLE_EQ(pnl.unrealized, -6.0 * (190.0 - 200.0)); // = 60
}

TEST(PnLWithFee, IncreaseLong_FeeGoesIntoAveragePrice)
{
    PnL pnl(nullptr);
    // First open without fee (so realized not polluted)
    pnl.update_trade(100.0, 10.0, 0.0);
    // Increase same side: fee added to total_cost, affecting avg_price (not realized)
    pnl.update_trade(110.0, 5.0, 4.0);
    pnl.update_current_price(115.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 15.0);
    double expected_avg = (100.0*10.0 + 110.0*5.0 + 4.0) / 15.0;
    ASSERT_NEAR(pnl.avg_price, expected_avg, EPS);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0); // fee not in realized here
    ASSERT_NEAR(pnl.unrealized, 15.0 * (115.0 - expected_avg), EPS);
}

TEST(PnLWithFee, IncreaseShort_FeeGoesIntoAveragePrice)
{
    PnL pnl(nullptr);
    pnl.update_trade(220.0, -4.0, 0.0);
    pnl.update_trade(210.0, -6.0, 1.5); // increase short with fee
    pnl.update_current_price(200.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -10.0);
    double expected_avg = (220.0 * -4.0 + 210.0 * -6.0 + 1.5) / -10.0;
    ASSERT_NEAR(pnl.avg_price, expected_avg, EPS);
    ASSERT_DOUBLE_EQ(pnl.realized, 0.0);
    ASSERT_NEAR(pnl.unrealized, -10.0 * (200.0 - expected_avg), EPS);
}

TEST(PnLWithFee, CloseAll_Long_WithFeeHitsRealized)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 8.0, 0.0);    // long 8 @100
    pnl.update_trade(112.0, -8.0, 2.0);   // close all with fee
    pnl.update_current_price(130.0);      // irrelevant, flat

    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 0.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 8.0*(112.0 - 100.0) - 2.0); // 96 - 2 = 94
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0);
}

TEST(PnLWithFee, CloseAll_Short_WithFeeHitsRealized)
{
    PnL pnl(nullptr);
    pnl.update_trade(300.0, -5.0, 0.0);   // short 5 @300
    pnl.update_trade(280.0, 5.0, 1.2);    // close all with fee
    pnl.update_current_price(100.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 0.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 0.0);
    // realized = (-5)*(280-300) - 1.2 = 100 - 1.2 = 98.8
    ASSERT_DOUBLE_EQ(pnl.realized, -5.0*(280.0 - 300.0) - 1.2);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 0.0);
}

TEST(PnLWithFee, ClosePart_Long_FeeReducesRealized)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 12.0, 0.0);   // long 12 @100
    pnl.update_trade(110.0, -5.0, 0.8);   // close 5 @110, fee=0.8
    pnl.update_current_price(108.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 7.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 100.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 5.0*(110.0 - 100.0) - 0.8); // 50 - 0.8 = 49.2
    ASSERT_DOUBLE_EQ(pnl.unrealized, 7.0*(108.0 - 100.0));     // = 56
}

TEST(PnLWithFee, ClosePart_Short_FeeReducesRealized)
{
    PnL pnl(nullptr);
    pnl.update_trade(250.0, -10.0, 0.0);  // short 10 @250
    pnl.update_trade(240.0, 4.0, 0.7);    // cover 4 @240, fee=0.7
    pnl.update_current_price(230.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -6.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 250.0);
    // Realized: (-1)*4*(240-250) - 0.7 = 39.3
    ASSERT_NEAR(pnl.realized, 4.0 * (250.0 - 240.0) - 0.7, EPS); // = 39.3
    ASSERT_DOUBLE_EQ(pnl.unrealized, -6.0 * (230.0 - 250.0)); // = 120
}

TEST(PnLWithFee, Flip_LongToShort_FeeHitsRealized_AndNewAvgIsTradePrice)
{
    PnL pnl(nullptr);
    pnl.update_trade(100.0, 10.0, 0.0);    // long 10 @100
    pnl.update_trade(95.0, -14.0, 1.5);    // close 10, open short 4 @95, fee=1.5
    pnl.update_current_price(90.0);

    ASSERT_DOUBLE_EQ(pnl.volume, -4.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 95.0);
    ASSERT_DOUBLE_EQ(pnl.realized, 10.0*(95.0 - 100.0) - 1.5); // -50 - 1.5 = -51.5
    ASSERT_DOUBLE_EQ(pnl.unrealized, -4.0 * (90.0 - 95.0));    // = 20
}

TEST(PnLWithFee, Flip_ShortToLong_FeeHitsRealized_AndNewAvgIsTradePrice)
{
    PnL pnl(nullptr);
    pnl.update_trade(210.0, -6.0, 0.0);    // short 6 @210
    pnl.update_trade(220.0, 9.0, 2.2);     // close 6, open long 3 @220, fee=2.2
    pnl.update_current_price(230.0);

    ASSERT_DOUBLE_EQ(pnl.volume, 3.0);
    ASSERT_DOUBLE_EQ(pnl.avg_price, 220.0);
    // realized = (-6)*(220-210) - 2.2 = -60 - 2.2 = -62.2
    ASSERT_DOUBLE_EQ(pnl.realized, -6.0*(220.0 - 210.0) - 2.2);
    ASSERT_DOUBLE_EQ(pnl.unrealized, 3.0*(230.0 - 220.0));     // = 30
}

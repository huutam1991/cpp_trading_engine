#include <gtest/gtest.h>
#include <pnl/pnl.h>

#include <app_utils/app_utils.h>

static constexpr double EPS = 1e-12;

TEST(AppUtilsTest, ClientOrderIdToSystemOrderIdDeterministic)
{
    const std::string client_order_id = "web_coin_5q7wniqcdycxzoo55z4og3";

    OrderId id1 = AppUtils::client_order_id_to_system_order_id(client_order_id);
    OrderId id2 = AppUtils::client_order_id_to_system_order_id(client_order_id);

    // same input must always generate same output
    ASSERT_EQ(id1, 9061898491642833256);
    ASSERT_EQ(id2, 9061898491642833256);

    // generated id should be positive
    ASSERT_GT(id1, 0);
}

TEST(AppUtilsTest, ClientOrderIdToSystemOrderIdDeterministic2)
{
    const std::string client_order_id = "ios_coin_1JLsLqQEVQSBr3OM4kl7";

    OrderId id1 = AppUtils::client_order_id_to_system_order_id(client_order_id);
    OrderId id2 = AppUtils::client_order_id_to_system_order_id(client_order_id);

    // same input must always generate same output
    ASSERT_EQ(id1, 5555645155965599368);
    ASSERT_EQ(id2, 5555645155965599368);

    // generated id should be positive
    ASSERT_GT(id1, 0);
}

TEST(AppUtilsTest, ClientOrderIdToSystemOrderIdDifferentInputs)
{
    const std::string client_order_id_1 = "web_coin_5q7wniqcdycxzoo55z4og3";
    const std::string client_order_id_2 = "ios_coin_1JLsLqQEVQSBr3OM4kl7";

    OrderId id1 = AppUtils::client_order_id_to_system_order_id(client_order_id_1);
    OrderId id2 = AppUtils::client_order_id_to_system_order_id(client_order_id_2);

    // different inputs should generate different outputs
    ASSERT_NE(id1, id2);
}

TEST(AppUtilsTest, ClientOrderIdToSystemOrderIdEmptyString)
{
    OrderId id = AppUtils::client_order_id_to_system_order_id("");

    // even empty string should generate deterministic non-negative value
    ASSERT_GE(id, 0);
}

TEST(AppUtilsTest, ClientOrderIdToSystemOrderIdLongString)
{
    std::string long_client_order_id(1000, 'A');

    OrderId id1 = AppUtils::client_order_id_to_system_order_id(long_client_order_id);
    OrderId id2 = AppUtils::client_order_id_to_system_order_id(long_client_order_id);

    ASSERT_EQ(id1, id2);
    ASSERT_GT(id1, 0);
}

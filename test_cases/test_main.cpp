#include <gtest/gtest.h>

int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

TEST(FactorialTest, Basic) {
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(5), 120);
}
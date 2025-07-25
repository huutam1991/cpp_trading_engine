#include <gtest/gtest.h>

#include <c_json/json.h>

int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

TEST(FactorialTest, Basic) {
    JsonNew json;
    json["factorial_0"] = factorial(0);
    EXPECT_EQ(json["factorial_0"].get_string_value(), "1");
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(5), 120);
}
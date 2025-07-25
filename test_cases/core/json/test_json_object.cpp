#include <gtest/gtest.h>
#include <c_json/json.h>

TEST(JsonObjectTest, Basic)
{
    std::cout << "Running JsonObjectTest" << std::endl;
    JsonNew j;
    j[1] = "hello";
    EXPECT_EQ((std::string)j[1], "hello");
}
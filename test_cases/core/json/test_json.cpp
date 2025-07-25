#include <gtest/gtest.h>
#include <c_json/json.h>

TEST(JsonNewTest, ComplexObjectStructure)
{
    JsonNew root;
    root["name"] = "ChatGPT";
    root["active"] = true;
    root["version"] = 4.0;
    root["users"] = 1000000;
    root["null_field"] = nullptr;

    JsonNew arr;
    arr[0] = "openai";
    arr[1] = 2023;
    arr[2] = false;
    root["metadata"] = arr;

    bool is_null = root["null_field"] == nullptr;
    bool is_not_null = root["users"] == nullptr;

    ASSERT_EQ((std::string)root["name"], "ChatGPT");
    ASSERT_EQ((bool)root["active"], true);
    ASSERT_EQ((double)root["version"], 4.0);
    ASSERT_EQ((int)root["users"], 1000000);
    ASSERT_EQ(is_null, true);
    ASSERT_EQ(is_not_null, false);
    ASSERT_EQ(root["null_field"], nullptr);
    ASSERT_TRUE(root["null_field"].is_null());

    ASSERT_EQ((std::string)root["metadata"][0], "openai");
    ASSERT_EQ((int)root["metadata"][1], 2023);
    ASSERT_EQ((bool)root["metadata"][2], false);
}

TEST(JsonNewTest, SharedReferenceCopy)
{
    JsonNew original;
    original["value"] = "test";

    JsonNew copy = original; // shallow copy, shared underlying data

    ASSERT_EQ((std::string)copy["value"], "test");

    // Check that they point to same value instance
    ASSERT_EQ((std::string)original["value"], (std::string)copy["value"]);
}

TEST(JsonNewTest, MutationReflectsInCopy)
{
    JsonNew a;
    a["key"] = 123;

    JsonNew b = a; // same underlying object
    b["key"] = 456;

    ASSERT_EQ((int)a["key"], 456); // change in b reflects in a
    ASSERT_EQ((int)b["key"], 456);
}

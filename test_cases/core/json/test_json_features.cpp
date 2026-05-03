#include <gtest/gtest.h>
#include <json/json.h>
#include <json/json_object.h>

#include <time/measure_time.h>

TEST(JsonTestFeature, GetStringValue)
{
    Json a;
    a["name"] = "Nguyen Huu Tam";
    std::string info = a;

    ASSERT_EQ(info == "{\"name\":\"Nguyen Huu Tam\"}", true);
}

TEST(JsonTestFeature, Operator_Compare_EqualJsonNew)
{
    Json a;
    a["data"] = "same";
    Json b = a;

    ASSERT_EQ(a["data"] == b["data"], true);
}

TEST(JsonTestFeature, Operator_Compare_NotEqualJsonNew)
{
    Json a;
    a["data"] = "Tam";
    Json b;
    b["data"] = "Nguyen";

    ASSERT_EQ(a["data"] != b["data"], true);
}

TEST(JsonTestFeature, Operator_Compare_Equal)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json a;
    a["key1"] = 42;
    a["key2"] = "Tam";
    a["key3"] = std::string("Nguyen");

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ(a["key1"] == 42, true);
    ASSERT_EQ(a["key1"] == 43, false);
    ASSERT_EQ(a["key1"] == 41, false);
    ASSERT_EQ(a["key1"] == "Tam", false);

    ASSERT_EQ(a["key2"] == "Tam", true);
    ASSERT_EQ(a["key2"] == std::string_view("Tam"), true);
    ASSERT_EQ(a["key2"] == std::string("Tam"), true);
    ASSERT_EQ(a["key2"] == "Nguyen", false);
    ASSERT_EQ(a["key2"] == 42, false);
    ASSERT_EQ(a["key2"] == nullptr, false);

    ASSERT_EQ(a["key3"] == "Nguyen", true);
    ASSERT_EQ(a["key3"] == std::string_view("Nguyen"), true);
    ASSERT_EQ(a["key3"] == std::string("Nguyen"), true);
    ASSERT_EQ(a["key3"] == "Tam", false);
    ASSERT_EQ(a["key3"] == 42, false);
    ASSERT_EQ(a["key3"] == nullptr, false);
}

TEST(JsonTestFeature, Operator_Compare_NotEqual)
{
    Json a;
    a["num"] = 100;
    a["name"] = "Tam";

    ASSERT_EQ(a["num"] != 99, true);
    ASSERT_EQ(a["num"] != 100, false);
    ASSERT_EQ(a["name"] != "Tam", false);
    ASSERT_EQ(a["name"] != "Nguyen", true);
}

TEST(JsonTestFeature, Operator_Compare_LessThan)
{
    Json a;
    a["score"] = 75;

    ASSERT_EQ(a["score"] < 80, true);
    ASSERT_EQ(a["score"] < 75, false);
    ASSERT_EQ(a["score"] < 70, false);
}

TEST(JsonTestFeature, Operator_Compare_GreaterThan)
{
    Json a;
    a["score"] = 85;

    ASSERT_EQ(a["score"] > 80, true);
    ASSERT_EQ(a["score"] > 85, false);
    ASSERT_EQ(a["score"] > 90, false);
}

TEST(JsonTestFeature, Operator_Compare_LessThanOrEqual)
{
    Json a;
    a["score"] = 60;

    ASSERT_EQ(a["score"] <= 60, true);
    ASSERT_EQ(a["score"] <= 70, true);
    ASSERT_EQ(a["score"] <= 50, false);
}

TEST(JsonTestFeature, Operator_Compare_GreaterThanOrEqual)
{
    Json a;
    a["score"] = 90;

    ASSERT_EQ(a["score"] >= 90, true);
    ASSERT_EQ(a["score"] >= 80, true);
    ASSERT_EQ(a["score"] >= 100, false);
}

TEST(JsonTestFeature, Operator_AddAssign)
{
    Json a;
    a["count"] = 5;

    a["count"] += 3;
    ASSERT_EQ(a["count"] == 8, true);

    a["text"] = "Hello";
    a["text"] += std::string(" World");
    ASSERT_EQ(a["text"] == "Hello World", true);
}

TEST(JsonTestFeature, Operator_SubtractAssign)
{
    Json a;
    a["value"] = 20;

    a["value"] -= 5;
    ASSERT_EQ(a["value"] == 15, true);

    a["value"] -= 15;
    ASSERT_EQ(a["value"] == 0, true);

    Json b;
    b["value"] = 20.6;

    b["value"] -= 5.2;
    ASSERT_NEAR((double)b["value"], 15.4, 1e-9);

    b["value"] -= 15.9;
    ASSERT_NEAR((double)b["value"], -0.5, 1e-9);
}

TEST(JsonTestFeature, HasAndRemoveField_NestedStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json config = {
        {"name", "Nguyen Huu Tam"},
        {"settings", {
            {"display", {
                {"resolution", "1920x1080"},
                {"brightness", 75}
            }},
            {"sound", {
                {"volume", 50},
                {"muted", false}
            }},
            {"network", {
                {"wifi", {
                    {"enabled", true},
                    {"ssid", "MyHomeWiFi"}
                }},
                {"ethernet", {
                    {"enabled", false}
                }}
            }}
        }},
        {"metadata", {
            {"created", "2025-07-26"},
            {"version", "1.0"}
        }}
    };

    // -------------------------------
    // Assert before removal
    // -------------------------------
    ASSERT_TRUE(config.has_field("settings"));
    ASSERT_TRUE(config["settings"].has_field("display"));
    ASSERT_TRUE(config["settings"]["display"].has_field("resolution"));
    ASSERT_TRUE(config["settings"]["network"]["wifi"].has_field("ssid"));
    ASSERT_TRUE(config["metadata"].has_field("version"));
    ASSERT_FALSE(config.has_field("non_existing_key"));

    // -------------------------------
    // Remove some fields
    // -------------------------------
    config["settings"]["display"].remove_field("brightness");
    config["settings"]["network"].remove_field("ethernet");
    config["metadata"].remove_field("version");

    // -------------------------------
    // Assert after removal
    // -------------------------------
    ASSERT_TRUE(config["settings"]["display"].has_field("resolution"));
    ASSERT_FALSE(config["settings"]["display"].has_field("brightness"));

    ASSERT_TRUE(config["settings"]["network"].has_field("wifi"));
    ASSERT_FALSE(config["settings"]["network"].has_field("ethernet"));

    ASSERT_TRUE(config["metadata"].has_field("created"));
    ASSERT_FALSE(config["metadata"].has_field("version"));
}

TEST(JsonTestFeature, SetIsStringFormat_WorksCorrectly)
{
    // Arrange
    Json a = {
        {"key", "Tam"}
    };

    // Act
    a["key"].set_is_string_format(true);

    // Assert
    std::string result = a.get_string_value();
    ASSERT_EQ(result, "{\"key\":\"Tam\"}");

    // Act: Set to false
    a["key"].set_is_string_format(false);
    std::string result2 = a.get_string_value();
    ASSERT_EQ(result2, "{\"key\":Tam}");  // still string, but not formatted as string
}

TEST(JsonTestFeature, SetSize_MakesArrayWithGivenSize)
{
    // Arrange
    Json json;
    // Act
    json.set_size(3);
    // Assert
    ASSERT_EQ(json.size(), 3);

    // Act
    json.set_size(2);
    // Assert
    ASSERT_EQ(json.size(), 2);

    // Act
    json.set_size(4);
    // Assert
    ASSERT_EQ(json.size(), 4);
}

TEST(JsonTestFeature, SetCapacity_MakesArrayWithGivenCapacity)
{
    // Arrange
    Json json;
    // Act
    json.set_capacity(3);
    // Assert
    ASSERT_EQ(json.capacity(), 3);

    // Act
    json.set_capacity(7);
    // Assert
    ASSERT_EQ(json.capacity(), 7);

    // Act
    json.set_size(4);
    // Assert
    ASSERT_EQ(json.capacity(), 7);
}

TEST(JsonTestFeature, Size_ReturnsCorrectObjectOrArraySize)
{
    // Object case
    Json obj = {
        {"name", "Tam"},
        {"age", 30}
    };
    ASSERT_EQ(obj.size(), 2);

    // Array case
    Json arr;
    arr.set_size(4);
    ASSERT_EQ(arr.size(), 4);
}

TEST(JsonTestFeature, Reverse_ReversesArrayCorrectly)
{
    // Arrange
    Json arr;
    arr.set_size(3);
    arr[0] = "first";
    arr[1] = "second";
    arr[2] = "third";

    // Act
    arr.reverse();

    // Assert
    ASSERT_EQ((std::string)arr[0], "third");
    ASSERT_EQ((std::string)arr[1], "second");
    ASSERT_EQ((std::string)arr[2], "first");
}

TEST(JsonTestFeature, NullFieldTransitionDeepStructure_NullptrOnly)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json config = {
        {"user", {
            {"name", nullptr},
            {"email", "huutam1991@gmail.com"},
            {"profile", {
                {"age", 30},
                {"bio", nullptr}
            }}
        }},
        {"system", {
            {"version", "1.0"},
            {"description", "default config"}
        }}
    };

    // -------------------------------
    // Initial null checks
    // -------------------------------
    ASSERT_TRUE(config["user"]["name"] == nullptr);
    ASSERT_TRUE(config["user"]["profile"]["bio"] == nullptr);
    ASSERT_FALSE(config["user"]["email"] == nullptr);
    ASSERT_FALSE(config["system"]["version"] == nullptr);

    // -------------------------------
    // Modify values
    // -------------------------------
    config["user"]["name"] = "Nguyen Huu Tam";  // from null to string
    config["user"]["profile"]["bio"] = "Low-latency developer";
    config["user"]["email"] = nullptr;          // from string to null
    config["system"]["version"] = nullptr;

    // -------------------------------
    // Post-modification checks
    // -------------------------------
    ASSERT_FALSE(config["user"]["name"] == nullptr);
    ASSERT_FALSE(config["user"]["profile"]["bio"] == nullptr);
    ASSERT_TRUE(config["user"]["email"] == nullptr);
    ASSERT_TRUE(config["system"]["version"] == nullptr);

    // -------------------------------
    // Extra value assertions
    // -------------------------------
    ASSERT_EQ((std::string)config["user"]["name"], "Nguyen Huu Tam");
    ASSERT_EQ((std::string)config["user"]["profile"]["bio"], "Low-latency developer");
}

TEST(JsonTestFeature, Sort_SortsArrayBasedOnCustomComparator)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json array;
    Json item1; item1["value"] = 30;
    Json item2; item2["value"] = 10;
    Json item3; item3["value"] = 20;

    array.push_back(item1);
    array.push_back(item2);
    array.push_back(item3);

    // -------------------------------
    // Action
    // -------------------------------
    array.sort([](Json& a, Json& b) {
        return (int)a["value"] < (int)b["value"];
    });

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((int)array[0]["value"], 10);
    ASSERT_EQ((int)array[1]["value"], 20);
    ASSERT_EQ((int)array[2]["value"], 30);
}

TEST(JsonTestFeature, PushBack_AppendsToArrayCorrectly)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json array;
    array.push_back("first");
    array.push_back("second");

    Json nested;
    nested["key"] = "value";
    array.push_back(nested);

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)array[0], "first");
    ASSERT_EQ((std::string)array[1], "second");
    ASSERT_EQ((std::string)array[2]["key"], "value");
    ASSERT_EQ(array.size(), 3);
}

TEST(JsonTestFeature, DeepClone_CreatesFullyIndependentCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json original;
    original["name"] = "Nguyen Huu Tam";
    original["age"] = 30;

    // Nested object
    original["skills"]["C++"] = "expert";
    original["skills"]["Rust"] = "intermediate";

    // Array-like structure
    original.set_size(2);
    original[0] = "first";
    original[1] = "second";

    // Deep nested object
    original["project"]["info"]["name"] = "TradingEngine";
    original["project"]["info"]["latency_ns"] = 50;

    // -------------------------------
    // Action
    // -------------------------------
    Json clone = original.deep_clone();

    // -------------------------------
    // Assert (Before modification: values must match)
    // -------------------------------
    ASSERT_EQ((std::string)clone["name"], (std::string)original["name"]);
    ASSERT_EQ((int)clone["age"], (int)original["age"]);
    ASSERT_EQ((std::string)clone["skills"]["C++"], (std::string)original["skills"]["C++"]);
    ASSERT_EQ((std::string)clone["skills"]["Rust"], (std::string)original["skills"]["Rust"]);
    ASSERT_EQ((std::string)clone[0], (std::string)original[0]);
    ASSERT_EQ((std::string)clone[1], (std::string)original[1]);
    ASSERT_EQ((std::string)clone["project"]["info"]["name"], (std::string)original["project"]["info"]["name"]);
    ASSERT_EQ((int)clone["project"]["info"]["latency_ns"], (int)original["project"]["info"]["latency_ns"]);

    // -------------------------------
    // Modify clone
    // -------------------------------
    clone["name"] = "Clone Tam";
    clone["skills"]["Rust"] = "beginner";
    clone[0] = "changed in clone";
    clone["project"]["info"]["latency_ns"] = 999;

    // -------------------------------
    // Modify original
    // -------------------------------
    original["name"] = "Original Tam";
    original["skills"]["C++"] = "master";
    original[1] = "changed in original";
    original["project"]["info"]["name"] = "NewEngine";

    // -------------------------------
    // Assert (clone not affected by original changes)
    // -------------------------------
    ASSERT_EQ((std::string)clone["name"], "Clone Tam");
    ASSERT_EQ((std::string)clone["skills"]["C++"], "expert");
    ASSERT_EQ((std::string)clone[1], "second");
    ASSERT_EQ((std::string)clone["project"]["info"]["name"], "TradingEngine");

    // -------------------------------
    // Assert (original not affected by clone changes)
    // -------------------------------
    ASSERT_EQ((std::string)original["name"], "Original Tam");
    ASSERT_EQ((std::string)original["skills"]["Rust"], "intermediate");
    ASSERT_EQ((std::string)original[0], "first");
    ASSERT_EQ((int)original["project"]["info"]["latency_ns"], 50);

    // -------------------------------
    // Final Assert: Same key but different value
    // -------------------------------
    ASSERT_NE((std::string)clone["name"], (std::string)original["name"]);
    ASSERT_NE((std::string)clone["skills"]["Rust"], (std::string)original["skills"]["Rust"]);
    ASSERT_NE((std::string)clone[0], (std::string)original[0]);
    ASSERT_NE((int)clone["project"]["info"]["latency_ns"], (int)original["project"]["info"]["latency_ns"]);
    ASSERT_NE((std::string)clone["project"]["info"]["name"], (std::string)original["project"]["info"]["name"]);
}

TEST(JsonTestFeature, DeepClone_ArrayStructure_IndependentCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json original;
    original.set_size(3);

    original[0]["name"] = "OrderBook";
    original[0]["status"] = "active";

    original[1]["name"] = "Vault";
    original[1]["status"] = "completed";

    original[2]["name"] = "LatencyTracker";
    original[2]["metrics"]["latency_ns"] = 42;
    original[2]["metrics"]["throughput"] = 15000;

    // -------------------------------
    // Action
    // -------------------------------
    Json clone = original.deep_clone();

    // -------------------------------
    // Assert (Before modification: same values)
    // -------------------------------
    ASSERT_EQ((std::string)clone[0]["name"], (std::string)original[0]["name"]);
    ASSERT_EQ((std::string)clone[1]["status"], (std::string)original[1]["status"]);
    ASSERT_EQ((int)clone[2]["metrics"]["latency_ns"], (int)original[2]["metrics"]["latency_ns"]);
    ASSERT_EQ((int)clone[2]["metrics"]["throughput"], (int)original[2]["metrics"]["throughput"]);

    // -------------------------------
    // Modify clone
    // -------------------------------
    clone[0]["name"] = "ClonedOrderBook";
    clone[1]["status"] = "cloned_status";
    clone[2]["metrics"]["latency_ns"] = 999;

    // -------------------------------
    // Modify original
    // -------------------------------
    original[1]["name"] = "OriginalVault";
    original[2]["metrics"]["throughput"] = 8888;

    // -------------------------------
    // Assert (clone not affected by original)
    // -------------------------------
    ASSERT_EQ((std::string)clone[0]["name"], "ClonedOrderBook");
    ASSERT_EQ((std::string)clone[1]["status"], "cloned_status");
    ASSERT_EQ((int)clone[2]["metrics"]["latency_ns"], 999);
    ASSERT_EQ((int)clone[2]["metrics"]["throughput"], 15000);

    // -------------------------------
    // Assert (original not affected by clone)
    // -------------------------------
    ASSERT_EQ((std::string)original[0]["name"], "OrderBook");
    ASSERT_EQ((std::string)original[1]["status"], "completed");
    ASSERT_EQ((int)original[2]["metrics"]["latency_ns"], 42);
    ASSERT_EQ((int)original[2]["metrics"]["throughput"], 8888);

    // -------------------------------
    // Final: ensure true independence
    // -------------------------------
    ASSERT_NE((std::string)clone[0]["name"], (std::string)original[0]["name"]);
    ASSERT_NE((int)clone[2]["metrics"]["latency_ns"], (int)original[2]["metrics"]["latency_ns"]);
}

TEST(JsonTestFeature, RangeBasedForLoop_Modify)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json json = {
        {"name", "Tam"},
        {"age", 33},
        {"lang", "C++"},
        {"skills", {
            {"C++", "expert"},
            {"Python", "intermediate"},
            {"Rust", "beginner"}
        }}
    };

    // Modify values inside loop
    for (auto& [key, value] : json)
    {
        if (value == "Tam")
        {
            value = "Nguyen Huu Tam";
        }
        else if (value == "C++")
        {
            value = "Rust";
        }
        else if (key == "age")
        {
            value += 1; // age becomes 34
        }
        else if (key == "skills")
        {
            value["C++"] = "advanced";
            value["Python"] = "expert";
        }
    }

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)json["name"], "Nguyen Huu Tam");
    ASSERT_EQ((std::string)json["lang"], "Rust");
    ASSERT_EQ((int)json["age"], 34);
    ASSERT_EQ((std::string)json["skills"]["C++"], "advanced");
    ASSERT_EQ((std::string)json["skills"]["Python"], "expert");
    ASSERT_EQ((std::string)json["skills"]["Rust"], "beginner");
}

TEST(JsonTestFeature, RangeBasedForLoop_Const)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json json;
    json["name"] = "Nguyen Huu Tam";
    json["age"] = 33;
    json["language"] = "C++";

    const Json& const_json = json;

    // -------------------------------
    // Act
    // -------------------------------
    for (const auto& [key, value] : const_json)
    {
        if (key == "name")
        {
            ASSERT_EQ(value == "Nguyen Huu Tam", true);
        }
        else if (key == "age")
        {
            ASSERT_EQ(value == 33, true);
        }
        else if (key == "language")
        {
            ASSERT_EQ(value == "C++", true);
        }
    }
}
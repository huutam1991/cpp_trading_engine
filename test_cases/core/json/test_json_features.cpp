#include <gtest/gtest.h>
#include <c_json/json.h>
#include <c_json/json_object.h>

#include <time/measure_time.h>

TEST(JsonTestFeature, HasAndRemoveField_NestedStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    JsonNew config = {
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
    JsonNew a = {
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
    JsonNew json;
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

TEST(JsonTestFeature, Size_ReturnsCorrectObjectOrArraySize)
{
    // Object case
    JsonNew obj = {
        {"name", "Tam"},
        {"age", 30}
    };
    ASSERT_EQ(obj.size(), 2);

    // Array case
    JsonNew arr;
    arr.set_size(4);
    ASSERT_EQ(arr.size(), 4);
}

TEST(JsonTestFeature, Reverse_ReversesArrayCorrectly)
{
    // Arrange
    JsonNew arr;
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
    JsonNew config = {
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
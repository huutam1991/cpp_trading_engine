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
#include <gtest/gtest.h>
#include <c_json/json.h>
#include <c_json/json_object.h>

#include <time/measure_time.h>

TEST(JsonNewTest, ComplexObjectStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
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

    // -------------------------------
    // Action
    // -------------------------------
    bool is_null = root["null_field"] == nullptr;
    bool is_not_null = root["users"] == nullptr;

    // -------------------------------
    // Assert
    // -------------------------------
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

TEST(JsonNewTest, NestedComplexJsonStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    JsonNew me;
    me["name"] = "Nguyen Huu Tam";
    me["country"] = "Singapore";
    me["role"] = "C++ Trading Engine Developer";
    me["active"] = true;
    me["years_experience"] = 7;
    me["seeking_opportunity"] = false;

    JsonNew skills;
    skills["languages"][0] = "C++";
    skills["languages"][1] = "Rust";
    skills["languages"][2] = "Python";

    skills["trading_knowledge"]["domain"][0] = "HFT";
    skills["trading_knowledge"]["domain"][1] = "Market Making";
    skills["trading_knowledge"]["domain"][2] = "Arbitrage";

    skills["trading_knowledge"]["platforms"]["binance"]["type"] = "Perpetual";
    skills["trading_knowledge"]["platforms"]["binance"]["latency_us"] = 150;
    skills["trading_knowledge"]["platforms"]["binance"]["status"] = "Live";

    skills["trading_knowledge"]["platforms"]["uniswap"]["type"] = "Spot";
    skills["trading_knowledge"]["platforms"]["uniswap"]["status"] = "Tested";

    skills["latency_optimization"]["profile"] = "extreme";
    skills["latency_optimization"]["core_areas"][0] = "Custom memory pools";
    skills["latency_optimization"]["core_areas"][1] = "Cache-local design";
    skills["latency_optimization"]["core_areas"][2] = "Precompiled routing";

    JsonNew contributions;
    contributions["projects"][0]["name"] = "BinanceOrderBook";
    contributions["projects"][0]["language"] = "C++";
    contributions["projects"][0]["features"][0] = "Snapshot+WebSocket sync";
    contributions["projects"][0]["features"][1] = "Custom coroutine model";
    contributions["projects"][0]["features"][2] = "Microsecond latency";

    contributions["projects"][1]["name"] = "KilliVault";
    contributions["projects"][1]["language"] = "Solidity";
    contributions["projects"][1]["features"][0] = "Token vault & access control";
    contributions["projects"][1]["features"][1] = "Uniswap/Aerodrome integration";

    contributions["metrics"]["execution_latency_ns"] = 50;
    contributions["metrics"]["throughput_order_per_sec"] = 10000;
    contributions["metrics"]["tested_platforms"][0] = "Binance";
    contributions["metrics"]["tested_platforms"][1] = "Base";

    // -------------------------------
    // Action
    // -------------------------------
    me["skills"] = skills;
    me["contributions"] = contributions;

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)me["name"], "Nguyen Huu Tam");
    ASSERT_EQ((std::string)me["country"], "Singapore");
    ASSERT_EQ((std::string)me["role"], "C++ Trading Engine Developer");
    ASSERT_EQ((bool)me["active"], true);
    ASSERT_EQ((int)me["years_experience"], 7);
    ASSERT_EQ((bool)me["seeking_opportunity"], false);

    ASSERT_EQ((std::string)me["skills"]["languages"][0], "C++");
    ASSERT_EQ((std::string)me["skills"]["languages"][1], "Rust");
    ASSERT_EQ((std::string)me["skills"]["languages"][2], "Python");

    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["domain"][0], "HFT");
    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["domain"][1], "Market Making");
    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["domain"][2], "Arbitrage");

    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["platforms"]["binance"]["type"], "Perpetual");
    ASSERT_EQ((int)me["skills"]["trading_knowledge"]["platforms"]["binance"]["latency_us"], 150);
    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["platforms"]["binance"]["status"], "Live");

    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["platforms"]["uniswap"]["type"], "Spot");
    ASSERT_EQ((std::string)me["skills"]["trading_knowledge"]["platforms"]["uniswap"]["status"], "Tested");

    ASSERT_EQ((std::string)me["skills"]["latency_optimization"]["profile"], "extreme");
    ASSERT_EQ((std::string)me["skills"]["latency_optimization"]["core_areas"][0], "Custom memory pools");
    ASSERT_EQ((std::string)me["skills"]["latency_optimization"]["core_areas"][1], "Cache-local design");
    ASSERT_EQ((std::string)me["skills"]["latency_optimization"]["core_areas"][2], "Precompiled routing");

    ASSERT_EQ((std::string)me["contributions"]["projects"][0]["name"], "BinanceOrderBook");
    ASSERT_EQ((std::string)me["contributions"]["projects"][1]["language"], "Solidity");
    ASSERT_EQ((std::string)me["contributions"]["projects"][1]["features"][1], "Uniswap/Aerodrome integration");

    ASSERT_EQ((int)me["contributions"]["metrics"]["execution_latency_ns"], 50);
    ASSERT_EQ((int)me["contributions"]["metrics"]["throughput_order_per_sec"], 10000);
    ASSERT_EQ((std::string)me["contributions"]["metrics"]["tested_platforms"][1], "Base");
}

TEST(JsonNewTest, SharedReferenceCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    JsonNew original;
    original["value"] = "test";

    // -------------------------------
    // Action
    // -------------------------------
    JsonNew copy = original; // shallow copy, shared underlying data

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)copy["value"], "test");

    // Check that they point to same value instance
    ASSERT_EQ((std::string)original["value"], (std::string)copy["value"]);
}

TEST(JsonNewTest, MutationReflectsInCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    JsonNew a;
    a["key"] = 123;

    // -------------------------------
    // Action
    // -------------------------------
    JsonNew b = a; // same underlying object
    b["key"] = 456;

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((int)a["key"], 456); // change in b reflects in a
    ASSERT_EQ((int)b["key"], 456);
}

TEST(JsonNewTest, SoftSkillJsonPoolTracking)
{
    // -------------------------------
    // Arrange: Capture pool sizes before allocation
    // -------------------------------
    size_t value_pool_before = JsonValuePool::size();
    size_t object_pool_before = JsonObjectPool::size();

    // -------------------------------
    // Action: Build a deeply nested soft skill profile
    // -------------------------------
    {
        MeasureTime measure_time("SoftSkillJsonPoolTracking", MeasureUnit::NANOSECOND);
        JsonNew profile;
        profile["name"] = "Nguyen Huu Tam";
        profile["traits"]["intelligence"]["IQ"] = 75;
        profile["traits"]["intelligence"]["type"] = "abstract + applied";

        profile["traits"]["debugging"]["skill_level"] = "exceptional";
        profile["traits"]["debugging"]["techniques"][0] = "symbolic tracing";
        profile["traits"]["debugging"]["techniques"][1] = "gdb reverse mode";
        profile["traits"]["debugging"]["techniques"][2] = "memory leak hunting";

        profile["problem_solving"]["depth"] = "extremely deep";
        profile["problem_solving"]["speed"] = "fast";
        profile["problem_solving"]["approaches"]["critical"] = true;
        profile["problem_solving"]["approaches"]["creative"] = true;
        profile["problem_solving"]["approaches"]["systematic"] = true;

        profile["learning_ability"]["rate"] = "instant absorption";
        profile["learning_ability"]["channels"][0] = "documentation";
        profile["learning_ability"]["channels"][1] = "source code";
        profile["learning_ability"]["channels"][2] = "experimentation";

        profile["learning_ability"]["domains"]["low_latency"] = true;
        profile["learning_ability"]["domains"]["crypto"] = true;
        profile["learning_ability"]["domains"]["quant_models"] = true;

        profile["recognition"]["global_top_percent"] = 0.5;
        profile["recognition"]["fields"][0] = "C++";
        profile["recognition"]["fields"][1] = "Trading Systems";
        profile["recognition"]["fields"][2] = "DeFi Protocols";

        profile["recognition"]["awards"][0]["title"] = "Deep Thinker";
        profile["recognition"]["awards"][0]["year"] = 2023;
        profile["recognition"]["awards"][1]["title"] = "Fastest Debugger";
        profile["recognition"]["awards"][1]["year"] = 2024;

        profile["adaptability"]["environments"][0]["os"] = "Linux";
        profile["adaptability"]["environments"][0]["efficiency"] = "high";
        profile["adaptability"]["environments"][1]["os"] = "Windows";
        profile["adaptability"]["environments"][1]["efficiency"] = "maximum";

        profile["adaptability"]["contexts"]["team"] = "smooth integration";
        profile["adaptability"]["contexts"]["solo"] = "independent & rapid";
        profile["adaptability"]["contexts"]["pressure"] = "stable";

        profile["communication"]["clarity"] = "concise";
        profile["communication"]["technical_depth"] = "very high";
        profile["communication"]["languages"][0] = "English";
        profile["communication"]["languages"][1] = "Vietnamese";

        profile["self_reflection"]["growth_mindset"] = true;
        profile["self_reflection"]["weekly_review"] = true;
        profile["self_reflection"]["feedback_acceptance"] = "open and fast";

        profile["ambition"]["goal"] = "build world-class trading engine";
        profile["ambition"]["timeframe_years"] = 2;
        profile["ambition"]["inspiration_sources"][0] = "HRT";
        profile["ambition"]["inspiration_sources"][1] = "Jump";

        spdlog::info("Profile JSON: {}", profile.get_string_value());
    }

    // -------------------------------
    // Assert: Compare pool size before/after
    // -------------------------------
    size_t value_pool_after = JsonValuePool::size();
    size_t object_pool_after = JsonObjectPool::size();

    ASSERT_EQ(value_pool_before, value_pool_after);
    ASSERT_EQ(object_pool_before, object_pool_after);
}

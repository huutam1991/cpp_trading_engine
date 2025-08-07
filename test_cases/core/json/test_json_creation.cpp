#include <gtest/gtest.h>
#include <json/json.h>
#include <json/json_object.h>

#include <time/measure_time.h>

TEST(JsonTestCreation, ComplexObjectStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json root;
    root["name"] = "ChatGPT";
    root["active"] = true;
    root["version"] = 4.0;
    root["users"] = 1000000;
    root["null_field"] = nullptr;

    Json arr;
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

    ASSERT_EQ((std::string)root["metadata"][0], "openai");
    ASSERT_EQ((int)root["metadata"][1], 2023);
    ASSERT_EQ((bool)root["metadata"][2], false);
}

TEST(JsonTestCreation, NestedComplexJsonStructure)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json me;
    me["name"] = "Nguyen Huu Tam";
    me["country"] = "Singapore";
    me["role"] = "C++ Trading Engine Developer";
    me["active"] = true;
    me["years_experience"] = 7;
    me["seeking_opportunity"] = false;

    Json skills;
    skills["languages"][0] = "C++";
    skills["languages"][1] = "Rust";
    skills["languages"][2] = "Python";

    skills["trading_knowledge"]["domain"][0] = "HFT";
    std::string key = "trading_knowledge";
    skills[key]["domain"][1] = "Market Making";
    skills[key]["domain"][2] = "Arbitrage";

    skills[key]["platforms"]["binance"]["type"] = "Perpetual";
    skills[key]["platforms"]["binance"]["latency_us"] = 150;
    skills[key]["platforms"]["binance"]["status"] = "Live";

    skills[key]["platforms"]["uniswap"]["type"] = "Spot";
    skills[key]["platforms"]["uniswap"]["status"] = "Tested";

    skills["latency_optimization"]["profile"] = "extreme";
    skills["latency_optimization"]["core_areas"][0] = "Custom memory pools";
    skills["latency_optimization"]["core_areas"][1] = "Cache-local design";
    skills["latency_optimization"]["core_areas"][2] = "Precompiled routing";

    Json contributions;
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

TEST(JsonTestCreation, SharedReferenceCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json original;
    original["value"] = "test";

    // -------------------------------
    // Action
    // -------------------------------
    Json copy = original; // shallow copy, shared underlying data

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)copy["value"], "test");

    // Check that they point to same value instance
    ASSERT_EQ((std::string)original["value"], (std::string)copy["value"]);
}

TEST(JsonTestCreation, MutationReflectsInCopy)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json a;
    a["key"] = 123;

    // -------------------------------
    // Action
    // -------------------------------
    Json b = a; // same underlying object
    b["key"] = 456;

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((int)a["key"], 456); // change in b reflects in a
    ASSERT_EQ((int)b["key"], 456);
}

TEST(JsonTestCreation, CreateByPairInitializerList_Extended_NoArray)
{
    // -------------------------------
    // Arrange
    // -------------------------------
    Json original = {
        {"name", "Nguyen Huu Tam"},
        {"age", 30},
        {"is_developer", true},
        {"value", "test"},
        {"skills", {
            {"C++", "expert"},
            {"Python", "intermediate"},
            {"Rust", "beginner"},
            {"Solidity", "advanced"}
        }},
        {"projects", {
            {"trading_engine", {
                {"language", "C++"},
                {"status", "in_progress"},
                {"performance", {
                    {"latency_ns", 50},
                    {"throughput_ops", 10000}
                }}
            }},
            {"vault", {
                {"language", "Solidity"},
                {"status", "completed"},
                {"audited", true}
            }}
        }},
        {"education", {
            {"university", "NUS"},
            {"degree", "Computer Science"},
            {"graduated", true}
        }},
        {"preferences", {
            {"os", {
                {"primary", "Linux"},
                {"secondary", "Windows"}
            }},
            {"language_1", "English"},
            {"language_2", "Vietnamese"}
        }}
    };

    // -------------------------------
    // Action
    // -------------------------------
    Json copy = original; // shallow copy

    // -------------------------------
    // Assert
    // -------------------------------
    ASSERT_EQ((std::string)copy["value"], "test");
    ASSERT_EQ((int)copy["age"], 30);
    ASSERT_EQ((std::string)copy["skills"]["C++"], "expert");
    ASSERT_EQ((std::string)copy["skills"]["Solidity"], "advanced");
    ASSERT_EQ((std::string)copy["projects"]["trading_engine"]["language"], "C++");
    ASSERT_EQ((std::string)copy["projects"]["trading_engine"]["status"], "in_progress");
    ASSERT_EQ((int)copy["projects"]["trading_engine"]["performance"]["latency_ns"], 50);
    ASSERT_EQ((int)copy["projects"]["trading_engine"]["performance"]["throughput_ops"], 10000);
    ASSERT_EQ((std::string)copy["projects"]["vault"]["language"], "Solidity");
    ASSERT_EQ((std::string)copy["projects"]["vault"]["status"], "completed");
    ASSERT_EQ((bool)copy["projects"]["vault"]["audited"], true);
    ASSERT_EQ((std::string)copy["education"]["university"], "NUS");
    ASSERT_EQ((std::string)copy["education"]["degree"], "Computer Science");
    ASSERT_EQ((bool)copy["education"]["graduated"], true);
    ASSERT_EQ((std::string)copy["preferences"]["os"]["primary"], "Linux");
    ASSERT_EQ((std::string)copy["preferences"]["os"]["secondary"], "Windows");
    ASSERT_EQ((std::string)copy["preferences"]["language_1"], "English");
    ASSERT_EQ((std::string)copy["preferences"]["language_2"], "Vietnamese");

    // Check shared reference
    ASSERT_EQ((std::string)original["value"], (std::string)copy["value"]);
    ASSERT_EQ((int)original["projects"]["trading_engine"]["performance"]["latency_ns"],
              (int)copy["projects"]["trading_engine"]["performance"]["latency_ns"]);
}

TEST(JsonTestCreation, SoftSkillJsonPoolTracking)
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
        MeasureTime measure_time("SoftSkillJsonPoolTracking", MeasureUnit::MICROSECOND);

        for (size_t i = 0; i < 1; ++i)
        {
            Json profile;
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

            std::string json_string = profile.get_string_value();
            // spdlog::info("Profile JSON: {}", json_string);
        }

    }

    // -------------------------------
    // Assert: Compare pool size before/after
    // -------------------------------
    size_t value_pool_after = JsonValuePool::size();
    size_t object_pool_after = JsonObjectPool::size();

    ASSERT_EQ(value_pool_before, value_pool_after);
    ASSERT_EQ(object_pool_before, object_pool_after);
}

TEST(JsonTestCreation, JsonParse)
{
    // -------------------------------
    // Arrange: Create a complex JSON string
    // -------------------------------
    std::string data = "{\"ambition\":{\"inspiration_sources\":[\"HRT\",\"Jump\"],\"timeframe_years\":2,\"goal\":\"build world-class trading engine\"},\"communication\":{\"languages\":[\"English\",\"Vietnamese\"],\"technical_depth\":\"very high\",\"clarity\":\"concise\"},\"self_reflection\":{\"weekly_review\":true,\"feedback_acceptance\":\"open and fast\",\"growth_mindset\":true},\"adaptability\":{\"contexts\":{\"pressure\":\"stable\",\"solo\":\"independent & rapid\",\"team\":\"smooth integration\"},\"environments\":[{\"efficiency\":\"high\",\"os\":\"Linux\"},{\"efficiency\":\"maximum\",\"os\":\"Windows\"}]},\"recognition\":{\"awards\":[{\"year\":2023,\"title\":\"Deep Thinker\"},{\"year\":2024,\"title\":\"Fastest Debugger\"}],\"fields\":[\"C++\",\"Trading Systems\",\"DeFi Protocols\"],\"global_top_percent\":0.5},\"learning_ability\":{\"domains\":{\"quant_models\":true,\"crypto\":true,\"low_latency\":true},\"channels\":[\"documentation\",\"source code\",\"experimentation\"],\"rate\":\"instant absorption\"},\"problem_solving\":{\"speed\":\"fast\",\"approaches\":{\"systematic\":true,\"creative\":true,\"critical\":true},\"depth\":\"extremely deep\"},\"traits\":{\"debugging\":{\"techniques\":[\"symbolic tracing\",\"gdb reverse mode\",\"memory leak hunting\"],\"skill_level\":\"exceptional\"},\"intelligence\":{\"type\":\"abstract + applied\",\"IQ\":75}},\"name\":\"Nguyen Huu Tam\"}";

    // -------------------------------
    // Action: Parse the JSON string using Json
    // -------------------------------
    Json json = Json::parse(data);

    // -------------------------------
    // Assert: Verify the parsed values
    // -------------------------------
    ASSERT_EQ((std::string)json["name"], "Nguyen Huu Tam");
    ASSERT_EQ((std::string)json["ambition"]["goal"], "build world-class trading engine");
    ASSERT_EQ((double)json["recognition"]["global_top_percent"], 0.5);
    ASSERT_EQ((std::string)json["communication"]["clarity"], "concise");
    ASSERT_EQ((bool)json["self_reflection"]["growth_mindset"], true);
    ASSERT_EQ((std::string)json["adaptability"]["contexts"]["team"], "smooth integration");
    ASSERT_EQ((std::string)json["learning_ability"]["rate"], "instant absorption");
    ASSERT_EQ((std::string)json["problem_solving"]["depth"], "extremely deep");
    ASSERT_EQ((std::string)json["traits"]["debugging"]["skill_level"], "exceptional");
    ASSERT_EQ((int)json["traits"]["intelligence"]["IQ"], 75);
    ASSERT_EQ((std::string)json["traits"]["intelligence"]["type"], "abstract + applied");
    ASSERT_EQ((std::string)json["traits"]["debugging"]["techniques"][0], "symbolic tracing");
    ASSERT_EQ((std::string)json["traits"]["debugging"]["techniques"][1], "gdb reverse mode");
    ASSERT_EQ((std::string)json["traits"]["debugging"]["techniques"][2], "memory leak hunting");
    ASSERT_EQ((bool)json["learning_ability"]["domains"]["quant_models"], true);
    ASSERT_EQ((bool)json["learning_ability"]["domains"]["crypto"], true);
    ASSERT_EQ((bool)json["learning_ability"]["domains"]["low_latency"], true);
    ASSERT_EQ((std::string)json["recognition"]["awards"][0]["title"], "Deep Thinker");
    ASSERT_EQ((int)json["recognition"]["awards"][0]["year"], 2023);
    ASSERT_EQ((std::string)json["recognition"]["awards"][1]["title"], "Fastest Debugger");
    ASSERT_EQ((int)json["recognition"]["awards"][1]["year"], 2024);
    ASSERT_EQ((std::string)json["adaptability"]["environments"][0]["os"], "Linux");
    ASSERT_EQ((std::string)json["adaptability"]["environments"][1]["os"], "Windows");
    ASSERT_EQ((std::string)json["adaptability"]["environments"][0]["efficiency"], "high");
    ASSERT_EQ((std::string)json["adaptability"]["environments"][1]["efficiency"], "maximum");
    ASSERT_EQ((std::string)json["communication"]["languages"][0], "English");
    ASSERT_EQ((std::string)json["communication"]["languages"][1], "Vietnamese");
    ASSERT_EQ((bool)json["self_reflection"]["weekly_review"], true);
    ASSERT_EQ((std::string)json["self_reflection"]["feedback_acceptance"], "open and fast");
}

TEST(JsonTestCreation, JsonParse_PoolCountStable)
{
    // -------------------------------
    // Arrange: Create JSON string
    // -------------------------------
    std::string data = R"({
        "system": {
            "name": "EngineX",
            "version": "1.2.3",
            "performance": {
                "latency_us": 15.7,
                "throughput": 1000000,
                "stability": true
            },
            "modules": ["parser", "matcher", "router", "logger"],
            "platform": {"os": "Linux", "arch": "x86_64"},
            "team": {"lead": "Tam", "members": ["Alice", "Bob", "Charlie"]}
        },
        "features": {
            "realtime": true,
            "backtest": true,
            "exchange_support": ["Binance", "Bybit", "OKX", "Deribit"],
            "languages": {"primary": "C++", "secondary": "Rust"}
        },
        "limits": {
            "max_orders": 100000,
            "max_symbols": 5000,
            "rate_limit_per_sec": 1500,
            "negative1": -234,
            "negative2": -816.48364
        },
        "meta": {
            "timestamp": 1724000000000,
            "description": "Stress test for parser and memory pool integrity"
        }
    })";

    // -------------------------------
    // Record pool state before parse
    // -------------------------------
    int obj_count_before = JsonObjectPool::size();
    int val_count_before = JsonValuePool::size();
    int str_count_before = StringPool::size();

    // -------------------------------
    // Parse
    // -------------------------------

    {
        Json json;
        json = Json::parse(data);

        // -------------------------------
        // Basic field verification
        // -------------------------------
        ASSERT_EQ((std::string)json["system"]["name"], "EngineX");
        ASSERT_EQ((std::string)json["system"]["team"]["lead"], "Tam");
        ASSERT_EQ((std::string)json["features"]["languages"]["primary"], "C++");
        ASSERT_EQ((int)json["limits"]["max_orders"], 100000);
        ASSERT_EQ((int)json["limits"]["negative1"], -234);
        ASSERT_EQ((double)json["limits"]["negative2"], -816.48364);
        ASSERT_EQ((bool)json["system"]["performance"]["stability"], true);
        ASSERT_EQ((std::string)json["features"]["exchange_support"][0], "Binance");
        ASSERT_EQ((std::string)json["features"]["exchange_support"][1], "Bybit");
        ASSERT_EQ((std::string)json["features"]["exchange_support"][2], "OKX");
        ASSERT_EQ((std::string)json["features"]["exchange_support"][3], "Deribit");
        ASSERT_EQ((int)json["system"]["performance"]["latency_us"], 15);
        ASSERT_EQ((int)json["system"]["performance"]["throughput"], 1000000);
        ASSERT_EQ((std::string)json["meta"]["description"], "Stress test for parser and memory pool integrity");
        ASSERT_EQ((int)json["limits"]["rate_limit_per_sec"], 1500);
        ASSERT_EQ((int)json["limits"]["max_symbols"], 5000);
        ASSERT_EQ((int)json["system"]["team"]["members"].size(), 3);
        ASSERT_EQ((std::string)json["system"]["platform"]["os"], "Linux");
        ASSERT_EQ((std::string)json["system"]["platform"]["arch"], "x86_64");
        ASSERT_EQ((int)json["system"]["modules"].size(), 4);
        ASSERT_EQ((std::string)json["system"]["modules"][0], "parser");
        ASSERT_EQ((std::string)json["system"]["modules"][1], "matcher");
        ASSERT_EQ((std::string)json["system"]["modules"][2], "router");
        ASSERT_EQ((std::string)json["system"]["modules"][3], "logger");
        ASSERT_EQ((uint64_t)json["meta"]["timestamp"], 1724000000000);
    }

    // -------------------------------
    // Assert: pool usage remains consistent
    // -------------------------------
    int obj_count_after = JsonObjectPool::size();
    int val_count_after = JsonValuePool::size();
    int str_count_after = StringPool::size();

    ASSERT_EQ(obj_count_before, obj_count_after);
    ASSERT_EQ(val_count_before, val_count_after);
    ASSERT_EQ(str_count_before, str_count_after);

    // -------------------------------
    // Assert: Pool head/tail consistency
    // -------------------------------
    int head_pool_after = JsonObjectPool::head();
    int tail_pool_after = JsonObjectPool::tail();
    int head_value_after = JsonValuePool::head();
    int tail_value_after = JsonValuePool::tail();
    int head_string_after = StringPool::head();
    int tail_string_after = StringPool::tail();

    ASSERT_EQ(head_pool_after, tail_pool_after);
    ASSERT_EQ(head_value_after, tail_value_after);
    ASSERT_EQ(head_string_after, tail_string_after);
}
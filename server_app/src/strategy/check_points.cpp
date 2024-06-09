#include <strategy/check_points.h>

CheckPoints::CheckPoints(const std::string symbol, double volumn, double move_price, double sell_buy_ratio) :
    m_symbol(symbol), m_buy_volumn(volumn), m_move_price(move_price), m_sell_buy_ratio(sell_buy_ratio)
{
    m_collection_name = symbol + "_" + std::to_string((size_t)volumn) + "_" + std::to_string((size_t)move_price);
    m_checkpoint_list = DataModel::get_data_model_map(STRATEGY_DB_NAME, m_collection_name, "checkpoint_id");
}

std::string CheckPoints::get_collection_name()
{
    return m_collection_name;
}

std::string CheckPoints::get_checkpoint_id(double price)
{
    return m_symbol + "_" + std::to_string((size_t)price);
}

DataModel CheckPoints::get_checkpoint_by_price(double price)
{
    std::string checkpoint_id = get_checkpoint_id(price);

    if (m_checkpoint_list.find(checkpoint_id) != m_checkpoint_list.end())
    {
        return m_checkpoint_list[checkpoint_id];
    }

    return create_checkpoint_data_model(price);
}

DataModel CheckPoints::create_checkpoint_data_model(double price)
{
    std::string checkpoint_id = get_checkpoint_id(price);

    DataModel checkpoint(STRATEGY_DB_NAME, m_collection_name);
    checkpoint = {
        // Id
        {"checkpoint_id", checkpoint_id},

        // Info
        {"info", {
            {"symbol", m_symbol},
            {"price", price},
        }},

        // Size
        {"size", {
            {"buy_volumn", m_buy_volumn},
            {"move_price", m_move_price},
            {"sell_buy_ratio", m_sell_buy_ratio},
        }},

        // Current positions
        {"positions", {
            {"buy_spot", {
                {"quantity", 0.0},
                {"volumn_in_usdt", 0.0},
            }},
            {"sell_perpetual", {
                {"quantity", 0.0},
                {"volumn_in_usdt", 0.0},
            }},
        }},

        // For accounting
        {"accounting", {
            {"buy_spot_profit", 0.0},
            {"sell_perpetual_profit", 0.0},
            {"total_profit", 0.0},
            {"visit_times", 0.0},
        }},

        // Is active checkpoint
        {"is_current_checkpoint", false},
    };

    // Add to [m_checkpoint_list]
    m_checkpoint_list.insert(std::make_pair(checkpoint_id, checkpoint));

    return checkpoint;
}

DataModel CheckPoints::get_current_checkpoint()
{
    bool is_current_checkpoint = false;

    for (auto& it : m_checkpoint_list)
    {
        is_current_checkpoint = (bool)it.second["is_current_checkpoint"];

        if (is_current_checkpoint == true)
        {
            return it.second;
        }
    }

    return DataModel(JsonNull());
}

double CheckPoints::get_total_profit()
{
    double total_profit = 0;

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        total_profit += (double)checkpoint["accounting"]["total_profit"];
    }

    return total_profit;
}

Json CheckPoints::get_buy_spot_holding()
{
    double quantity = 0;
    double volumn_in_usdt = 0;

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        quantity += (double)checkpoint["positions"]["buy_spot"]["quantity"];
        volumn_in_usdt += (double)checkpoint["positions"]["buy_spot"]["volumn_in_usdt"];
    }

    return {
        {"quantity", quantity},
        {"volumn_in_usdt", volumn_in_usdt},
    };
}

std::string CheckPoints::get_min_checkpoint()
{
    double min_price = -1;
    std::string res = "";

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        double price = checkpoint["info"]["price"];
        std::string checkpoint_id = checkpoint["checkpoint_id"];

        if (res == "")
        {
            min_price = price;
            res = checkpoint_id;
        }
        else
        {
            if (price < min_price)
            {
                min_price = price;
                res = checkpoint_id;
            }
        }
    }

    return res;
}

std::string CheckPoints::get_max_checkpoint()
{
    double max_price = -1;
    std::string res = "";

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        double price = checkpoint["info"]["price"];
        std::string checkpoint_id = checkpoint["checkpoint_id"];

        if (res == "")
        {
            max_price = price;
            res = checkpoint_id;
        }
        else
        {
            if (price > max_price)
            {
                max_price = price;
                res = checkpoint_id;
            }
        }
    }

    return res;
}

Json CheckPoints::get_neighbor_checkpoints()
{
    Json current_checkpoint = get_current_checkpoint().get_data().deep_clone();
    std::string symbol = current_checkpoint["info"]["symbol"];
    double price = current_checkpoint["info"]["price"];
    double move_price = current_checkpoint["size"]["move_price"];

    return {
        {"next_checkpoint", symbol + "_" + std::to_string(long(price + move_price))},
        {"prev_checkpoint", symbol + "_" + std::to_string(long(price - move_price))},
    };
}

double CheckPoints::get_price_distance()
{
    std::string max_checkpoint = get_max_checkpoint();
    std::string min_checkpoint = get_min_checkpoint();

    double max_price = 0;
    double min_price = 0;

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        std::string checkpoint_id = checkpoint["checkpoint_id"];

        if (checkpoint_id == max_checkpoint)
        {
            max_price = checkpoint["info"]["price"];
        }
        else if (checkpoint_id == min_checkpoint)
        {
            min_price = checkpoint["info"]["price"];
        }
    }

    return max_price - min_price;
}

Json CheckPoints::get_current_info()
{
    Json current_checkpoint = get_current_checkpoint().get_data().deep_clone();
    current_checkpoint.remove_field("is_current_checkpoint");
    current_checkpoint.remove_field("_id");
    current_checkpoint.remove_field("info");
    current_checkpoint.remove_field("size");
    current_checkpoint.remove_field("accounting");

    return {
        {"current_checkpoint", current_checkpoint},
        {"buy_spot_holding", get_buy_spot_holding()},
        {"total_checkpoints", m_checkpoint_list.size()},
        {"min_checkpoint", get_min_checkpoint()},
        {"max_checkpoint", get_max_checkpoint()},
        {"neighbor_checkpoints", get_neighbor_checkpoints()},
        {"price_distance", get_price_distance()},
        {"total_profit", get_total_profit()}
    };
}
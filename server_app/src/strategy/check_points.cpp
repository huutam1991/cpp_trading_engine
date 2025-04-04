#include <strategy/check_points.h>

CheckPointList::CheckPointList(const std::string symbol, double volumn, double move_price, double sell_buy_ratio) :
    m_symbol(symbol), m_buy_volumn(volumn), m_move_price(move_price), m_sell_buy_ratio(sell_buy_ratio)
{
    m_collection_name = symbol + "_" + std::to_string((size_t)volumn) + "_" + std::to_string((size_t)move_price);
    m_checkpoint_list = DataModel::load_data_model_map<std::string>(STRATEGY_DB_NAME, m_collection_name, "checkpoint_id");
}

std::string CheckPointList::get_collection_name()
{
    return m_collection_name;
}

std::string CheckPointList::get_checkpoint_id(double price)
{
    return m_symbol + "_" + std::to_string((size_t)price);
}

DataModel CheckPointList::get_checkpoint_by_price(double price)
{
    std::string checkpoint_id = get_checkpoint_id(price);

    if (m_checkpoint_list.find(checkpoint_id) != m_checkpoint_list.end())
    {
        return m_checkpoint_list[checkpoint_id];
    }

    return create_checkpoint_data_model(price);
}

DataModel CheckPointList::create_checkpoint_data_model(double price)
{
    std::string checkpoint_id = get_checkpoint_id(price);

    DataModel checkpoint(STRATEGY_DB_NAME, m_collection_name);
    checkpoint = {
        // Id
        {"checkpoint_id", checkpoint_id},

        // Orders that related to this checkpoint
        {"open_order_id", 0},
        {"close_order_id", 0},

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

DataModel CheckPointList::get_current_checkpoint()
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

DataModel CheckPointList::get_one_holding_checkpoint()
{
    for (auto& it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        double quantity = checkpoint["positions"]["buy_spot"]["quantity"];

        // [quantity] > 0 means this is a holding checkpoint
        if (quantity > 0)
        {
            return it.second;
        }
    }

    return DataModel(JsonNull());
}

DataModel CheckPointList::get_checkpoint_can_take_profit(double price, double take_profit) {
    for (auto& it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        double quantity = checkpoint["positions"]["buy_spot"]["quantity"];
        double mark_price = checkpoint["info"]["price"];
        double move_price = price - mark_price;

        // [move_price] >= [take_profit] means this checkpoint can take profit
        if (move_price >= take_profit && quantity > 0)
        {
            return it.second;
        }
    }

    return DataModel(JsonNull());
}

double CheckPointList::get_total_profit()
{
    double total_profit = 0;

    for (auto it : m_checkpoint_list)
    {
        DataModel& checkpoint = it.second;
        total_profit += (double)checkpoint["accounting"]["total_profit"];
    }

    return total_profit;
}

Json CheckPointList::get_buy_spot_holding()
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

std::string CheckPointList::get_min_checkpoint()
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

std::string CheckPointList::get_max_checkpoint()
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

Json CheckPointList::get_neighbor_checkpoints()
{
    Json current_checkpoint = get_current_checkpoint().get_data().deep_clone();
    double price = current_checkpoint["info"]["price"];
    double move_price = current_checkpoint["size"]["move_price"];

    Json next_checkpoint = {
        {"next_price", price + move_price},
    };

    Json curr_checkpoint = {
        {"curr_price", price},
    };

    Json prev_checkpoint = {
        {"prev_price", price - move_price},
    };

    Json res = Json::create_array();
    res.push_back(next_checkpoint);
    res.push_back(curr_checkpoint);
    res.push_back(prev_checkpoint);

    return res;
}

double CheckPointList::get_price_distance()
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

Json CheckPointList::get_current_info()
{
    return {
        {"buy_spot_holding", get_buy_spot_holding()},
        {"total_checkpoints", m_checkpoint_list.size()},
        {"min_checkpoint", get_min_checkpoint()},
        {"max_checkpoint", get_max_checkpoint()},
        {"neighbor_checkpoints", get_neighbor_checkpoints()},
        {"max_min_price_distance", get_price_distance()},
        {"total_profit", get_total_profit()}
    };
}
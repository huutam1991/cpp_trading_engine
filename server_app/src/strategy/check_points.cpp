#include <strategy/check_points.h>

CheckPoints::CheckPoints(const std::string symbol, double volumn, double move_price) :
    m_symbol(symbol), m_volumn(volumn), m_move_price(move_price),
    m_collection_name(symbol + "_" + std::to_string((size_t)volumn) + "_" + std::to_string((size_t)move_price))
{
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
            {"volumn", m_volumn},
            {"move_price", m_move_price},
        }},

        // Current positions
        {"positions", {
            {"buy_spot", 0.0},
            {"sell_perpetual", 0.0},
        }},

        // For accounting
        {"accounting", {
            {"total_profit", 0.0},
            {"visit_times", 0.0},
        }},

        // Is active checkpoint
        {"is_current_checkpoint", false},
    };

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
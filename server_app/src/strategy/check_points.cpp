#include <strategy/check_points.h>

CheckPoints::CheckPoints(const std::string symbol, double volumn, double move_price) :
    m_collection_name(symbol + "_" + std::to_string((size_t)volumn) + "_" + std::to_string((size_t)move_price))
{
    m_checkpoint_list = DataModel::get_data_model_map(STRATEGY_DB_NAME, m_collection_name, "checkpoint_id");
}

std::string CheckPoints::get_collection_name()
{
    return m_collection_name;
}
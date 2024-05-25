#include <strategy/check_points.h>

CheckPoints::CheckPoints(const std::string symbol, double volumn, double move_price)
{
    std::string collection_name = symbol + "_" + std::to_string(volumn) + "_" + std::to_string(move_price);
    m_checkpoint_list = DataModel::get_data_model_map(STRATEGY_DB_NAME, collection_name, "checkpoint_id");
}
#ifndef CHECK_POINTS_H
#define CHECK_POINTS_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

class CheckPoints
{
    // Data fields

    // // Id
    // std::string m_symbol;
    // double m_price;
    // std::string m_checkpoint_id; // m_checkpoint_id = m_symbol + "_" + std::to_string(m_price)

    // // Size
    // double m_volumn;
    // double m_move_value;

    // // Current positions
    // double m_buy_spot;
    // double m_sell_perpetual;

    // // For accounting
    // double m_total_profit;
    // size_t m_visit_times;

    // // Is active checkpoint
    // bool m_is_current_checkpoint = false;

    std::unordered_map<std::string, DataModel> m_checkpoint_list;

public:
    CheckPoints(const std::string symbol, double volumn, double move_price);
};

#endif //CHECK_POINTS_H
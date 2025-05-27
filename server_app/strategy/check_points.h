#pragma once

#include <utils/util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

#include <order/order.h>

class CheckPointList
{
    // Data fields

    // Id
    std::string m_symbol;
    double m_current_price = 2800.0;

    // Size
    double m_buy_volumn;
    double m_move_price;
    double m_sell_buy_ratio;

    std::unordered_map<std::string, DataModel> m_checkpoint_list;
    std::string m_collection_name;

private:
    std::string get_collection_name();
    std::string get_checkpoint_id(double price);

    DataModel create_checkpoint_data_model(double price);

    // For getting information
    double      get_total_profit();
    Json        get_buy_spot_holding();
    Json        get_neighbor_checkpoints();
    std::string get_min_checkpoint();
    std::string get_max_checkpoint();
    double      get_price_distance();

public:
    CheckPointList(const std::string symbol, double volumn, double move_price, double sell_buy_ratio);

    std::string get_symbol() { return m_symbol; }
    DataModel   get_current_checkpoint();
    DataModel   get_one_holding_checkpoint();
    DataModel   get_checkpoint_by_price(double price);
    DataModel   get_checkpoint_can_take_profit(double price, double take_profit);
    Json        get_current_info();
};

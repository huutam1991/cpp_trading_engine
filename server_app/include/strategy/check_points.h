#ifndef CHECK_POINTS_H
#define CHECK_POINTS_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

#include <order/order.h>

class CheckPoints
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

public:
    CheckPoints(const std::string symbol, double volumn, double move_price, double sell_buy_ratio);

    DataModel get_current_checkpoint();
    DataModel get_checkpoint_by_price(double price);
    double    get_total_profit();
    Json      get_buy_spot_holding();
};

#endif //CHECK_POINTS_H
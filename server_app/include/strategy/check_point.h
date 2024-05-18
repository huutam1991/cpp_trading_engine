#ifndef CHECK_POINT_H
#define CHECK_POINT_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>

class CheckPoint
{
    std::string m_symbol;
    double m_volumn;
    double m_move_value;
    double m_price;

    // Current positions
    double m_buy_spot;
    double m_sell_perpetual;

    // For accounting
    double m_profit;
    size_t m_visit_time;

public:
    void save_to_db();

};

#endif //CHECK_POINT_H
#ifndef STRATEGY_H
#define STRATEGY_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

#include <strategy/check_points.h>

class Strategy
{
    Singleton(Strategy)

private:
    // Info
    std::string m_symbol;
    double m_volumn;
    double m_move_price;
    double m_sell_buy_ratio;

    // Status
    double m_current_price = -1.0;
    bool m_is_running = false;

    // Checkpoints
    std::shared_ptr<CheckPoints> m_checkpoints;

    void start();
    void stop();

public:
    void init();
    void on_config_change();
    void update(double price);

    double get_current_price();
};

#endif //STRATEGY_H
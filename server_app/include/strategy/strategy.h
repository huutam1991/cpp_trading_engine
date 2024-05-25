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

    // Status
    double m_current_price = -1.0;
    bool m_is_running = false;

    // Checkpoints
    std::shared_ptr<CheckPoints> m_checkpoints;

public:
    void init();
    void update(double price);

    double get_current_price();
};

#endif //STRATEGY_H
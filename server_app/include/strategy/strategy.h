#ifndef STRATEGY_H
#define STRATEGY_H

#include <util_macros.h>
#include <app_constants.h>
#include <json/json.h>
#include <data_model/data_model.h>

class Strategy
{
    Singleton(Strategy)

private:
    std::string m_symbol;
    double m_volumn;
    double m_move_price;

    bool m_is_running = false;

public:
    void init();
};

#endif //STRATEGY_H
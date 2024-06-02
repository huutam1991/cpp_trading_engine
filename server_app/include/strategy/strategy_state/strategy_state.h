#ifndef STRATEGY_STATE_H
#define STRATEGY_STATE_H

#include <gateways/gateway.h>
#include <strategy/check_points.h>

enum StateStatus
{
    START,
    PLACING,
    MONITORING,
    CLOSING,
    STOP,
};

class StrategyState
{
protected:
    Gateway* m_gateway;
    CheckPoints* m_checkpoints;


public:
    StrategyState(Gateway* gateway, CheckPoints* checkpoints);
    ~StrategyState();

    static DataModel get_state_status();
    static void set_state_status(const std::string& status);

    virtual void begin();
    virtual void end();
    virtual void run(double price);
};

#endif //STRATEGY_STATE_H
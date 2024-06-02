#ifndef STRATEGY_STATE_H
#define STRATEGY_STATE_H

#include <gateways/gateway.h>
#include <strategy/check_points.h>

enum StrategyStateEnum
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

    bool m_should_end = false;

public:
    StrategyState(Gateway* gateway, CheckPoints* checkpoints);
    ~StrategyState();

    virtual void begin();
    virtual void end();
    virtual void run(double price);
};

#endif //STRATEGY_STATE_H
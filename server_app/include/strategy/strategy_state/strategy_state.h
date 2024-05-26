#ifndef STRATEGY_STATE_H
#define STRATEGY_STATE_H

#include <gateways/gateway.h>
#include <strategy/check_points.h>

class StrategyState
{
protected:
    Gateway* m_gateway;
    CheckPoints* m_checkpoints;

public:
    StrategyState(Gateway* gateway, CheckPoints* checkpoints);
};

#endif //STRATEGY_STATE_H
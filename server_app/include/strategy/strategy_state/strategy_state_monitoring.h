#ifndef STRATEGY_STATE_MONITORING_H
#define STRATEGY_STATE_MONITORING_H

#include <strategy/strategy_state/strategy_state.h>

class StrategyStateMonitoring : public StrategyState
{
public:
    StrategyStateMonitoring(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPointList>& checkpoints);

    virtual void begin();
    virtual void end();
    virtual TaskVoid run(Json data);
};

#endif //STRATEGY_STATE_MONITORING_H
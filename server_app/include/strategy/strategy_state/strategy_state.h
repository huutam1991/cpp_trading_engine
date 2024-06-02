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
    std::shared_ptr<Gateway>& m_gateway;
    std::shared_ptr<CheckPoints>& m_checkpoints;


public:
    StrategyState(std::shared_ptr<Gateway>& gateway, std::shared_ptr<CheckPoints>& checkpoints);
    ~StrategyState();

    static DataModel get_state_status();
    static void set_state_status(const std::string& status);

    virtual void begin();
    virtual void end();
    virtual void run(double price);
};

#endif //STRATEGY_STATE_H
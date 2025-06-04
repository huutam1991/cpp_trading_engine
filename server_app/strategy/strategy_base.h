#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_abstract.h>

template<class Strategy, class StrategyConfig, const char* StrategyDBName>
class StrategyBase : public StategyAbstract
{
    SavableObject<StrategyConfig> m_config;

public:
    StrategyBase() : m_config{SavableObject::load_single_object(StrategyDBName, "config")}
    {

    }
    
    virtual TaskVoid update(StrategyUpdateData data) override
    {

    }
};
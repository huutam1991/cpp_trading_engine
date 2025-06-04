#pragma once

#include <data_model/savable_object.h>

#include <strategy/strategy_abstract.h>
#include <strategy/strategy_state_base.h>

template<class Strategy, class StrategyConfig, const char* StrategyDBName>
class StrategyBase : public StategyAbstract
{
    SavableObject<StrategyConfig> m_config;
    SavableObject<StrategyStateData> m_current_state;
    std::unordered_map<StrategyState, StrategyStateBase*> m_states;
    StrategyState m_previous_state;
    bool m_is_config_updating = false;

public:
    StrategyBase() : 
        m_config{SavableObject<StrategyConfig>::load_single_object(StrategyDBName, "config")},
        m_current_state{SavableObject<StrategyStateData>::load_single_object(StrategyDBName, "state")},
        m_states{StrategyBase::init_states()}
    {
        m_previous_state = m_current_state.object.state;
    }
    
    TaskVoid update(StrategyUpdateData data) override
    {
        // Dont do update when strategy's config is updating
        if (m_is_config_updating == true)
        {
            co_return;
        }

        StrategyState current_state = m_current_state.object.state;

        // Check change state
        if (m_previous_state != current_state)
        {
            // Run end() method of m_previous_state
            if (m_states.find(m_previous_state) != m_states.end())
            {
                m_states[m_previous_state]->end();
            }

            // Run begin() method of new state
            if (m_states.find(current_state) != m_states.end())
            {
                m_states[current_state]->begin();
            }
        }

        // Update [m_previous_state]
        m_previous_state = current_state;

        co_await m_states[current_state]->update(std::move(data));

        co_return;
    }
};
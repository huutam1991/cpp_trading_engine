#pragma once

#include <data_model/savable_object.h>

#include <app_constants.h>
#include <strategy/strategy_abstract.h>
#include <strategy/strategy_state_base.h>

template<class StrategyConfig, const char* StrategyName, size_t eventBaseID>
class StrategyBase : public StrategyAbstract
{
    std::string m_strategy_name;
    SavableObject<StrategyConfig> m_config;
    SavableObject<StrategyStateData> m_current_state;
    std::unordered_map<StrategyState, StrategyStateBase*> m_states;
    StrategyState m_previous_state;

public:
    StrategyBase() : 
        StrategyAbstract(EventBaseManager::get_event_base_by_id(eventBaseID)),
        m_strategy_name(StrategyName),
        m_config{SavableObject<StrategyConfig>::load_single_object(m_strategy_name + "_strategy", "config")},
        m_current_state{SavableObject<StrategyStateData>::load_single_object(m_strategy_name + "_strategy", "state")}
    {}
    
    std::unordered_map<StrategyState, StrategyStateBase*> init_states();

    TaskVoid init() override
    {
        m_states = init_states();
        m_previous_state = m_current_state.object.state;

        co_return;
    }
    
    TaskVoid update(StrategyUpdateData data) override
    {
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

    void on_config_change(StrategyConfig new_config)
    {
        update_config(std::move(new_config)).start_running_on(event_base);
    }

    StrategyConfig& get_config()
    {
        return m_config;
    }

protected:
    virtual TaskVoid update_config(StrategyConfig new_config)
    {
        m_config = std::move(new_config);

        co_return;
    }
};
#pragma once

#include <data_model/savable_object.h>

#include <app_constants.h>
#include <strategy/strategy_abstract.h>
#include <strategy/strategy_state_base.h>

template<class StrategyConfig, const char* StrategyName, size_t eventBaseID>
class StrategyBase : public StrategyAbstract
{
protected:
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

    TaskVoid init() override
    {
        m_states = init_states();
        auto current_state = m_current_state.object.state;

        // Start the current state
        m_states[current_state]->begin();

        m_previous_state = current_state;

        co_return;
    }
    
    TaskVoid update(StrategyUpdateData data) override final
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

    StrategyConfig& get_config_reference()
    {
        return m_config;
    }

    TaskVoid apply_config(StrategyConfig new_config)
    {
        m_config = new_config;
        on_config_change(std::move(new_config));

        co_return;
    }

    // For API requests
    std::string get_name()
    {
        return m_strategy_name;
    }

    Json get_config()
    {
        return m_config.to_json();
    }

    void update_config(Json& data)
    {
        StrategyConfig new_config = StrategyConfig::from_json(data);
        apply_config(std::move(new_config)).start_running_on(event_base);
    }

    virtual Json get_info(Json& params) 
    { 
        return Json(); 
    } 

protected:
    virtual std::unordered_map<StrategyState, StrategyStateBase*> init_states() { return {}; }
    virtual void on_config_change(StrategyConfig new_config) {}
};
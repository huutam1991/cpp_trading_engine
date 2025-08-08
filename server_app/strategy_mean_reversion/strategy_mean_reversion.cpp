

#include <strategy_mean_reversion/strategy_mean_reversion.h>
#include <gateways/gateway_manager.h>
#include <mongo_db/mongo_db.h>
#include <json/json.h>
#include <time/timer.h>
#include <app_constants.h>
#include <app_utils/app_utils.h>

// StrategyState
#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_run.h>
#include <strategy_mean_reversion/strategy_mean_reversion_state/strategy_mr_state_stop.h>

std::unordered_map<std::string, StrategyMeanReversionState*>* StrategyMeanReversion::get_strategy_states()
{
    static std::unordered_map<std::string, StrategyMeanReversionState*> m_strategy_states;

    // Init StrategyState by name
    if (m_strategy_states.size() == 0)
    {
        std::shared_ptr<Gateway>& gateway = StrategyMeanReversion::instance().m_gateway;

        m_strategy_states["RUN"] = new StrategyMeanReversionStateRun(gateway, StrategyMeanReversion::instance().m_config);
        m_strategy_states["STOP"] = new StrategyMeanReversionStateStop(gateway, StrategyMeanReversion::instance().m_config);
    }

    return &m_strategy_states;
}

void StrategyMeanReversion::init()
{
    MeanReversionSimpleGuard g(m_is_init);

    // Load current strategy info
    DataModel config = DataModel::load_single_data_model(STRATEGY_DB_NAME, "mean_reversion_config");

    // If there's no config data available, add default config
    if (config.get_data().has_field("symbol") == false)
    {
        config = {
            {"symbol", "BTCUSDT"},
            {"buy_volumn", (long)300},
            {"buy_at_lower_price", (long)200},
            {"sell_at_higher_price", (long)10},
            {"too_low_price_delta", (long)90},
            {"too_high_price_delta", (long)30},
            {"is_running", false}
        };
    }

    m_config.symbol = std::string(config["symbol"]);
    m_config.buy_volumn = (double)config["buy_volumn"];
    m_config.buy_at_lower_price = (double)config["buy_at_lower_price"];
    m_config.sell_at_higher_price = (double)config["sell_at_higher_price"];
    m_config.too_low_price_delta = (double)config["too_low_price_delta"];
    m_config.too_high_price_delta = (double)config["too_high_price_delta"];
    m_config.is_running = (bool)config["is_running"];

    // Log config
    spdlog::info("StrategyMeanReversion config:");
    spdlog::info("- symbol: {}", m_config.symbol);
    spdlog::info("- buy_volumn: {}", m_config.buy_volumn);
    spdlog::info("- buy_at_lower_price: {}", m_config.buy_at_lower_price);
    spdlog::info("- sell_at_higher_price: {}", m_config.sell_at_higher_price);
    spdlog::info("- too_low_price_delta: {}", m_config.too_low_price_delta);
    spdlog::info("- too_high_price_delta: {}", m_config.too_high_price_delta);
    spdlog::info("- is_running: {}", m_config.is_running);

    // Add price callback + subscribe to symbol
    m_gateway = GatewayManager::instance().get_gateway(ExchangeId::BINANCE);
    m_gateway->register_price_update([this](const Instrument* instrument, double price)
    {
        std::unique_lock lock(m_strategy_mutex);

        // spdlog::debug("symbol: {}, price: {}", instrument->exchange_symbol, price);

        // Can miss some price update
        if (m_has_data_update.is_value_set() == false)
        {
            m_state_data_queue.push(MRPriceUpdate{instrument->exchange_symbol, price});

            // Inform has data update
            m_has_data_update.set_value(true);
        }
    });
    const Instrument* instrument = Instrument::get_instrument_by_symbol(m_gateway->get_exchange(), m_config.symbol);
    m_gateway->subscribe_instruments({instrument});

    // Subscribe order update from OrderManager
    OrderManager::instance().register_order_update([this](Order& order)
    {
        std::unique_lock lock(m_strategy_mutex);

        m_state_data_queue.push(order);

        // Inform has data update
        m_has_data_update.set_value(true);
    });

    //
    m_gateway->check_remove_canceled_orders(m_config.symbol);

    if (m_is_run_update == false)
    {
        m_update_task = update();
        m_update_task.start_running_on(EventBaseManager::get_event_base_by_id(EventBaseID::MEAN_REVERSION_STRATEGY));

        m_is_run_update = true;
    }
}

void StrategyMeanReversion::on_config_change()
{
    // Re-init config
    init();

    // Check start-stop
    if (m_config.is_running)
    {
        run();
    }
    else
    {
        stop();
    }
}

void StrategyMeanReversion::run()
{
    StrategyMeanReversionState::set_state_status("RUN");
}

void StrategyMeanReversion::stop()
{
    StrategyMeanReversionState::set_state_status("STOP");
}

Task<void> StrategyMeanReversion::update()
{
    std::unordered_map<std::string, StrategyMeanReversionState*>* strategy_states = get_strategy_states();
    std::string current_status = "";

    while (true)
    {
        // Dont do update when strategy is init
        if (m_is_init == true)
        {
            co_await Timer::sleep_for(2000);
            continue;
        }

        co_await wait_new_data_update();

        while (m_state_data_queue.size() > 0)
        {
            StrategyMeanReversionData data;
            {
                std::unique_lock lock(m_strategy_mutex);

                data = m_state_data_queue.front();
                m_state_data_queue.pop();
            }

            std::string status = StrategyMeanReversionState::get_state_status()["status"];

            // Check change state
            if (current_status != status)
            {
                // Run end() method of current state
                if ((*strategy_states).find(current_status) != (*strategy_states).end())
                {
                    (*strategy_states)[current_status]->end();
                }

                // Run begin() method of new state
                if ((*strategy_states).find(status) != (*strategy_states).end())
                {
                    (*strategy_states)[status]->begin();
                }
            }

            // Update [current_status]
            current_status = status;

            co_await (*strategy_states)[current_status]->run(std::move(data));
        }

    }
}

Future<bool> StrategyMeanReversion::wait_new_data_update()
{
    return Future<bool>([this](Future<bool>::FutureValue value)
    {
        m_has_data_update = value;
    });
}

Json StrategyMeanReversion::get_orders_chain()
{
    Json info;

    Json orders = MongoDB::instance()
        .set_db_and_collection("order", "order_list")
        .find_many();

    orders.for_each([&info](Json& order)
    {
        if (order["status"] == "FILLED")
        {
            order.remove_field("_id");
            info.push_back(order);
        }
    });

    info.sort([](Json& a, Json& b)
    {
        return (size_t)a["order_id"] < (size_t)b["order_id"];
    });

    return info;
}

Json StrategyMeanReversion::get_open_orders()
{
    std::unordered_map<std::string, StrategyMeanReversionState*>* strategy_states = get_strategy_states();
    std::string status = StrategyMeanReversionState::get_state_status()["status"];

    // Run get_open_orders() method of new state
    if ((*strategy_states).find(status) != (*strategy_states).end())
    {
        return (*strategy_states)[status]->get_open_orders();
    }

    return {};
}
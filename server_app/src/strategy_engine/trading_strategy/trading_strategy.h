#ifndef STRATEGY_ENGINE_TRADING_STRATEGY_H
#define STRATEGY_ENGINE_TRADING_STRATEGY_H

#include <strategy_engine/base_strategy.h>
#include <ring_buffer/ring_buffer.h>
#include <constants.h>
#include <app_constants.h>
#include <data_model/data_model.h>

class Json;
class OrderBook;

using namespace std;

namespace strategy_engine
{
    class HedgingStrategy;

    class TradingStrategy : public BaseStrategy
    {
    public:
        TradingStrategy(const Json inputs);
        ~TradingStrategy();
        Json get_strategy_inputs();

        virtual ResponseStatusCode update_strategy_inputs(Json& strategy_inputs) = 0;
        virtual void stop_trading(bool force_stop_hedging = false) = 0;

        void attach_trade_data_info(Json& trade_report);
    protected:
        void read_common_inputs();
        Json prepare_sending_data_to_FE(Json& data);
        void delete_hedging_strategy_if_need();
        void init_config_data_model();
        void save_strategy_inputs_to_DB();

        vector<shared_ptr<HedgingStrategy>> m_hedging_strategies;
        long m_strategy_id;

        Market m_trading_market_id;
        Market m_scanning_market_id;

        string m_main_symbol;
        Json m_main_symbol_info;
        Json m_main_order_book;
        shared_ptr<OrderBook> m_main_order_book_buffer;

        long m_repeat_times;
        long double m_offset_bid;
        long double m_offset_ask;
        long double m_bid_quantity;
        long double m_ask_quantity;
        bool m_is_arm_bid;
        bool m_is_arm_ask;
        bool m_is_auto_hedge;

        long m_tick_counter = 0;

        DataModel m_config_data_model;

        StrategyState m_trading_state;
        StrategyState m_hedging_state;

        string m_scanning_exchange_name;

        long m_opened_order_id;
        size_t m_order_manager_callback_id = -1;
        string m_client_order_id;

        Json m_trading_data;
    };
}

#endif
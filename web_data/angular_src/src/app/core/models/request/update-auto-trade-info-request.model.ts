import { MarketType, TradeType } from '@core/index';

export interface UpdateAutoTradeInfoRequest {
  is_running?: boolean;
  sub_market: string;
  type: TradeType;
  user_id: string;
  expected_profit?: number;
  replace_times?: number;
  enable_replace?: boolean;
  market_type?: MarketType;
  stop_loss_tick_currency?: number;
  stop_loss_tick_main?: number;
}

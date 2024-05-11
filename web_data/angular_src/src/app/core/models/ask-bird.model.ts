import { TradeType, MarketType } from './type';

export interface AskBidModel {
  expected_profit: number;
  is_running: boolean;
  replace_times: number;
  stop_loss_tick_currency: number;
  stop_loss_tick_main: number;
  sub_market: string;
  type: TradeType;
  market_type: MarketType;
}

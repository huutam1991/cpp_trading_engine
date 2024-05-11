import { ISymbolTickPriceModel } from './symbol-tick-price.model';
import { TheoreticalModel } from './theoretical-order.model';

export interface ArbitrageModel {
  ask_offset: number;
  ask_order_volumn: number;
  ask_price_with_offset: number;
  ask_update_offset: number;
  bid_offset: number;
  bid_order_volumn: number;
  bid_price_with_offset: number;
  bid_update_offset: number;
  currency: ISymbolTickPriceModel;
  main: ISymbolTickPriceModel;
  non_main: ISymbolTickPriceModel;
  non_main_market_tick_size: number;
  profit_with_fee_on_ask: number;
  profit_with_fee_on_bid: number;
  theorical_ask: number;
  theorical_ask_orders: Array<TheoreticalModel>;
  theorical_bid_orders: Array<TheoreticalModel>;
  theorical_bid: number;
  user_config?: any;
}

export interface IArbitrageOffsetModel {
  ask_offset: number;
  ask_order_volumn: number;
  ask_price_with_offset: number;
  ask_update_offset: number;
  bid_offset: number;
  bid_order_volumn: number;
  bid_price_with_offset: number;
  bid_update_offset: number;
  non_main_market_tick_size: number;
  profit_with_fee_on_ask: number;
  profit_with_fee_on_bid: number;
  user_config: any;
}

export class ArbitrageOffsetModel implements IArbitrageOffsetModel {
  ask_offset: number;
  ask_order_volumn: number;
  ask_price_with_offset: number;
  ask_update_offset: number;
  bid_offset: number;
  bid_order_volumn: number;
  bid_price_with_offset: number;
  bid_update_offset: number;
  non_main_market_tick_size: number;
  profit_with_fee_on_ask: number;
  profit_with_fee_on_bid: number;
  user_config: any;

  constructor(data: ArbitrageModel) {
    this.ask_offset = data.ask_offset;
    this.ask_order_volumn = data.ask_order_volumn;
    this.ask_price_with_offset = data.ask_price_with_offset;
    this.ask_update_offset = data.ask_update_offset;
    this.bid_offset = data.bid_offset;
    this.bid_order_volumn = data.bid_order_volumn;
    this.bid_price_with_offset = data.bid_price_with_offset;
    this.bid_update_offset = data.bid_update_offset;
    this.non_main_market_tick_size = data.non_main_market_tick_size;
    this.profit_with_fee_on_ask = data.profit_with_fee_on_ask;
    this.profit_with_fee_on_bid = data.profit_with_fee_on_bid;
    this.user_config = data.user_config;
  }
}

import { ISymbolTickPriceModel } from './symbol-tick-price.model';
import { TheoreticalModel } from './theoretical-order.model';

export interface ScanningMarketModel {
  currency: ISymbolTickPriceModel;
  main: ISymbolTickPriceModel;
  non_main: ISymbolTickPriceModel;
  theorical_ask_orders: Array<TheoreticalModel>;
  theorical_bid_orders: Array<TheoreticalModel>;
  
  main_market_asset: string;
  non_main_ask: number;
  non_main_bid: number;
  order_volumn: number;
  profit_on_ask: number;
  profit_on_bid: number;
  profit_with_fee_on_ask: number;
  profit_with_fee_on_bid: number;
  theorical_ask: number;
  theorical_bid: number;
}

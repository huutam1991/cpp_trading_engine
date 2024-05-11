import { ISymbolTickPriceModel } from './symbol-tick-price.model';

export interface PriceMarketModel {
  currency: ISymbolTickPriceModel;
  main: ISymbolTickPriceModel;
  non_main: ISymbolTickPriceModel;
}

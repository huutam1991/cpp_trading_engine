export interface ISymbolTickPriceModel {
  ask: number;
  bid: number;
  symbol: string;
}

export class SymbolTickPrice implements ISymbolTickPriceModel {
  public ask: number;
  public bid: number;
  public symbol: string;
  constructor(data?: ISymbolTickPriceModel) {
    this.ask = data?.ask || 0;
    this.bid = data?.bid || 0;
    this.symbol = data?.symbol || '';
  }
}

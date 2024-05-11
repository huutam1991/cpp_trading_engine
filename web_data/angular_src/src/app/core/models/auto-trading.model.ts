import { AskBidModel } from './ask-bird.model';

export interface AutoTradingModel {
  ask: AskBidModel;
  bid: AskBidModel;
  orderVolume: number;
  user_id: string;
}

export class AutoTradingData implements AutoTradingModel {
  public ask: AskBidModel;
  public bid: AskBidModel;
  public orderVolume: number;
  public user_id: string;
  constructor(ask: AskBidModel, bid: AskBidModel, user_id: string) {
    this.ask = ask;
    this.bid = bid;
    this.orderVolume = 0;
    this.user_id = user_id;
  }
}

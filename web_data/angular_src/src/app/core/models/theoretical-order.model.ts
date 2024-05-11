import { SideType } from './type';

export interface TheoreticalModel {
  BNB_fee: number;
  price: number;
  price_with_offset?: number;
  quantity: number;
  side: SideType;
  symbol: string;
}

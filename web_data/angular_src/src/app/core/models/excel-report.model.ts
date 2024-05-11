export interface ExcelReportModel {
  orderId: number;
  BNB_fee: string;
  ask: number;
  bid: number;
  date_time: number;
  finish_calculation_time: Date;
  finish_place_order_time: Date;
  strategy: string;
  symbol: string;
  tick_time: string;
  user_id: string;
}

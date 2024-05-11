import * as Model from '@common/models';

// export const ORDER_STATUS_COLOR_MAP = {
//   FILLED: '#198754',
//   NEW: '#6c757d',
//   CANCELED: '#ef4444',
//   PARTIALLY_FILLED: '#ffc107',
// };

export const STRATEGY_SIDE_COLOR_MAP = {
  'MM Arbitrage': '#007bff',
  'Market Scanning': '#22c55e',
};

export const EXCEL_REPORT_TABLE_CONFIG: Model.ITableConfig = {
  columnDefinition: [
    new Model.IndexColumn('index', 'index', 2),
    new Model.NumberColumn('orderId', 'Order Id', 8),
    new Model.TextColumn('symbol', 'Symbol', 8),
    new Model.TypeColumn('strategy', 'Strategy', 10, STRATEGY_SIDE_COLOR_MAP),

    new Model.TextColumn('bid', 'Bid price', 8),
    new Model.TextColumn('ask', 'Ask price', 8),
    new Model.TextColumn('BNB_fee', 'BNB price', 8),

    new Model.TextColumn('tick_time', 'Ticker Time', 14),
    new Model.TextColumn('finish_calculation_time', 'Finish Calculation', 14),
    new Model.TextColumn('finish_place_order_time', 'Finish Place Order', 14),
    new Model.TextColumn('user_id', 'User', 5),
  ],
  title: '',
  btnExport: false,
  btnAdd: false,
};

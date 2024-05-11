import * as Model from '@common/models';

export const ORDER_SIDE_COLOR_MAP = {
  SELL: '#ef4444',
  BUY: '#22c55e',
};

export const SUGGESTED_ORDER_TABLE_CONFIG: Model.ITableConfig = {
  columnDefinition: [
    new Model.IndexColumn('index', 'index', 2),
    new Model.TypeColumn('side', 'Side', 5, ORDER_SIDE_COLOR_MAP),
    new Model.TextColumn('symbol', 'Symbol', 8),
    new Model.TextColumn('price', 'Price', 8),
    new Model.TextColumn('quantity', 'Quantity', 8),
    new Model.TextColumn('BNB_fee', 'Fee in BNB', 8),
  ],
};

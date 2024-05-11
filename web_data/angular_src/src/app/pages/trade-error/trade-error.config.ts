import * as Model from '@common/models';

export const ORDER_STATUS_COLOR_MAP = {
  FILLED: '#198754',
  NEW: '#6c757d',
  CANCELED: '#ef4444',
  PARTIALLY_FILLED: '#ffc107',
};

export const TRADE_ERROR = {
  SELL: '#ef4444',
  BUY: '#22c55e',
};

export const TRADE_ERROR_TABLE_CONFIG: Model.ITableConfig = {
  columnDefinition: [
    new Model.IndexColumn('index', 'index', 2),
    new Model.TypeColumn('side', 'Side', 5, TRADE_ERROR),
    new Model.TextColumn('symbol', 'Symbol', 6),
    new Model.TextColumn('price', 'Price', 6),
    new Model.TextColumn('quantity', 'Quantity', 6),
    new Model.TextColumn('date_time', 'Date Time', 10),
    new Model.TextColumn('code', 'Code', 5),
    new Model.TextColumn('msg', 'Message', 25, false, undefined, undefined, '#c82333'),
  ],
  title: '',
  btnExport: false,
  btnAdd: false,
};

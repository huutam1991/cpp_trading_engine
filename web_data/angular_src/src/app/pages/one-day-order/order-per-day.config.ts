import * as Model from '@common/models';

export const ORDER_STATUS_COLOR_MAP = {
  FILLED: '#198754',
  NEW: '#6c757d',
  CANCELED: '#ef4444',
  WAITING: '#17a2b8',
  PARTIALLY_FILLED: '#ffc107',
  REJECTED: '#343a40'
};

export const ORDER_SIDE_COLOR_MAP = {
  SELL: '#ef4444',
  BUY: '#22c55e',
};

export const ORDER_PER_DAY_TABLE_CONFIG: Model.ITableConfig = {
  columnDefinition: [
    new Model.IndexColumn('index', 'index', 2),
    new Model.NumberColumn('orderId', 'Order Id', 10),
    new Model.TypeColumn('side', 'Side', 10, ORDER_SIDE_COLOR_MAP),
    new Model.TextColumn('symbol', 'Symbol', 10),
    new Model.TextColumn('price', 'Price', 10),
    new Model.TextColumn('quantity', 'Quantity', 10),
    new Model.TextColumn('last_executed_quantity', 'Filled Quantity', 10),
    new Model.TextColumn('type', 'Type', 15),
    new Model.StatusColumn('status', 'Status', 10, ORDER_STATUS_COLOR_MAP),
  ],
  title: '',
  btnExport: false,
  btnAdd: false,
};

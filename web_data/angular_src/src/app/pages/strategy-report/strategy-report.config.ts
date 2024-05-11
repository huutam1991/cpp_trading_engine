import * as Model from '@common/models';

export const STRATEGY_NAME_SIDE_COLOR_MAP = {
  'MM Arbitrage': '#007bff',
  'Market Scanning': '#22c55e',
};


export const STRATEGY_REPORT_TABLE_CONFIG: Model.ITableConfig = {
  columnDefinition: [
    new Model.IndexColumn('index', 'index', 2),
    new Model.TypeColumn('strategy_name', 'Strategy', 10, STRATEGY_NAME_SIDE_COLOR_MAP),
    new Model.TextColumn('symbol_list', 'Symbol', 10),
    new Model.TextColumn('user_id', 'User', 10),
    new Model.TextColumn('hit_time_str', 'Hit time', 10),
    new Model.NumberColumn(
      'profit',
      'Profit',
      10,
      false,
      false,
      false,
      false,
      undefined,
      true
    ),
  ],
  title: '',
  btnExport: false,
  btnAdd: false,
};

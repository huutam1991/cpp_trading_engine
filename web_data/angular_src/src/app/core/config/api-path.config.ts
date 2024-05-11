export const ApiPathConfig = {
  auth: {
    login: '/login',
  },
  symbols: '/all_symbols',
  autoTrade: '/auto_trade_info',
  orderPerDay: '/24h_orders',
  tradeError: '/trade_errors',
  scanMarket: '/scan_market',
  stopScanMarket: '/stop_scanning_market',
  updateOrderVolume: '/update_order_volumn',
  strategyReport: '/trading_result_list',
  excelReport: '/price_ticker_list',
  mmArbitrage: {
    scan: '/mm_arbitrage_scan_market',
    stop: '/mm_arbitrage_stop_market',
    config: '/mm_arbitrage_update_config',
  },

  downloadFile: {
    excelReport: '/report_price_ticker',
    strategyReport: '/report_trading_result',
  },

  setting: {
    connect: '/connect_binance_simulator',
    getConfig: '/get_config',
    getDbNameList: '/get_db_name_list',
    getSymbolNameList: '/get_collection_name_list',
    setConfig: '/set_config ',
    getBackTestingMode: '/get_back_testing_mode',
    switchBackTestingMode: '/switch_back_testing_mode',
    cleanBackTestingData: '/clean_back_testing_data'
  },
};

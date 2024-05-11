import { environment } from '@env/environment';
import { ApiPathConfig } from './api-path.config';

export class ApiPath {
  //AUTH MODULE
  public static LOGIN = environment.api_service.concat(
    ApiPathConfig.auth.login
  );

  public static SYMBOL = environment.api_service.concat(ApiPathConfig.symbols);

  public static AUTO_TRADE_INFO = environment.api_service.concat(
    ApiPathConfig.autoTrade
  );

  public static ORDER_PER_DAY = environment.api_service.concat(
    ApiPathConfig.orderPerDay
  );
  public static TRADE_ERROR = environment.api_service.concat(
    ApiPathConfig.tradeError
  );
  public static SCAN_MARKET = environment.api_service.concat(
    ApiPathConfig.scanMarket
  );
  public static STOP_SCAN_MARKET = environment.api_service.concat(
    ApiPathConfig.stopScanMarket
  );
  public static UPDATE_ORDER_VOLUME = environment.api_service.concat(
    ApiPathConfig.updateOrderVolume
  );
  public static STRATEGY_REPORT = environment.api_service.concat(
    ApiPathConfig.strategyReport
  );

  public static EXCEL_REPORT = environment.api_service.concat(
    ApiPathConfig.excelReport
  );

  public static MM_ARBITRAGE_SCAN = environment.api_service.concat(
    ApiPathConfig.mmArbitrage.scan
  );
  public static MM_ARBITRAGE_STOP = environment.api_service.concat(
    ApiPathConfig.mmArbitrage.stop
  );
  public static MM_ARBITRAGE_UPDATE = environment.api_service.concat(
    ApiPathConfig.mmArbitrage.config
  );

  public static EXCEL_FILE_DOWNLOAD = environment.api_service.concat(
    ApiPathConfig.downloadFile.excelReport
  );
  public static STRATEGY_FILE_DOWNLOAD = environment.api_service.concat(
    ApiPathConfig.downloadFile.strategyReport
  );

  //SETTING
  public static CONNECT_BINANCE_SIMULATOR = environment.api_service.concat(
    ApiPathConfig.setting.connect
  );
  public static GET_CONFIG = environment.api_service.concat(
    ApiPathConfig.setting.getConfig
  );
  public static GET_DB_NAMES = environment.api_service.concat(
    ApiPathConfig.setting.getDbNameList
  );
  public static GET_SYMBOL_NAMES = environment.api_service.concat(
    ApiPathConfig.setting.getSymbolNameList
  );
  public static SET_CONFIG = environment.api_service.concat(
    ApiPathConfig.setting.setConfig
  );
  public static GET_MODE_TESTING = environment.api_service.concat(
    ApiPathConfig.setting.getBackTestingMode
  );
  public static SWITCH_MODE_TESTING = environment.api_service.concat(
    ApiPathConfig.setting.switchBackTestingMode
  );
  public static CLEAN_BACK_TESTING_DATA = environment.api_service.concat(
    ApiPathConfig.setting.cleanBackTestingData
  );

}

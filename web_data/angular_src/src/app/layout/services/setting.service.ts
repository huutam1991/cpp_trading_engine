import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ApiPath } from '@core/config';
import { ConfigSidenavModel, ModeType } from '@core/models';
import { BaseResponseModel } from '@core/models/base';
import { BackTestingRequest } from '@core/models/request';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class SettingService {
  constructor(protected httpClient: HttpClient) {}

  public connectToBinanceSimulator(): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.CONNECT_BINANCE_SIMULATOR}`
    );
  }

  public getConfig(): Observable<BaseResponseModel<ConfigSidenavModel>> {
    return this.httpClient.get<BaseResponseModel<ConfigSidenavModel>>(
      `${ApiPath.GET_CONFIG}`
    );
  }

  public getDbNames(): Observable<BaseResponseModel<Array<string>>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.GET_DB_NAMES}`
    );
  }

  public getSymbolNames(dbName: string): Observable<BaseResponseModel<Array<string>>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.GET_SYMBOL_NAMES}?db_name=${dbName}`
    );
  }

  public setConfig(
    request: BackTestingRequest
  ): Observable<BaseResponseModel<any>> {
    return this.httpClient.post<BaseResponseModel<any>>(
      `${ApiPath.SET_CONFIG}`,
      { ...request }
    );
  }

  public switchBackTestingMode(
    mode: ModeType
  ): Observable<BaseResponseModel<any>> {
    return this.httpClient.post<BaseResponseModel<any>>(
      `${ApiPath.SWITCH_MODE_TESTING}`,
      { back_testing_mode: mode }
    );
  }

  public getBackTestingMode(
  ): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.GET_MODE_TESTING}`
    );
  }

  public cleanBackTestingData(
  ): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.CLEAN_BACK_TESTING_DATA}`
    );
  }

}

import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ApiPath } from '@core/config';
import { ExcelReportModel, OrderModel, StrategyModel } from '@core/models';
import { BaseResponseModel } from '@core/models/base';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class ReportService {
  constructor(protected httpClient: HttpClient) {}

  public getOrders(
    from: number,
    to: number
  ): Observable<BaseResponseModel<Array<OrderModel>>> {
    return this.httpClient.get<BaseResponseModel<Array<OrderModel>>>(
      `${ApiPath.ORDER_PER_DAY}?from=${from}&to=${to}`
    );
  }

  public getTradeErrors(
    from: number,
    to: number
  ): Observable<BaseResponseModel<Array<OrderModel>>> {
    return this.httpClient.get<BaseResponseModel<Array<OrderModel>>>(
      `${ApiPath.TRADE_ERROR}?from=${from}&to=${to}`
    );
  }

  public strategyReport(
    from: number,
    to: number
  ): Observable<BaseResponseModel<Array<StrategyModel>>> {
    return this.httpClient.get<BaseResponseModel<Array<StrategyModel>>>(
      `${ApiPath.STRATEGY_REPORT}?from=${from}&to=${to}`
    );
  }

  public excelReport(
    from: number,
    to: number
  ): Observable<BaseResponseModel<Array<ExcelReportModel>>> {
    return this.httpClient.get<BaseResponseModel<Array<ExcelReportModel>>>(
      `${ApiPath.EXCEL_REPORT}?from=${from}&to=${to}`
    );
  }

  public excelReportFileDownload(from: number, to: number): Observable<any> {
    return this.httpClient.get<any>(
      `${ApiPath.EXCEL_FILE_DOWNLOAD}?from=${from}&to=${to}`,
      {
        responseType: 'blob' as 'json',
      }
    );
  }

  public strategyReportFileDownload(from: number, to: number): Observable<any> {
    return this.httpClient.get<any>(
      `${ApiPath.STRATEGY_FILE_DOWNLOAD}?from=${from}&to=${to}`,
      {
        responseType: 'blob' as 'json',
      }
    );
  }
}

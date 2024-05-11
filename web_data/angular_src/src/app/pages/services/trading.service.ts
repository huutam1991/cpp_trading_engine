import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ApiPath } from '@core/config';
import { BaseResponseModel } from '@core/models/base';
import { BehaviorSubject, Observable, Subject, map, tap } from 'rxjs';
import { WebsocketService } from './websocket.service';
import { environment } from '@env/environment';
import {
  ArbitrageModel,
  AskBidModel,
  PriceMarketModel,
  TheoreticalBidAskModel,
  TheoreticalModel,
} from '@core/models';
import { UpdateConfigRequest } from '@core/models/request';

@Injectable({
  providedIn: 'root',
})
export class TradingService {
  public messages: Subject<any> = new Subject();

  constructor(protected httpClient: HttpClient) {}

  public getAllSymbols(): Observable<BaseResponseModel<Array<string>>> {
    return this.httpClient
      .get<BaseResponseModel<Array<string>>>(ApiPath.SYMBOL)
      .pipe(tap((res) => (this.allSymbols = res.data)));
  }

  public getAutoTradingInfo(): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(ApiPath.AUTO_TRADE_INFO);
  }

  public updateAutoTradingInfo(data: any): Observable<BaseResponseModel<any>> {
    return this.httpClient.post<BaseResponseModel<any>>(
      ApiPath.AUTO_TRADE_INFO,
      { auto_trade_info: data }
    );
  }
  public scanMarket(data: string): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.SCAN_MARKET}?sub_market=${data}`
    );
  }

  public stopScanMarket(data: string): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.STOP_SCAN_MARKET}?sub_market=${data}`
    );
  }
  public updateOrderVolume(
    data: string,
    volume: number
  ): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.UPDATE_ORDER_VOLUME}?sub_market=${data}&order_volumn=${volume}`
    );
  }

  //#region MM Arbitrage
  public mmArbitrageScan(data: string): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.MM_ARBITRAGE_SCAN}?sub_market=${data}`
    );
  }

  public mmArbitrageStop(data: string): Observable<BaseResponseModel<any>> {
    return this.httpClient.get<BaseResponseModel<any>>(
      `${ApiPath.MM_ARBITRAGE_SCAN}?sub_market=${data}`
    );
  }
  public mmArbitrageUpdateConfig(
    data: UpdateConfigRequest
  ): Observable<BaseResponseModel<any>> {
    const sub = data.sub_market;
    delete data.sub_market;
    return this.httpClient.post<BaseResponseModel<any>>(
      `${ApiPath.MM_ARBITRAGE_UPDATE}?sub_market=${sub}`,
      {
        config: data,
      }
    );
  }
  //#endregion

  //#region Get all Symbols
  public symbols$: BehaviorSubject<Array<string>> = new BehaviorSubject<
    Array<string>
  >([]);

  get allSymbols(): Array<string> {
    return this.symbols$.getValue();
  }

  set allSymbols(value: Array<string>) {
    this.symbols$.next(value);
  }
  //#endregion

  //#region Current Sub Market
  public currentSubMarket$: BehaviorSubject<string | null> =
    new BehaviorSubject<string | null>(null);

  get currentSubMarket(): string | null {
    return this.currentSubMarket$.getValue();
  }

  set currentSubMarket(value: string | null) {
    this.currentSubMarket$.next(value);
  }
  //#endregion

  //#region Sub-Market Bid
  public subMarketBid$: BehaviorSubject<AskBidModel | null> =
    new BehaviorSubject<AskBidModel | null>(null);
  get subMarketBid(): AskBidModel | null {
    return this.subMarketBid$.getValue();
  }

  set subMarketBid(value: AskBidModel | null) {
    this.subMarketBid$.next(value);
  }
  //#endregion

  //#region Sub-Market Ask
  public subMarketAsk$: BehaviorSubject<AskBidModel | null> =
    new BehaviorSubject<AskBidModel | null>(null);

  get subMarketAsk(): AskBidModel | null {
    return this.subMarketAsk$.getValue();
  }

  set subMarketAsk(value: AskBidModel | null) {
    this.subMarketAsk$.next(value);
  }
  //#endregion

  //#region Price Market
  public priceMarket$: BehaviorSubject<PriceMarketModel | null> =
    new BehaviorSubject<PriceMarketModel | null>(null);

  get priceMarket(): PriceMarketModel | null {
    return this.priceMarket$.getValue();
  }

  set priceMarket(value: PriceMarketModel | null) {
    this.priceMarket$.next(value);
  }
  //#endregion

  //#region Bid Suggested Order
  public bidOrder$: BehaviorSubject<Array<TheoreticalModel>> =
    new BehaviorSubject<Array<TheoreticalModel>>([]);

  get bidOrder(): Array<TheoreticalModel> {
    return this.bidOrder$.getValue();
  }

  set bidOrder(value: Array<TheoreticalModel>) {
    this.bidOrder$.next(value);
  }
  //#endregion

  //#region Bid Suggested Order
  public askOrder$: BehaviorSubject<Array<TheoreticalModel>> =
    new BehaviorSubject<Array<TheoreticalModel>>([]);

  get askOrder(): Array<TheoreticalModel> {
    return this.askOrder$.getValue();
  }

  set askOrder(value: Array<TheoreticalModel>) {
    this.askOrder$.next(value);
  }
  //#endregion

  //#region Theoretical Bid Ask
  public theoreticalBidAsk$: BehaviorSubject<TheoreticalBidAskModel | null> =
    new BehaviorSubject<TheoreticalBidAskModel | null>(null);

  get theoreticalBidAsk(): TheoreticalBidAskModel | null {
    return this.theoreticalBidAsk$.getValue();
  }

  set theoreticalBidAsk(value: TheoreticalBidAskModel | null) {
    this.theoreticalBidAsk$.next(value);
  }
  //#endregion

  //#region Theoretical Bid Ask
  public orderVolume$: BehaviorSubject<number> = new BehaviorSubject<number>(0);

  get orderVolume(): number {
    return this.orderVolume$.getValue();
  }

  set orderVolume(value: number) {
    this.orderVolume$.next(value);
  }
  //#endregion

  //#region MM Arbitrage
  public mmArbitrage$: BehaviorSubject<ArbitrageModel | null> =
    new BehaviorSubject<ArbitrageModel | null>(null);

  get mmArbitrage(): ArbitrageModel | null {
    return this.mmArbitrage$.getValue();
  }

  set mmArbitrage(value: ArbitrageModel | null) {
    this.mmArbitrage$.next(value);
  }
  //#endregion
}

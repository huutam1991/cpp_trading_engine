import { TokenType } from '@angular/compiler';
import { Injectable } from '@angular/core';
import { StorageKey } from '@common/enum';
import { environment } from '@env/environment';
import { CookieService } from 'ngx-cookie-service';
import { config } from 'rxjs';
import { webSocket, WebSocketSubject } from 'rxjs/webSocket';

@Injectable()
export class WebsocketService {
  public autoTradeSubject$!: WebSocketSubject<any>;
  public scanningMarketSubject$!: WebSocketSubject<any>;
  public scanningMarketNotificationSubject$!: WebSocketSubject<any>;

  public orderStatus$!: WebSocketSubject<any>;
  public profitSubject$!: WebSocketSubject<any>;
  public mmArbitrageNotificationSubject$!: WebSocketSubject<any>;

  public mmArbitrageSubject$!: WebSocketSubject<any>;

  constructor(private cookieService: CookieService) {}

  public connect(): void {
    this.autoTradeSubject$ = webSocket(environment.WS_ENDPOINT);
    this.autoTradeSubject$.next({
      channel: 'auto_trade',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });

    this.scanningMarketNotificationSubject$ = webSocket(
      environment.WS_ENDPOINT
    );
    this.scanningMarketNotificationSubject$.next({
      channel: 'scanning_market_notification',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });

    this.scanningMarketSubject$ = webSocket(environment.WS_ENDPOINT);
    this.scanningMarketSubject$.next({
      channel: 'scanning_market',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });
  }

  public connectForOrderPerDay(): void {
    this.orderStatus$ = webSocket(environment.WS_ENDPOINT);
    this.orderStatus$.next({
      channel: 'order_status',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });

    this.profitSubject$ = webSocket(environment.WS_ENDPOINT);
    this.profitSubject$.next({
      channel: 'profit',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });
  }

  public connectForMmArbitrage(): void {
    this.mmArbitrageSubject$ = webSocket(environment.WS_ENDPOINT);
    this.mmArbitrageSubject$.next({
      channel: 'mm_arbitrage',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });
    
    this.profitSubject$ = webSocket(environment.WS_ENDPOINT);
    this.profitSubject$.next({
      channel: 'profit',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });

    this.mmArbitrageNotificationSubject$ = webSocket(environment.WS_ENDPOINT);
    this.mmArbitrageNotificationSubject$.next({
      channel: 'mm_arbitrage_notification',
      method: 'subscribe',
      token: this.cookieService.get(StorageKey.TOKEN),
    });
  }

  public close() {
    this.autoTradeSubject$.next(null);
    this.autoTradeSubject$.complete();

    this.scanningMarketNotificationSubject$.next(null);
    this.scanningMarketNotificationSubject$.complete();

    this.scanningMarketSubject$.next(null);
    this.scanningMarketSubject$.complete();
  }

  public closeForOrderPerDay(): void {
    this.orderStatus$.next(null);
    this.orderStatus$.complete();

    this.profitSubject$.next(null);
    this.profitSubject$.complete();
  }

  public closeForMmArbitrage(): void {
    this.mmArbitrageSubject$.next(null);
    this.mmArbitrageSubject$.complete();

    this.profitSubject$.next(null);
    this.profitSubject$.complete();

    this.mmArbitrageNotificationSubject$.next(null);
    this.mmArbitrageNotificationSubject$.complete();
  }
}

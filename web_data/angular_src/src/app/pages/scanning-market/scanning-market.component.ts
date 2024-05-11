import {
  ChangeDetectorRef,
  Component,
  OnDestroy,
  OnInit,
  ViewChild,
} from '@angular/core';
import { FormBuilder } from '@angular/forms';
import { LoginService } from '@auth/services/login.service';
import { NotificationService } from '@common/services/notification.service';
import {
  AskBidModel,
  AutoTradingModel,
  ISymbolTickPriceModel,
  ScanningMarketModel,
} from '@core/models';
import { PriceMarketComponent } from '@parts/index';
import { BehaviorSubject, Subject, finalize, takeUntil } from 'rxjs';
import { TradingService } from '../services/trading.service';
import { WebsocketService } from '../services/websocket.service';

@Component({
  selector: 'app-scanning-market',
  templateUrl: './scanning-market.component.html',
  styleUrls: ['./scanning-market.component.scss'],
  // changeDetection: ChangeDetectionStrategy.OnPush,
})
export class ScanningMarketComponent implements OnInit, OnDestroy {
  public dataAutoTrading: BehaviorSubject<Array<any>> = new BehaviorSubject<
    Array<any>
  >([]);
  public dataSource: Array<any> = [];
  public baseData: any;
  public dataTrading: any;

  public currentSub: string = '';
  public orderVolumeInput: number = 0;
  public isLoading: boolean = false;

  private readonly destroyed$ = new Subject();

  constructor(
    private fb: FormBuilder,
    private tradingService: TradingService,
    private ws: WebsocketService,
    private ref: ChangeDetectorRef,
    private authService: LoginService
  ) {}

  @ViewChild('priceMarket') priceMarket: PriceMarketComponent =
    new PriceMarketComponent(this.fb, this.tradingService, this.ref);

  ngOnDestroy(): void {
    this.ws.close();
    this.destroyed$.next(null);
    this.destroyed$.complete();
  }

  ngOnInit(): void {
    this._initDataTrading();
    this.ws.connect();

    this.ws.autoTradeSubject$
      .pipe(takeUntil(this.destroyed$))
      .subscribe((res) => {
        if (res && res.data) {
          let dataUpdated;
          this.dataAutoTrading.getValue().map((result) => {
            if (result[0] === res.data.sub_market) {
              dataUpdated = res.data;
            }
          });

          if (res.data.type === 'bid') {
            this.tradingService.subMarketBid = res.data;
            this.baseData[res.data.sub_market].bid = dataUpdated;
          } else {
            this.tradingService.subMarketAsk = res.data;
            this.baseData[res.data.sub_market].ask = dataUpdated;
          }
        }
      });

    this.ws.scanningMarketNotificationSubject$
      .pipe(takeUntil(this.destroyed$))
      .subscribe((res) => {
        if (res && res.data && !res.error) {
          console.log(res);
        }
      });

    this.ws.scanningMarketSubject$
      .pipe(takeUntil(this.destroyed$))
      .subscribe((res) => {
        if (res && res.data) {
          this.dataTrading.map((el: any) => {
            const isMapped =
              el[0].includes(res.data.currency.symbol) &&
              el[0].includes(res.data.non_main.symbol) &&
              el[0].includes(res.data.main.symbol);
            if (isMapped && el[2]) {
              el = el[2].orderVolume = res.data.order_volumn;
            }
            return el;
          });

          this.dataAutoTrading.next(this.dataTrading);

          const data = this.getCurrentData(res.data);
          if (data) {
            this.tradingService.priceMarket = {
              currency: data.currency,
              main: data.main,
              non_main: data.non_main,
            };
            this.tradingService.bidOrder = data.theorical_bid_orders;
            this.tradingService.askOrder = data.theorical_ask_orders;
            this.tradingService.theoreticalBidAsk = data;
            if (this.tradingService.orderVolume !== data.order_volumn) {
              this.tradingService.orderVolume = this.orderVolumeInput =
                data.order_volumn;
            }
            this.ref.detectChanges();
          }
        }
      });
  }

  onScanMarket(isScanMarket: boolean): void {
    if (isScanMarket) {
      if (this.priceMarket.priceGroup.invalid) return;
      this.tradingService.scanMarket(this._getSubMarket()).subscribe((res) => {
        if (res && !res.error) {
          this._initDataTrading();
        }
      });
    } else {
      this.tradingService
        .stopScanMarket(this._getSubMarket())
        .subscribe((res) => {
          if (res && !res.error) {
            this._initDataTrading();
          }
        });
    }
  }

  onUpdateOrderVolume(): void {
    if (!this.orderVolumeInput) return;

    this.tradingService
      .updateOrderVolume(this._getSubMarket(), this.orderVolumeInput)
      .subscribe((res) => {
        if (res && !res.error) {
          this._initDataTrading();
        }
      });
  }

  onSubMarketClick(sub: any): void {
    this.currentSub = sub[0];
    this.tradingService.currentSubMarket = sub[0];
    this.tradingService.subMarketAsk = sub[1].ask as AskBidModel;
    this.tradingService.subMarketBid = sub[1].bid as AskBidModel;
  }

  trackFnBySub = (index: any, item: any[]) => item[0];

  private _initDataTrading(): void {
    this.isLoading = true;
    this.tradingService.getAllSymbols().subscribe();
    this.tradingService
      .getAutoTradingInfo()
      .pipe(
        takeUntil(this.destroyed$),
        finalize(() => (this.isLoading = false))
      )
      .subscribe((res) => {
        if (res && res.data) {
          this.baseData = res.data;
          this.dataTrading = Object.entries(this.baseData);
          this.dataTrading.map((el: any) => el.push({ orderVolume: 0 }));
          this.dataAutoTrading.next(this.dataTrading);

          this.currentSub = this.dataTrading[0][0];
          this.tradingService.currentSubMarket = this.currentSub;
          this.authService.userId$.next(
            (this.dataTrading[0][1] as AutoTradingModel).user_id
          );
          this.tradingService.subMarketBid = (
            this.dataTrading[0][1] as AutoTradingModel
          ).bid;
          this.tradingService.subMarketAsk = (
            this.dataTrading[0][1] as AutoTradingModel
          ).ask;
        }
      });
  }

  private getCurrentData(data: any): ScanningMarketModel | null {
    if (data && this.tradingService.currentSubMarket) {
      const sub = this.tradingService.currentSubMarket.split('-');

      const nonMain: ISymbolTickPriceModel =
        data.non_main.symbol === sub[2] ? data.non_main : null;
      const currency: ISymbolTickPriceModel =
        data.currency.symbol === sub[1] ? data.currency : null;
      const main: ISymbolTickPriceModel =
        data.main.symbol === sub[0] ? data.main : null;

      if (nonMain && currency && main)
        return {
          non_main: nonMain,
          currency: currency,
          main: main,
          ...data,
        };
    }

    return null;
  }

  private _getSubMarket = (): string =>
    `${this.priceMarket.priceGroup.value.mainMarket}-${this.priceMarket.priceGroup.value.currency}-${this.priceMarket.priceGroup.value.nonMarket}`;
}

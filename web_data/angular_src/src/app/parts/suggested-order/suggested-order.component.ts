
import { ChangeDetectorRef, Component, Input, OnInit } from '@angular/core';
import { FormBuilder, FormControl, FormGroup } from '@angular/forms';
import { LoginService } from '@auth/services/login.service';
import { AskBidModel, MarketType, TheoreticalModel, TradeType } from '@core/models';
import { UpdateAutoTradeInfoRequest } from '@core/models/request';
import { BehaviorSubject, Observable, finalize, of } from 'rxjs';
import { TradingService } from 'src/app/pages/services/trading.service';
import { SUGGESTED_ORDER_TABLE_CONFIG } from './table.config';

@Component({
  selector: 'app-suggested-order',
  templateUrl: './suggested-order.component.html',
  styleUrls: ['./suggested-order.component.scss']
})
export class SuggestedOrderComponent implements OnInit {
  @Input() type: TradeType = 'bid';

  public typeName: string = "Bid";
  public tableConfig = SUGGESTED_ORDER_TABLE_CONFIG;
  public dataSource: Observable<Array<any>> = new Observable();
  public isGoClicked = false;
  public orderGroup: FormGroup = new FormGroup({});
  public currentMarket: Array<string> = [];
  public isLoading = false;
  public marketType: MarketType = 'LIMIT';

  constructor(
    private ref: ChangeDetectorRef,
    private tradingService: TradingService,
    private formBuilder: FormBuilder,
    private authService: LoginService
  ) {}

  ngOnInit(): void {
    this._initGroup();

    this.typeName = this.type == 'bid' ? 'Bid' : 'Ask';

    if (this.type == 'bid') {
      this.do_subscribe(this.tradingService.bidOrder$, this.tradingService.subMarketBid$);
    } else if (this.type == 'ask') {
      this.do_subscribe(this.tradingService.askOrder$, this.tradingService.subMarketAsk$);
    }

    this.tradingService.currentSubMarket$.subscribe((res) => {
      if (res) {
        this.currentMarket = res.split('-');
      }
    });
  }

  private do_subscribe(order: BehaviorSubject<Array<TheoreticalModel>>, subMarket: BehaviorSubject<AskBidModel | null>): void {
    order.subscribe((res) => {
      if (res) {
        this.dataSource = of(res);
        this.ref.detectChanges();
      }
    });
    subMarket.subscribe((res) => {
      if (res) {
        this.isGoClicked = res.is_running;
        this.marketType = res.market_type;
        this._initGroup(res);
      }
    });
  }

  onUpdateMarketType(marketType: MarketType): void {
    this._updateMarketType(marketType);
  }

  onUpdateTickClicked(): void {
    if (!this.tradingService.currentSubMarket) return;

    const request: UpdateAutoTradeInfoRequest = {
      stop_loss_tick_currency: this.orderGroup.get('stop_loss_tick_currency')
        ?.value,
      stop_loss_tick_main: this.orderGroup.get('stop_loss_tick_main')?.value,
      sub_market: this.tradingService.currentSubMarket,
      type: this.type,
      user_id: this.authService.userId$.getValue(),
    };
    this._updateAutoTradingHttp(request);
  }

  runningOrder(isRunning: boolean): void {
    this._updateOrder(isRunning);
  }

  countTime(isPlus: boolean): void {
    if (!isPlus && this.orderGroup.get('replace_times')?.value === 0) return;
    this._updateWithReplaceTime(isPlus);
  }

  private _initGroup(data?: AskBidModel): void {
    this.orderGroup = this.formBuilder.group({
      stop_loss_tick_currency: new FormControl(
        data ? data.stop_loss_tick_currency : 0
      ),
      stop_loss_tick_main: new FormControl(data ? data.stop_loss_tick_main : 0),
      expected_profit: new FormControl({
        value: data ? data.expected_profit : null,
        disabled: this.isGoClicked,
      }),
      replace_times: new FormControl(data ? data.replace_times : 0),
    });
  }

  private _updateOrder(isGo: boolean): void {
    if (!this.tradingService.currentSubMarket) return;
    const request: UpdateAutoTradeInfoRequest = {
      is_running: isGo,
      sub_market: this.tradingService.currentSubMarket,
      type: this.type,
      user_id: this.authService.userId$.getValue(),
      expected_profit: isGo
        ? +this.orderGroup.get('expected_profit')?.value
        : 0,
      enable_replace: this.marketType == 'STOP_LOSS_ONLY' ? false : true,
    };

    this._updateAutoTradingHttp(request);
  }

  private _updateWithReplaceTime(isPlus: boolean): void {
    if (!this.tradingService.currentSubMarket) return;

    const request: UpdateAutoTradeInfoRequest = {
      sub_market: this.tradingService.currentSubMarket,
      type: this.type,
      user_id: this.authService.userId$.getValue(),
      replace_times: isPlus
        ? this.orderGroup.get('replace_times')?.value + 1
        : this.orderGroup.get('replace_times')?.value - 1,
      enable_replace: this.marketType == 'STOP_LOSS_ONLY' ? false : true,
    };
    this._updateAutoTradingHttp(request);
  }

  private _updateMarketType(marketType: MarketType): void {
    if (!this.tradingService.currentSubMarket) return;

    const request: UpdateAutoTradeInfoRequest = {
      market_type: marketType,
      enable_replace: marketType == 'STOP_LOSS_ONLY' ? false : true,
      sub_market: this.tradingService.currentSubMarket,
      type: this.type,
      user_id: this.authService.userId$.getValue(),
    };
    this._updateAutoTradingHttp(request);
  }

  private _updateAutoTradingHttp(request: UpdateAutoTradeInfoRequest): void {
    this.isLoading = true;
    this.tradingService
      .updateAutoTradingInfo(request)
      .pipe(finalize(() => (this.isLoading = false)))
      .subscribe((res) => {
        if (request.is_running) {
          this.isGoClicked = request.is_running;
        }
      });
  }
}


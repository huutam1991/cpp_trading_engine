import {
  ChangeDetectorRef,
  Component,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
} from '@angular/core';
import { MatCheckboxChange } from '@angular/material/checkbox';
import { StorageKey } from '@common/enum';
import { NotificationService } from '@common/services/notification.service';
import {
  ArbitrageModel,
  ArbitrageOffsetModel,
  ConfigModel,
  TradeType,
} from '@core/models';
import { UpdateConfigRequest } from '@core/models/request';
import { isEqual, isNull } from 'lodash';
import { CookieService } from 'ngx-cookie-service';
import { TradingService } from 'src/app/pages/services/trading.service';

@Component({
  selector: 'app-testimonial-group',
  templateUrl: './testimonial-group.component.html',
  styleUrls: ['./testimonial-group.component.scss'],
})
export class TestimonialGroupComponent implements OnInit, OnChanges {
  // @Input() subMarketsubMarket[0]: string = '';
  // @Input() arbitrageData!: ArbitrageOffsetModel;

  @Input() arbitrageItem!: {
    subMarket: any;
    arbitrageModel: ArbitrageOffsetModel;
  };

  public data!: ArbitrageModel;
  public dataConfig!: ConfigModel;

  public checkedArmBid: boolean = false;
  public checkedArmAsk: boolean = false;
  public checkedArmHedge: boolean = false;

  public bidOffset: number | null = null;
  public askOffset: number | null = null;

  public askUpdateOffset: number | null = null;
  public bidUpdateOffset: number | null = null;

  public askOrderVolume: number | null = null;
  public bidOrderVolume: number | null = null;

  public isArming = false;

  constructor(
    private tradingService: TradingService,
    private cookieService: CookieService,
    private notificationService: NotificationService,
    private ref: ChangeDetectorRef
  ) {}

  ngOnChanges(changes: SimpleChanges): void {
    if (
      changes &&
      changes['arbitrageItem'] &&
      changes['arbitrageItem'].currentValue
    ) {
      this.tradingService.mmArbitrage$.subscribe((res) => {
        if (res) {
          const checkSubMarket =
            this.arbitrageItem.subMarket[0]
              .split('-')
              .includes(res.main.symbol) &&
            this.arbitrageItem.subMarket[0]
              .split('-')
              .includes(res.non_main.symbol) &&
            this.arbitrageItem.subMarket[0]
              .split('-')
              .includes(res.currency.symbol);
          if (checkSubMarket) {
            this.data = res;
            this._bindingData();
          }
        }
      });
    }
  }

  ngOnInit(): void {}

  onOffsetClick(type: TradeType): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
    };
    if (type === 'bid' && this.bidOffset && this.bidOffset > 0) {
      request.bid_offset = this.bidOffset;
      this._updateMmArbitrageHttp(request, type);
    }

    if (type === 'ask' && this.askOffset && this.askOffset > 0) {
      request.ask_offset = this.askOffset;
      this._updateMmArbitrageHttp(request, type);
    }
  }

  onUpdateOffsetBtnClick(type: TradeType): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
    };
    if (type === 'bid' && this.bidUpdateOffset && this.bidUpdateOffset > 0) {
      request.bid_update_offset = this.bidUpdateOffset;
      this._updateMmArbitrageHttp(request, type);
    }
    if (type === 'ask' && this.askUpdateOffset && this.askUpdateOffset > 0) {
      request.ask_update_offset = this.askUpdateOffset;
      this._updateMmArbitrageHttp(request, type);
    }
  }

  onOrderVolumeBtnClick(type: TradeType): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
    };
    if (type === 'bid' && this.bidOrderVolume && this.bidOrderVolume > 0) {
      request.bid_volumn = this.bidOrderVolume;
      this._updateMmArbitrageHttp(request, type);
    }

    if (type === 'ask' && this.askOrderVolume && this.askOrderVolume > 0) {
      request.ask_volumn = this.askOrderVolume;
      this._updateMmArbitrageHttp(request, type);
    }
  }

  onArmBidChecked(event: MatCheckboxChange): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
      arm_bid: event.checked,
    };
    this._updateMmArbitrageHttp(request);
  }

  onArmAskChecked(event: MatCheckboxChange): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
      arm_ask: event.checked,
    };
    this._updateMmArbitrageHttp(request);
  }

  onAutoHedgeChecked(event: MatCheckboxChange): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
      auto_hedge: event.checked,
    };
    this._updateMmArbitrageHttp(request);
  }

  onGoArbitrage(isGo: boolean): void {
    const request: UpdateConfigRequest = {
      sub_market: this.arbitrageItem.subMarket[0],
      is_arming: isGo,
    };
    this._updateMmArbitrageHttp(request);
  }

  private _bindingData(): void {
    if (
      isNull(this.askOffset) ||
      !isEqual(
        this.data.ask_offset,
        this.arbitrageItem.arbitrageModel.ask_offset
      )
    )
      this.askOffset = this.data.ask_offset;

    if (
      isNull(this.bidOffset) ||
      !isEqual(
        this.data.bid_offset,
        this.arbitrageItem.arbitrageModel.bid_offset
      )
    ) {
      this.bidOffset = this.data.bid_offset;
    }

    if (
      isNull(this.askUpdateOffset) ||
      !isEqual(
        this.data.ask_update_offset,
        this.arbitrageItem.arbitrageModel.ask_update_offset
      )
    )
      this.askUpdateOffset = this.data.ask_update_offset;
    if (
      isNull(this.bidUpdateOffset) ||
      !isEqual(
        this.data.bid_update_offset,
        this.arbitrageItem.arbitrageModel.bid_update_offset
      )
    )
      this.bidUpdateOffset = this.data.bid_update_offset;

    if (
      isNull(this.askOrderVolume) ||
      !isEqual(
        this.data.ask_order_volumn,
        this.arbitrageItem.arbitrageModel.ask_order_volumn
      )
    )
      this.askOrderVolume = this.data.ask_order_volumn;

    if (
      isNull(this.bidOrderVolume) ||
      !isEqual(
        this.data.bid_order_volumn,
        this.arbitrageItem.arbitrageModel.bid_order_volumn
      )
    )
      this.bidOrderVolume = this.data.bid_order_volumn;

    if (
      Object.keys(this.data.user_config).length !== 0 &&
      (!this.dataConfig ||
        !isEqual(
          this.data.user_config,
          this.arbitrageItem.arbitrageModel.user_config
        ))
    ) {
      this.dataConfig =
        this.data.user_config[this.cookieService.get(StorageKey.USER)];
      this.checkedArmBid = this.dataConfig.arm_bid;
      this.checkedArmAsk = this.dataConfig.arm_ask;
      this.checkedArmHedge = this.dataConfig.auto_hedge;
      this.isArming = this.dataConfig.is_arming;
    }
    this.ref.detectChanges();
  }

  private _updateMmArbitrageHttp(
    request: UpdateConfigRequest,
    type?: TradeType
  ): void {
    this.tradingService.mmArbitrageUpdateConfig(request).subscribe((res) => {
      if (res && !res.error) {
        if (type) {
          this.notificationService.updateOrderOffsetNotification(
            type,
            request.bid_offset ||
              request.ask_offset ||
              request.bid_update_offset ||
              request.ask_update_offset ||
              request.bid_volumn ||
              request.ask_volumn ||
              0
          );
        }
      }
    });
  }
}

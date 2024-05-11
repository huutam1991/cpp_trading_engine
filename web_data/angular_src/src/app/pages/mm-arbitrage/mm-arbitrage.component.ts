import {
  ChangeDetectorRef,
  Component,
  ElementRef,
  HostListener,
  OnDestroy,
  OnInit,
  QueryList,
  ViewChildren,
} from '@angular/core';
import {
  FormBuilder,
  FormControl,
  FormGroup,
  Validators,
} from '@angular/forms';
import { StorageKey } from '@common/enum';
import { NotificationService } from '@common/services/notification.service';
import {
  ArbitrageModel,
  ArbitrageOffsetModel,
  ConfigModel,
  ISymbolTickPriceModel,
} from '@core/models';
import { CookieService } from 'ngx-cookie-service';
import { Observable, Subject, map, of, startWith, takeUntil } from 'rxjs';
import { TradingService } from '../services/trading.service';
import { WebsocketService } from '../services/websocket.service';

@Component({
  selector: 'app-mm-arbitrage',
  templateUrl: './mm-arbitrage.component.html',
  styleUrls: ['./mm-arbitrage.component.scss'],
})
export class MmArbitrageComponent implements OnInit, OnDestroy {
  @ViewChildren('children') children!: QueryList<ElementRef>;
  public priceGroup: FormGroup = new FormGroup({});

  public baseSelection: Array<string> = [];
  public nonMarketOption$: Observable<Array<string>> = new Observable();
  public currencyOption$: Observable<Array<string>> = new Observable();
  public mainMarketOption$: Observable<Array<string>> = new Observable();

  public subMarket: any[] = [];
  public currentSubMarket: string = '';
  public dataModel!: ArbitrageModel;
  public offsetModel!: ArbitrageOffsetModel;
  public mmArbitrageList: any[] = [];
  public isLoading: boolean = false;
  public currentArbitrage: string = '';

  private destroyed$ = new Subject();

  constructor(
    private _formBuilder: FormBuilder,
    private tradingService: TradingService,
    private ws: WebsocketService,
    private cookieService: CookieService,
    private notificationService: NotificationService,
    private ref: ChangeDetectorRef
  ) {}

  ngOnInit(): void {
    this.isLoading = true;
    this._initFormGroup();
    if (this.tradingService.allSymbols.length === 0) {
      this.tradingService
        .getAllSymbols()
        .pipe(takeUntil(this.destroyed$))
        .subscribe((res) => {
          if (res) {
            this.baseSelection = res.data;
            this.nonMarketOption$ = this.tradingService.symbols$;
            this.currencyOption$ = this.tradingService.symbols$;
            this.mainMarketOption$ = this.tradingService.symbols$;
            this._subscribeValueChange();
          }
        });
    } else {
      this.baseSelection = this.tradingService.symbols$.getValue();
      this.nonMarketOption$ = this.tradingService.symbols$;
      this.currencyOption$ = this.tradingService.symbols$;
      this.mainMarketOption$ = this.tradingService.symbols$;
      this._subscribeValueChange();
    }

    this._subscribeWebSocket();
  }

  ngOnDestroy(): void {
    this.ws.closeForMmArbitrage();
    this.destroyed$.next(null);
    this.destroyed$.complete();
  }

  onScanMarket(isScan: boolean): void {
    if (isScan) {
      if (this.priceGroup.invalid) return;
      this.tradingService
        .mmArbitrageScan(this._getSubMarketLabel())
        .subscribe((res) => {
          if (res && !res.error) {
            this.notificationService.showScanMarketNotification(
              isScan,
              this._getSubMarketLabel()
            );
            this.priceGroup.reset();
          }
        });
    } else {
      this.tradingService
        .mmArbitrageStop(this.currentArbitrage)
        .subscribe((res) => {
          if (res && !res.error) {
            this.notificationService.showScanMarketNotification(
              isScan,
              this.currentArbitrage
            );
            this.priceGroup.reset();
            const index = this.mmArbitrageList.findIndex(
              (x) => x.label === this.currentArbitrage
            );
            this.mmArbitrageList.splice(index, 1);
          }
        });
    }
  }

  onArbitrageClick(item: any): void {
    this.currentArbitrage = item.subMarket[0];
  }

  isActive(item: any): boolean {
    if (!this.currentArbitrage) {
      this.currentArbitrage = this.mmArbitrageList[0].subMarket[0];
      return true;
    }

    if (this.currentArbitrage === item.subMarket[0]) return true;
    return false;
  }

  trackByArbitrage = (index: number, item: any) => item.subMarket[0];

  private _initFormGroup(): void {
    this.priceGroup = this._formBuilder.group({
      nonMarket: new FormControl(null, [Validators.required]),
      currency: new FormControl(null, [Validators.required]),
      mainMarket: new FormControl(null, [Validators.required]),
    });
  }

  private _subscribeWebSocket(): void {
    this.ws.connectForMmArbitrage();
    this.ws.mmArbitrageSubject$.subscribe((res) => {
      if (res && res.data) {
        this.dataModel = res.data as ArbitrageModel;
        const userConfig = this.dataModel.user_config[
          this.cookieService.get(StorageKey.USER)
        ] as ConfigModel;
        this.subMarket = this._getSubMarket(
          this.dataModel.currency,
          this.dataModel.non_main,
          this.dataModel.main,
          userConfig
        );
        this.offsetModel = new ArbitrageOffsetModel(res.data);
        this.currentSubMarket = this.subMarket[0];

        const newArbitrage = {
          subMarket: this.subMarket,
          arbitrageModel: this.dataModel,
        };
        const currentIndex = this.mmArbitrageList.findIndex(
          (x) => x.subMarket[0] === this.subMarket[0]
        );
        if (currentIndex < 0) {
          this.mmArbitrageList.push(newArbitrage);
        } else {
          this.mmArbitrageList[currentIndex] = newArbitrage;
        }
        this.tradingService.mmArbitrage = this.dataModel;
        this.ref.detectChanges();
        this.isLoading = false;
      }
    });

    this.ws.profitSubject$.subscribe((res) => {
      if (res) {
        console.log('profit');
        console.log(res);
      }
    });

    this.ws.mmArbitrageNotificationSubject$.subscribe((res) => {
      if (res) {
        console.log('notification');
        console.log(res);
      }
    });
  }

  private _subscribeValueChange(): void {
    this.nonMarketOption$ =
      this.priceGroup.get('nonMarket')?.valueChanges.pipe(
        startWith(''),
        map((value: string) => this._filter(value))
      ) || of([]);

    this.currencyOption$ =
      this.priceGroup.get('currency')?.valueChanges.pipe(
        startWith(''),
        map((value) => this._filter(value || ''))
      ) || of([]);

    this.mainMarketOption$ =
      this.priceGroup.get('mainMarket')?.valueChanges.pipe(
        startWith(''),
        map((value) => this._filter(value || ''))
      ) || of([]);
  }

  private _filter(value: string): string[] {
    const filterValue = value?.toLowerCase();
    if (this.baseSelection.length === 0) return [];

    return this.baseSelection.filter((option) =>
      option.toLowerCase().includes(filterValue)
    );
  }

  private _getSubMarket(
    currency: ISymbolTickPriceModel,
    nonMarket: ISymbolTickPriceModel,
    main: ISymbolTickPriceModel,
    userConfig: ConfigModel
  ): any[] {
    const data = [];
    const sub = `${main.symbol}-${currency.symbol}-${nonMarket.symbol}`;
    const model = {
      ask: {
        sub_market: sub.split('-').join('_'),
        expected_profit: userConfig?.placed_order.ask,
        is_running: true,
      },
      bid: {
        sub_market: sub.split('-').join('_'),
        expected_profit: userConfig?.placed_order.bid,
        is_running: true,
      },
    };

    data.push(sub);
    data.push(model);

    return data;
  }

  private _getSubMarketLabel = (): string =>
    `${this.priceGroup.get('mainMarket')?.value}-${
      this.priceGroup.get('currency')?.value
    }-${this.priceGroup.get('nonMarket')?.value}`;

  @HostListener('window:scroll', ['$event'])
  onScrollEvent(event: Event) {
    if ((event.target as any).scrollLeft) {
      this.children.forEach((item) => {
        (item.nativeElement as HTMLDivElement).scrollLeft = (
          event.target as any
        ).scrollLeft;
        if ((item.nativeElement as HTMLDivElement).scrollLeft < 50) {
          (item.nativeElement as HTMLDivElement).scrollLeft = 0;
        }

        if ((item.nativeElement as HTMLDivElement).scrollLeft > 580) {
          (item.nativeElement as HTMLDivElement).scrollLeft += 50;
        }
      });
    }
  }
}

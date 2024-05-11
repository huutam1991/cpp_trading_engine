import { ChangeDetectorRef, Component, OnInit } from '@angular/core';
import {
  FormBuilder,
  FormControl,
  FormGroup,
  Validators,
} from '@angular/forms';
import { SymbolTickPrice } from '@core/models';
import { Observable, combineLatest, map, of, startWith } from 'rxjs';
import { TradingService } from 'src/app/pages/services/trading.service';

@Component({
  selector: 'app-price-market',
  templateUrl: './price-market.component.html',
  styleUrls: ['./price-market.component.scss'],
})
export class PriceMarketComponent implements OnInit {
  public priceGroup: FormGroup = new FormGroup({});

  public baseSelection: Array<string> = [];

  public nonMarketOption: Array<string> = [];
  public currencyOption: Array<string> = [];
  public mainMarketOption: Array<string> = [];

  public nonMarketOption$: Observable<Array<string>> = new Observable();
  public currencyOption$: Observable<Array<string>> = new Observable();
  public mainMarketOption$: Observable<Array<string>> = new Observable();

  public currencyPrice = new SymbolTickPrice();
  public nonMarketPrice = new SymbolTickPrice();
  public mainMarketPrice = new SymbolTickPrice();
  public isLoading = false;

  constructor(
    private _formBuilder: FormBuilder,
    private tradingService: TradingService,
    private ref: ChangeDetectorRef
  ) {}

  ngOnInit(): void {
    this.isLoading = true;
    this._initFormGroup();
    this._initData();
    this.tradingService.currentSubMarket$.subscribe((res) => {
      if (res) {
        this._initFormGroup(res.split('-'));
        if (this.tradingService.allSymbols.length === 0) {
          this.tradingService.getAllSymbols().subscribe((result) => {
            this.nonMarketOption$ = this.tradingService.symbols$;
            this.currencyOption$ = this.tradingService.symbols$;
            this.mainMarketOption$ = this.tradingService.symbols$;

            const json = JSON.stringify(result);
            this.baseSelection = JSON.parse(json);
          });
        } else {
          const json = JSON.stringify(this.tradingService.allSymbols);
          this.baseSelection = JSON.parse(json);

          this.nonMarketOption$ = this.tradingService.symbols$;
          this.currencyOption$ = this.tradingService.symbols$;
          this.mainMarketOption$ = this.tradingService.symbols$;
        }

        this.nonMarketOption$ =
          this.priceGroup.get('nonMarket')!.valueChanges.pipe(
            startWith(this.priceGroup.get('nonMarket')?.value || ''),
            map((value: string) => this._filter(value))
          ) || of([]);

        this.currencyOption$ =
          this.priceGroup.get('currency')!.valueChanges.pipe(
            startWith(this.priceGroup.get('currency')?.value || ''),
            map((value) => this._filter(value || ''))
          ) || of([]);

        this.mainMarketOption$ =
          this.priceGroup.get('mainMarket')!.valueChanges.pipe(
            startWith(this.priceGroup.get('mainMarket')?.value || ''),
            map((value) => this._filter(value || ''))
          ) || of([]);

        this._subscriptionPriceMarket();
      }
    });
  }

  private _initFormGroup(subMarketValue?: Array<string>): void {
    this.priceGroup = this._formBuilder.group({
      nonMarket: new FormControl(subMarketValue ? subMarketValue[2] : null, [
        Validators.required,
      ]),
      currency: new FormControl(subMarketValue ? subMarketValue[1] : null, [
        Validators.required,
      ]),
      mainMarket: new FormControl(subMarketValue ? subMarketValue[0] : null, [
        Validators.required,
      ]),
    });
  }

  private _initData(): void {
    combineLatest([
      this.tradingService.symbols$,
      this.tradingService.currentSubMarket$,
    ]).subscribe((res) => {
      if (res && res[0] && res[1]) {
        this._initFormGroup(res[1].split('-'));
      }
    });
  }

  private _filter(value: string): string[] {
    const filterValue = value.toLowerCase();
    if (this.baseSelection.length === 0) return [];

    return this.baseSelection.filter((option) =>
      option.toLowerCase().includes(filterValue)
    );
  }

  private _subscriptionPriceMarket(): void {
    this.tradingService.priceMarket$.subscribe((res) => {
      if (res) {
        this.currencyPrice = new SymbolTickPrice(res.currency);
        this.nonMarketPrice = new SymbolTickPrice(res.non_main);
        this.mainMarketPrice = new SymbolTickPrice(res.main);
        this.isLoading = false;
        this.ref.detectChanges();
      }
    });
  }
}

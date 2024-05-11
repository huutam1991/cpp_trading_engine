import {
  ChangeDetectorRef,
  Component,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
} from '@angular/core';
import { AskBidModel } from '@core/models';
import { isEqual } from 'lodash';
import { TradingService } from 'src/app/pages/services/trading.service';

@Component({
  selector: 'app-sub-market',
  templateUrl: './sub-market.component.html',
  styleUrls: ['./sub-market.component.scss'],
})
export class SubMarketComponent implements OnInit, OnChanges {
  @Input() subMarket: any[] = [];
  @Input() isShowOrderVolume: boolean = true;
  @Input() orderVolume: number = 0;

  public object: Array<string> = [];

  get nonMarket(): string {
    return this.object.at(2) || '';
  }
  get currency(): string {
    return this.object.at(1) || '';
  }
  get mainMarket(): string {
    return this.object.at(0) || '';
  }

  // public orderVolume: number = 0;
  public model!: {
    ask: AskBidModel;
    bid: AskBidModel;
  };

  constructor(
    private tradingService: TradingService,
    private ref: ChangeDetectorRef
  ) {}

  ngOnChanges(changes: SimpleChanges): void {
    if (
      changes &&
      changes['subMarket'] &&
      changes['subMarket'].currentValue &&
      changes['subMarket'].currentValue.length > 0
    ) {
      this.object = this.subMarket[0].split('-');
      this.model = this.subMarket[1];
      this.ref.detectChanges();
    }
  }

  ngOnInit(): void {}
}

import { ChangeDetectorRef, Component, OnInit } from '@angular/core';
import { TheoreticalBidAskModel } from '@core/models';
import { TradingService } from 'src/app/pages/services/trading.service';

@Component({
  selector: 'app-theoretical-part',
  templateUrl: './theoretical-part.component.html',
  styleUrls: ['./theoretical-part.component.scss'],
})
export class TheoreticalPartComponent implements OnInit {
  public theoreticalBidAskData!: TheoreticalBidAskModel;
  public isLoading = false;
  constructor(
    private tradingService: TradingService,
    private ref: ChangeDetectorRef
  ) {}

  ngOnInit(): void {
    this.isLoading = true;
    this.tradingService.theoreticalBidAsk$.subscribe((res) => {
      if (res) {
        this.theoreticalBidAskData = res;
        this.ref.detectChanges();
        this.isLoading = false;
      }
    });
  }
}

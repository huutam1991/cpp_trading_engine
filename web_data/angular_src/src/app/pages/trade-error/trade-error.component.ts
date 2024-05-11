import { Component, OnInit } from '@angular/core';
import { OrderModel } from '@core/models';
import { Observable, Subject, of, takeUntil } from 'rxjs';
import { TRADE_ERROR_TABLE_CONFIG } from './trade-error.config';
import { NotificationService } from '@common/services/notification.service';
import { FormGroup, FormControl } from '@angular/forms';
import { ReportService } from '../services';

@Component({
  selector: 'app-trade-error',
  templateUrl: './trade-error.component.html',
  styleUrls: ['./trade-error.component.scss']
})
export class TradeErrorComponent implements OnInit {
  public tableConfig = TRADE_ERROR_TABLE_CONFIG;
  public dataSource: Observable<Array<OrderModel>> = new Observable();
  private currentDate = new Date();
  public rangeForm = new FormGroup({
    start: new FormControl(
      new Date(
        this.currentDate.getFullYear(),
        this.currentDate.getMonth(),
        this.currentDate.getDate(),
        0,
        0,
        0
      )
    ),
    end: new FormControl(
      new Date(
        this.currentDate.getFullYear(),
        this.currentDate.getMonth(),
        this.currentDate.getDate(),
        23,
        59,
        59
      )
    ),
  });

  public isLoading = false;
  private readonly destroyed$ = new Subject();

  constructor(
    private service: ReportService,
    private notificationService: NotificationService
  ) {}

  ngOnInit(): void {
    this._initDataGrid();
  }

  ngOnDestroy(): void {
    this.destroyed$.next(null);
    this.destroyed$.complete();
  }

  onSearchClick(): void {
    if (this.rangeForm.invalid) return;
    this._initDataGrid();
  }

  private _initDataGrid(): void {
    const from = this._getTimeFrom(this.rangeForm.value.start || new Date());
    const to = this._getTimeTo(this.rangeForm.value.end || new Date());
    this.service
      .getTradeErrors(from, to)
      .pipe(takeUntil(this.destroyed$))
      .subscribe((res) => {
        if (res && res.data) {
          this.dataSource = of(res.data);
        }
      });
  }

  private _getTimeFrom = (date: Date): number =>
    new Date(
      date.getFullYear(),
      date.getMonth(),
      date.getDate(),
      0,
      0,
      0
    ).getTime();

  private _getTimeTo = (date: Date): number =>
    new Date(
      date.getFullYear(),
      date.getMonth(),
      date.getDate(),
      23,
      59,
      59
    ).getTime();

}

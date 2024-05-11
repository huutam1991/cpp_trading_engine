import { Component, OnInit, OnDestroy } from '@angular/core';
import { OrderModel } from '@core/models';
import { Observable, Subject, finalize, of, takeLast, takeUntil } from 'rxjs';
import { ORDER_PER_DAY_TABLE_CONFIG } from './order-per-day.config';
import { WebsocketService } from '../services/websocket.service';
import { NotificationService } from '@common/services/notification.service';
import { FormGroup, FormControl } from '@angular/forms';
import { ReportService } from '../services';

@Component({
  selector: 'app-one-day-order',
  templateUrl: './one-day-order.component.html',
  styleUrls: ['./one-day-order.component.scss'],
})
export class OneDayOrderComponent implements OnInit, OnDestroy {
  public tableConfig = ORDER_PER_DAY_TABLE_CONFIG;
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
    private ws: WebsocketService,
    private notificationService: NotificationService
  ) {}

  ngOnInit(): void {
    this._initDataGrid();
    this.ws.connectForOrderPerDay();

    this.ws.orderStatus$
      .pipe(
        finalize(() => {
        })
      )
      .subscribe((res) => {
        if (res && res.data) {
          this.notificationService.showOrderUpdateNotification(
            res.data.orderId,
            res.data.side,
            res.data.symbol,
            res.data.status
          );
          this._initDataGrid();
        }
      });

    this.ws.profitSubject$.subscribe((res) => {
      if (res) {
      }
    });
  }

  ngOnDestroy(): void {
    this.destroyed$.next(null);
    this.destroyed$.complete();
    this.ws.closeForOrderPerDay();
  }

  onSearchClick(): void {
    if (this.rangeForm.invalid) return;
    this._initDataGrid();
  }

  private _initDataGrid(): void {
    const from = this._getTimeFrom(this.rangeForm.value.start || new Date());
    const to = this._getTimeTo(this.rangeForm.value.end || new Date());
    this.service
      .getOrders(from, to)
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

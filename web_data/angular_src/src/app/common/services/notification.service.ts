import { Injectable } from '@angular/core';
import { MatSnackBar, MatSnackBarConfig } from '@angular/material/snack-bar';
import { NotificationComponent } from '@common/components/notification/notification.component';
import { Toast, ToastType } from '@common/models';
import { SideType, TradeType } from '@core/models';
import { Observable, filter } from 'rxjs';
import { BehaviorSubject } from 'rxjs/internal/BehaviorSubject';

@Injectable({
  providedIn: 'root',
})
export class NotificationService {
  subject: BehaviorSubject<Toast | null>;
  toast$: Observable<Toast | null>;
  private delayMilliseconds = 1500;

  constructor() {
    this.subject = new BehaviorSubject<Toast | null>(null);
    this.toast$ = this.subject
      .asObservable()
      .pipe(filter((toast) => toast !== null));
  }

  show(
    type: ToastType,
    title: string,
    body: string,
    delay: number = this.delayMilliseconds
  ) {
    this.subject.next({ type, title, body, delay });
  }

  showUpdatedOrderVolumeNotification(
    value: number | string,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = 'Update order volume';
    const body = `<b class="blue-color">New volume</b>: <b>${value}</b>`;
    this.subject.next({ type, title, body, delay });
  }

  showScanMarketNotification(
    isNew: boolean,
    subMarket: string,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = isNew ? 'Start scanning market' : 'Stop scanning market';
    const body = `<b class="blue-color">Non-main market</b>: <b>${
      subMarket.split('-')[2]
    }</b> <br />
    <b class="blue-color">Currency</b>: <b>${subMarket.split('-')[1]}</b> <br />
    <b class="blue-color">Main market</b>: <b>${subMarket.split('-')[0]}</b>`;
    this.subject.next({ type, title, body, delay });
  }

  showOrderUpdateNotification(
    orderId: number,
    side: SideType,
    symbol: string,
    status: string,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = 'Order updated';

    let colorStatus = '';
    switch (status) {
      case 'FILLED':
        colorStatus = 'filled-color';
        break;
      case 'CANCELED':
        colorStatus = 'canceled-color';
        break;
      case 'PARTIALLY_FILLED':
        colorStatus = 'partially-filled-color';
        break;
      default:
        colorStatus = 'new-color';
        break;
    }

    const body = `
    <b class="blue-color">Order Id</b>: <b>${orderId}</b> <br />
    <b class="blue-color">Side</b>: <b class="${
      side === 'BUY' ? 'buy-color' : 'sell-color'
    }">${side}</b> <br />
    <b class="blue-color">Symbol</b>: <b>${symbol}</b> <br />
    <b class="blue-color">Status</b>: <b class="${colorStatus}">${status}</b>`;
    this.subject.next({ type, title, body, delay });
  }

  updateScanningMarketNotification(
    value: string,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = 'Scanning Market Notification';
    const body = `<b class="blue-color">New volume</b>: <b class="canceled-color">${value}</b>`;
    this.subject.next({ type, title, body, delay });
  }

  updateStopLossTickNotification(
    typeId: TradeType,
    currentMarketValue: number | string,
    mainMarketValue: number | string,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = 'Update Stop Loss tick';
    const body = `
    <b class="blue-color">Order Id</b>: <b class="uppercase">${typeId}</b> <br />
    <b class="blue-color">Currency Market</b>: <b class="filled-color">${currentMarketValue}</b> <br />
    <b class="blue-color">Main Market</b>: <b class="filled-color">${mainMarketValue}</b>`;
    this.subject.next({ type, title, body, delay });
  }

  updateOrderOffsetNotification(
    typeId: TradeType,
    value: number,
    delay: number = this.delayMilliseconds
  ): void {
    const type = 'info';
    const title = 'Update Order offset!';
    const body = `
    <b class="blue-color">Type</b>: <b class="uppercase">${typeId}</b> <br />
    <b class="blue-color">New value</b>: <b>${value}</b>`;
    this.subject.next({ type, title, body, delay });
  }
}

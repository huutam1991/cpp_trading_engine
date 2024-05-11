import { Component, OnInit } from '@angular/core';
import { IToast } from '@common/models';
import { NotificationService } from '@common/services/notification.service';

@Component({
  selector: 'app-toaster-container',
  templateUrl: './toaster-container.component.html',
  styleUrls: ['./toaster-container.component.scss'],
})
export class ToasterContainerComponent implements OnInit {
  toasts: IToast[] = [];

  constructor(private toaster: NotificationService) {}

  ngOnInit() {
    this.toaster.toast$.subscribe((toast) => {
      if (toast) {
        this.toasts = [toast, ...this.toasts];
        setTimeout(() => this.toasts.pop(), toast.delay || 5000);
      }
    });
  }

  remove(index: number) {
    this.toasts = this.toasts.filter((v, i) => i !== index);
  }
}

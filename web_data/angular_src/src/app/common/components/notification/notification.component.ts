import { Component, EventEmitter, Input, OnInit, Output } from '@angular/core';
import { Toast } from '@common/models';

@Component({
  selector: 'app-notification',
  templateUrl: './notification.component.html',
  styleUrls: ['./notification.component.scss'],
})
export class NotificationComponent implements OnInit {
  @Input() toast = new Toast('info');
  @Input() i: number = 0;

  @Output() remove = new EventEmitter<number>();

  constructor() {}

  ngOnInit() {}
}

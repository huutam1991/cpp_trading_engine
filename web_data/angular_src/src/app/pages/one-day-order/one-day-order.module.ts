import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { WebsocketService } from '../services/websocket.service';
import { OneDayOrderComponent } from './one-day-order.component';
import { ReportService } from '../services';

@NgModule({
  declarations: [OneDayOrderComponent],
  imports: [
    RouterModule.forChild([{ path: '', component: OneDayOrderComponent }]),

    CommonAppModule.forRoot(),
  ],
  providers: [ReportService, WebsocketService],
})
export class OneDayOrderModule {}

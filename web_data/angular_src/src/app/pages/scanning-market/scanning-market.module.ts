import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import {
  SuggestedOrderModule,
  PriceMarketModule,
  SubMarketModule,
  TheoreticalPartModule,
} from '@parts/index';
import { NgxMaskModule } from 'ngx-mask';
import { TradingService } from '../services/trading.service';
import { WebsocketService } from '../services/websocket.service';
import { ScanningMarketComponent } from './scanning-market.component';

@NgModule({
  declarations: [ScanningMarketComponent],
  imports: [
    RouterModule.forChild([
      {
        path: '',
        component: ScanningMarketComponent,
      },
    ]),
    CommonAppModule.forRoot(),
    NgxMaskModule.forRoot(),
    SubMarketModule,
    SuggestedOrderModule,
    PriceMarketModule,
    TheoreticalPartModule,
  ],
  providers: [TradingService, WebsocketService],
})
export class ScanningMarketModule {}

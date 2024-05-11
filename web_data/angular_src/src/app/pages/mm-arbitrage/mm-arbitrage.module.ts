import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { MmArbitrageComponent } from './mm-arbitrage.component';
import { TradingService } from '../services/trading.service';
import { WebsocketService } from '../services/websocket.service';
import { SubMarketModule, TestimonialGroupModule } from '@parts/index';

@NgModule({
  declarations: [MmArbitrageComponent],
  imports: [
    RouterModule.forChild([
      {
        path: '',
        component: MmArbitrageComponent,
      },
    ]),
    CommonAppModule.forRoot(),
    SubMarketModule,
    TestimonialGroupModule
  ],
  providers: [TradingService, WebsocketService],
})
export class MmArbitrageModule {}

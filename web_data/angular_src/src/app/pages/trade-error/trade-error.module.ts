import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { TradeErrorComponent } from './trade-error.component';
import { ReportService } from '../services';

@NgModule({
  declarations: [TradeErrorComponent],
  imports: [
    RouterModule.forChild([{ path: '', component: TradeErrorComponent }]),

    CommonAppModule.forRoot(),
  ],
  providers: [ReportService],
})
export class TradeErrorModule {}

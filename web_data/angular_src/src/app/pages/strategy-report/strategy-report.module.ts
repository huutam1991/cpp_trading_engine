import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { StrategyReportComponent } from './strategy-report.component';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { ReportService } from '../services/report.service';

@NgModule({
  declarations: [StrategyReportComponent],
  imports: [
    RouterModule.forChild([
      {
        path: '',
        component: StrategyReportComponent,
      },
    ]),
    CommonAppModule.forRoot(),
  ],
  providers: [ReportService],
})
export class StrategyReportModule {}

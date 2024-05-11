import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ExcelReportComponent } from './excel-report.component';
import { CommonAppModule } from '@common/common.module';
import { RouterModule } from '@angular/router';
import { ReportService } from '../services/report.service';

@NgModule({
  declarations: [ExcelReportComponent],
  imports: [
    RouterModule.forChild([
      {
        path: '',
        component: ExcelReportComponent,
      },
    ]),
    CommonAppModule.forRoot(),
  ],
  providers: [ReportService],
})
export class ExcelReportModule {}

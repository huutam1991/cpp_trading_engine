import { Component, OnInit, OnDestroy } from '@angular/core';
import { FormGroup, FormControl } from '@angular/forms';
import { takeUntil, finalize, Subject, Observable, of } from 'rxjs';
import { ReportService } from '../services/report.service';
import { EXCEL_REPORT_TABLE_CONFIG } from './excel-report.config';
import { ExcelReportModel } from '@core/models';

@Component({
  selector: 'app-excel-report',
  templateUrl: './excel-report.component.html',
  styleUrls: ['./excel-report.component.scss'],
})
export class ExcelReportComponent implements OnInit, OnDestroy {
  public tableConfig = EXCEL_REPORT_TABLE_CONFIG;
  public dataSource: Observable<Array<ExcelReportModel>> = new Observable();

  public isLoading = false;
  private readonly destroyed$ = new Subject();
  private currentDate = new Date();
  public rangeForm = new FormGroup({
    start: new FormControl(
      new Date(
        this.currentDate.getFullYear(),
        this.currentDate.getMonth(),
        this.currentDate.getDate() - 1,
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
  constructor(private reportService: ReportService) {}

  ngOnInit(): void {
    this._initDataGrid();
  }

  ngOnDestroy(): void {
    this.destroyed$.next(null);
    this.destroyed$.complete();
  }

  onSearchClick(): void {
    if (this.rangeForm.invalid) return;
    this._initDataGrid();
  }

  onExportClick(): void {
    const from = this._getTimeFrom(this.rangeForm.value.start || new Date());
    const to = this._getTimeTo(this.rangeForm.value.end || new Date());
    this.reportService.excelReportFileDownload(from, to).subscribe((file) => {
      if (file) {
        const blob = new Blob([file]);
        const downloadLink = document.createElement('a');
        downloadLink.href = window.URL.createObjectURL(blob);
        downloadLink.setAttribute(
          'download',
          `report_price_ticker_${new Date().getFullYear()}-${
            new Date().getMonth() + 1
          }-${new Date().getDate()}${new Date().getTime()}.xls`
        );
        document.body.appendChild(downloadLink);
        downloadLink.click();
      }
    });
  }

  private _initDataGrid(): void {
    this.isLoading = true;
    const from = this._getTimeFrom(this.rangeForm.value.start || new Date());
    const to = this._getTimeTo(this.rangeForm.value.end || new Date());
    this.reportService
      .excelReport(from, to)
      .pipe(
        takeUntil(this.destroyed$),
        finalize(() => (this.isLoading = false))
      )
      .subscribe((res) => {
        if (res && !res.error && res.data.length > 0) {
          this.dataSource = of(res.data);
        } else {
          this.dataSource = of([]);
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

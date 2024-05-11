import { Component, OnInit, OnDestroy } from '@angular/core';
import { ReportService } from '../services/report.service';
import { Observable, Subject, takeUntil, of, finalize } from 'rxjs';
import { STRATEGY_REPORT_TABLE_CONFIG } from './strategy-report.config';
import { StrategyModel } from '@core/models';
import { FormGroup, FormControl, Validators } from '@angular/forms';

@Component({
  selector: 'app-strategy-report',
  templateUrl: './strategy-report.component.html',
  styleUrls: ['./strategy-report.component.scss'],
})
export class StrategyReportComponent implements OnInit, OnDestroy {
  public tableConfig = STRATEGY_REPORT_TABLE_CONFIG;
  public dataSource: Observable<Array<StrategyModel>> = new Observable();

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
      ),
      [Validators.required]
    ),
    end: new FormControl(
      new Date(
        this.currentDate.getFullYear(),
        this.currentDate.getMonth(),
        this.currentDate.getDate(),
        23,
        59,
        59
      ),
      [Validators.required]
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
    if (this.rangeForm.invalid) return;

    const from = this._getTimeFrom(this.rangeForm.value.start || new Date());
    const to = this._getTimeTo(this.rangeForm.value.end || new Date());
    this.reportService
      .strategyReportFileDownload(from, to)
      .subscribe((file) => {
        if (file) {
          const blob = new Blob([file]);
          const downloadLink = document.createElement('a');
          downloadLink.href = window.URL.createObjectURL(blob);
          downloadLink.setAttribute(
            'download',
            `strategy_report_${new Date().getFullYear()}-${
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
      .strategyReport(from, to)
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

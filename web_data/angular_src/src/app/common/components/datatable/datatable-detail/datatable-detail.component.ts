import {
  Component,
  Input,
  OnChanges,
  OnInit,
  SimpleChanges,
} from '@angular/core';
import { IDisplayColumn, ITableConfig } from '@common/models';
import { DataTableUtils } from '@common/utils/utils';
import { Observable } from 'rxjs';

@Component({
  selector: 'app-datatable-detail',
  templateUrl: './datatable-detail.component.html',
  styleUrls: ['./datatable-detail.component.scss'],
})
export class DatatableDetailComponent implements OnInit, OnChanges {
  @Input() public tableConfigDetail: Array<IDisplayColumn> = [];
  @Input() public dataSourceDetail: Observable<any[]> = new Observable();

  public columnDefinition: Array<IDisplayColumn> = [];
  public displayColumns: Array<string> = [];
  public rowData = Array<object>();
  public utils = DataTableUtils;

  constructor() {}
  ngOnChanges(changes: SimpleChanges): void {
    if (
      changes['dataSourceDetail'] &&
      changes['dataSourceDetail'].currentValue
    ) {
      this.dataSourceDetail.subscribe((res) => {
        if (res && res.length > 0) {
          this.rowData = res;
        }
      });
    }

    if (
      changes['tableConfigDetail'] &&
      changes['tableConfigDetail'].currentValue
    ) {
      this.columnDefinition = this.tableConfigDetail;
      this._initData();
    }
  }

  ngOnInit(): void {
    this._initData();
  }

  public isNumber(format: any): boolean {
    return typeof format === 'number';
  }

  public isDateTime(format: any): boolean {
    return typeof format === 'string';
  }

  public pipeNumber(format: number | string): number {
    return typeof format === 'number' ? format : 0;
  }

  public pipeDateTime(format: number | string): string {
    return typeof format === 'string' ? format : '0';
  }

  private _initData(): void {
    this.displayColumns = this.columnDefinition.map((value) => value.id);
  }
}

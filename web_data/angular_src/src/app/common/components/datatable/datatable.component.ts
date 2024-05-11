import {
  animate,
  state,
  style,
  transition,
  trigger,
} from '@angular/animations';
import { SelectionModel } from '@angular/cdk/collections';
import {
  AfterViewChecked,
  AfterViewInit,
  ChangeDetectorRef,
  Component,
  EventEmitter,
  Input,
  NgZone,
  OnChanges,
  OnInit,
  Output,
  SimpleChanges,
  ViewChild,
} from '@angular/core';
import { MatPaginator, PageEvent } from '@angular/material/paginator';
import { MatTable } from '@angular/material/table';
import {
  ActionType,
  ButtonTableEvent,
  IDisplayColumn,
  ITableConfig,
} from '@common/models';
import { Observable, Subject, of, take } from 'rxjs';
import { DataTableUtils } from '../../utils/utils';

@Component({
  selector: 'app-datatable',
  templateUrl: './datatable.component.html',
  styleUrls: ['./datatable.component.scss'],
  animations: [
    trigger('detailExpand', [
      state('collapsed', style({ height: '0px', minHeight: '0' })),
      state('expanded', style({ height: '*' })),
      transition(
        'expanded <=> collapsed',
        animate('225ms cubic-bezier(0.4, 0.0, 0.2, 1)')
      ),
    ]),
  ],
})
export class DatatableComponent
  implements OnInit, OnChanges, AfterViewInit, AfterViewChecked
{
  // #region Decorator
  @Input() public tableConfig!: ITableConfig;
  @Input() public containerStyle: object = {
    width: '-webkit-fill-available',
  };
  @Input() public fullHeight: boolean = false;
  @Input() public dataSource: Observable<any> = new Observable();
  @Input() public isLoadingResults = false;

  @Output() public handleSort: EventEmitter<any> = new EventEmitter<any>();
  @Output() public handleSearch: EventEmitter<Event> =
    new EventEmitter<Event>();
  @Output() public pageSwitch: EventEmitter<PageEvent> =
    new EventEmitter<PageEvent>();
  @Output() public tableEvent: EventEmitter<ButtonTableEvent> =
    new EventEmitter<ButtonTableEvent>();
  @Output() public rowClick: EventEmitter<ButtonTableEvent> =
    new EventEmitter<ButtonTableEvent>();
  @Output() public viewDetailClick: EventEmitter<any> = new EventEmitter<any>();

  @ViewChild('matTable', { static: false }) public matTable!: MatTable<any>;
  @ViewChild(MatPaginator) paginator!: MatPaginator;
  // #endregion

  // public pageSizeOptions = environment.pagination.pageSizeOptions;
  public utils = DataTableUtils;
  public columnDefinition: Array<IDisplayColumn> = [];
  public detailColumnDefinition: Array<IDisplayColumn> = [];
  public displayColumns: Array<string> = [];
  public displayColumnsWithExpanded: Array<string> = [];
  public displayColumnsClone: Array<string> = [];

  public noScroll: boolean = false;
  public fixSecondColumnLeftPos = false;
  public data = [];
  public toolTipText: boolean = false;
  public selectedRow: any;
  public selectionModel: SelectionModel<any> = new SelectionModel<any>(
    true,
    []
  );

  public rowData = Array<object>();
  public rowDataDetail = Array<object>();
  public totalRecords: number = 0;
  public pageSize: number = 0;
  public pageNumber: number = 0;
  public expandedElement: any;
  public tableConfigDetail$: Array<IDisplayColumn> = [];
  public dataSourceDetail$: Observable<any[]> = new Observable();

  selectedRowIndex: any;

  searchInputControl: string = '';
  public isShowConfig = false;
  private readonly searchSubject = new Subject<Event | undefined>();

  public constructor(private cdr: ChangeDetectorRef, private ngZone: NgZone) {}

  public ngOnChanges(changes: SimpleChanges): void {
    if (changes['dataSource'] && changes['dataSource'].currentValue) {
      this.dataSource.subscribe((res) => {
        if (res) {
          this.rowData = res;
          // this.selectedRowIndex = null;
          // this.selectionModel = new SelectionModel<any>(true, []);
          this.totalRecords = res.length;
          // this.pageSize = res.pageSize;
          // this.pageNumber = res.pageNumber - 1;
        }
      });
    }

    if (changes['tableConfig'] && changes['tableConfig'].currentValue) {
      this.columnDefinition = this.tableConfig.columnDefinition;
      if (
        this.tableConfig.detailColumnDefinition &&
        this.tableConfig.detailColumnDefinition.length > 0
      ) {
        this.detailColumnDefinition = this.tableConfig.detailColumnDefinition;
      }
      this._initData();
    }
  }

  ngOnInit(): void {
    this._initData();
  }

  public ngAfterViewInit(): void {
    if (this.matTable) {
      this.matTable.updateStickyColumnStyles();
      this.ngZone.onMicrotaskEmpty
        .pipe(take(3))
        .subscribe(() => this.matTable.updateStickyColumnStyles());
    }
  }

  public ngAfterViewChecked(): void {
    this.cdr.detectChanges();
  }

  public isNumber(format: any): boolean {
    return typeof format === 'number';
  }

  public isDateTime(format: any): boolean {
    return typeof format === 'string';
  }

  public onBtnClick(event: Event, action: ActionType, item: object): void {
    event.stopPropagation();
    switch (action) {
      case 'deleted':
        this.tableEvent.emit({ action: 'deleted', rowItem: item });
        break;
      case 'view-permission':
        this.tableEvent.emit({ action: 'view-permission', rowItem: item });
        break;
      case 'enable':
        this.tableEvent.emit({ action: 'enable', rowItem: item });
        break;
      default:
        break;
    }
  }

  public onBtnExportClick(): void {
    this.tableEvent.emit({ action: 'export' });
  }
  public onBtnAddClick(): void {
    this.tableEvent.emit({ action: 'add' });
  }

  public onClickable(event: any): void {}

  public announceSortChange(event: any): void {}

  public toggleAllRows(): void {
    if (this.isAllSelected()) {
      this.selectionModel.clear();

      return;
    }

    // this.selectionModel.select(...this.dataSource.results);

    return;
  }

  public isAllSelected(): boolean {
    // const numSelected = this.selectionModel.selected.length;
    // const numRows = this.dataSource.results.length;

    // return numSelected === numRows;
    return false;
  }

  public isSelectedItem(row: object): boolean {
    if (this.selectionModel.isSelected(row)) {
      return true;
    }

    return false;
  }

  public pipeNumber(format: number | string): number {
    return typeof format === 'number' ? format : 0;
  }

  public pipeDateTime(format: number | string): string {
    return typeof format === 'string' ? format : '0';
  }

  public onMouseOver(event: any, data: string): void {
    if (event.target.className.split(' ').includes('cell-overflow')) {
      const selectElement = event.target;

      if (
        selectElement.offsetWidth === selectElement.scrollWidth ||
        Array.isArray(data)
      ) {
        this.toolTipText = true;
      } else {
        this.toolTipText = false;
      }
    }
  }

  public onExpandedRowClick(event: Event, element: any): void {
    event.stopPropagation();
    this.expandedElement = this.expandedElement === element ? null : element;
    this.dataSourceDetail$ = of(element.details);
  }

  public isLastChildNotSticky(index: number): string {
    const lastColumn = this.columnDefinition[index + 1];

    if (lastColumn && lastColumn.stickyEnd) {
      return `${
        this.columnDefinition[index] ? this.columnDefinition[index].weight : 0
      } px`;
    }

    return 'auto';
  }

  public styleObject(selectedRowIndex: number, id: number): object {
    if (selectedRowIndex === id) {
      return {
        'background-color': '#ffcc66! important;',
        'border-radius': '25px !important;',
      };
    }
    return {};
  }

  public onRowDblClick(event: Event, row: any, idx?: number): void {
    event.stopPropagation();
    this.selectedRowIndex = idx;
    this.rowClick.emit({ action: 'edit', rowItem: row });
    window.scrollTo(0, 0);
  }

  private _initData(): void {
    this.noScroll = this.tableConfig.noScroll || false;
    this.displayColumns = this.columnDefinition.map((value) => value.id);
    if (this.tableConfig.expandable) {
      this.displayColumnsWithExpanded = ['expand', ...this.displayColumns];
      if (this.tableConfig.detailColumnDefinition)
        this.tableConfigDetail$ = this.tableConfig.detailColumnDefinition;
    }
  }
}

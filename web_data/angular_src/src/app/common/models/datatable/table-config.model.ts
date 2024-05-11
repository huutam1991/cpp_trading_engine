import { FormGroup } from '@angular/forms';
import { IColumnData } from './column-data.model';
import { IDisplayColumn } from './display-column.model';

export interface IFilterBy {
  filterKey: string;
  condition: string | boolean;
}

export interface ITableConfig {
  columnDefinition: Array<IDisplayColumn>;
  pagination?: IColumnData;
  title?: string;
  btnExport?: boolean;
  btnAdd?: boolean;
  stickyHeader?: boolean;
  filterBy?: Array<IFilterBy>;
  expandable?: boolean;
  isDialog?: boolean;
  noScroll?: boolean;
  detailColumnDefinition?: Array<IDisplayColumn>;
}

export class TableConfig implements ITableConfig {
  public constructor(
    public columnDefinition: Array<IDisplayColumn>,
    public pagination: IColumnData,
    public title: string,
    public btnExport: boolean = false,
    public btnAdd: boolean = true,
    public stickyHeader?: boolean,
    public filterBy?: Array<IFilterBy>,
    public expandable?: boolean,
    public isDialog?: boolean,
    public noScroll?: boolean,
    public detailColumnDefinition?: Array<IDisplayColumn>
  ) {}
}

export declare type ButtonActionType =
  | 'add'
  | 'deleted'
  | 'edit'
  | 'export'
  | 'clicked-outside'
  | 'submit';

export declare type ActionSearchType = 'search' | 'reset' | 'save-filter';

export class ButtonActionModel {
  public action: ButtonActionType = 'edit';
  public rowItem?: object;
  public data?: object[];
}

export interface ActionSearchModel {
  action: ActionSearchType;
  form: any;
}

import { TooltipPosition } from '@angular/material/tooltip';

export declare type ActionType =
  | 'add'
  | 'lock'
  | 'unlock'
  | 'edit'
  | 'export'
  | 'view-permission'
  | 'cancel'
  | 'enable'
  | 'deleted'
  | 'update-rate'
  | 'advanced-search';

export interface IOption {
  id: ActionType;
  name: string;
  icon?: string;
  color?: string;
  isShow: boolean;
}

export interface IDisplayColumn {
  id: string;
  type: string;
  name: string;
  weight?: number;
  alignCenter?: boolean;
  alignRight?: boolean;
  showTooltip?: boolean;
  tooltipAlign?: TooltipPosition;
  className?: string;
  statusObject?: any;
  colorObject?: any;
  color?: string;
  options?: Array<IOption>;
  orderable?: boolean; // this can be either boolean or string id (for backward compatiable), to fix nested object compatibility;
  clickable?: boolean;
  disabled?: boolean;
  sticky?: boolean;
  stickyEnd?: boolean;
  localizedTextList?: string;
  format?: string | number;
  showSelectAll?: boolean;
  overflow?: boolean;
  checkNegative?: boolean;
}

export class TextColumn implements IDisplayColumn {
  public type = 'text';
  public constructor(
    public id: string,
    public name: string,
    public weight?: number,
    public showTooltip: boolean = false,
    public tooltipAlign?: TooltipPosition,
    public sticky?: boolean,
    public color?: string,
    public alignCenter: boolean = false,
    public alignRight: boolean = false,
    public className?: string,
    public stickyEnd?: boolean,
    public format?: string | number,
    public overflow: boolean = false,
    public orderable: boolean = true
  ) {}
}

export class SelectColumn implements IDisplayColumn {
  public type = 'select';
  public constructor(
    public id: string,
    public name: string,
    public weight?: number,
    public orderable: boolean = true,
    public showSelectAll: boolean = false,

    public alignCenter?: boolean,
    public alignRight?: boolean,
    public className?: string,

    public clickable?: boolean,
    public disabled?: boolean,
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class NumberColumn implements IDisplayColumn {
  public type = 'number';
  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public orderable: boolean = true,
    public alignCenter?: boolean,
    public alignRight?: boolean,
    public clickable?: boolean,
    public format?: number, // ex: 1, 2 will formatted 0.1, 0.11
    public checkNegative?: boolean,
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class MoneyColumn implements IDisplayColumn {
  public type = 'money';
  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public orderable: boolean = true,
    public alignCenter?: boolean,
    public alignRight?: boolean,
    public clickable?: boolean,
    public format?: number, // ex: 1, 2 will formatted 0.1, 0.11
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class StatusColumn implements IDisplayColumn {
  public type = 'status';
  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public colorObject?: any,
    public orderable?: boolean,
    public alignCenter?: boolean,
    public alignRight?: boolean,
    public clickable?: boolean,
    public sticky?: boolean,
    public stickyEnd?: boolean,
    public localizedTextList?: string,
    public isHtml?: boolean
  ) {}
}

export class ChipColumn implements IDisplayColumn {
  public type = 'chip';
  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public options?: Array<IOption>,
    public orderable?: boolean,
    public alignCenter?: boolean,
    public alignRight?: boolean,
    public clickable?: boolean,
    public sticky?: boolean,
    public stickyEnd?: boolean,
    public localizedTextList?: string,
    public isHtml?: boolean
  ) {}
}

export class OptionColumn implements IDisplayColumn {
  public type = 'options';
  public id = 'options';
  public name = 'Options';
  public alignRight = false;
  public showSelectAll = false;

  public constructor(
    public options: Array<IOption>,
    public weight: number,
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class OptionMenuColumn implements IDisplayColumn {
  public type = 'menu';
  public id = 'menu';
  public name = 'Options';
  public alignRight = false;
  public showSelectAll = false;

  public constructor(
    public options: Array<IOption>,
    public weight: number,
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class OptionButtonColumn implements IDisplayColumn {
  public type = 'button';
  public id = 'button';
  public name = 'Options';
  public alignRight = false;
  public showSelectAll = false;

  public constructor(
    public options: Array<IOption>,
    public weight: number,
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class CheckboxColumn implements IDisplayColumn {
  public type = 'selection';
  public id = 'selection';
  public name = 'common-names.select';

  public constructor(
    public weight: number,
    public showSelectAll: boolean,
    public sticky?: boolean,
    public alignCenter?: boolean
  ) {}
}

export class IndexColumn implements IDisplayColumn {
  public type = 'index';

  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public orderable?: boolean,
    public alignCenter?: boolean,
    public sticky?: boolean
  ) {}
}

export class DateTimeColumn implements IDisplayColumn {
  public type = 'date';

  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public orderable: boolean = true,
    public alignRight?: boolean,
    public clickable?: boolean,
    public format: string = 'YYYY/MM/DD',
    public sticky?: boolean,
    public stickyEnd?: boolean
  ) {}
}

export class TypeColumn implements IDisplayColumn {
  public type = 'type';
  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public colorObject: any,
    public alignCenter?: boolean,
    public sticky?: boolean,
    public alignRight?: boolean,
    public clickable?: boolean,
    public orderable?: boolean,
    public stickyEnd?: boolean,
    public localizedTextList?: string,
    public isHtml?: boolean
  ) {}
}

export class ImageColumn implements IDisplayColumn {
  public type = 'image';

  public constructor(
    public id: string,
    public name: string,
    public weight: number,
    public alignCenter?: boolean
  ) {}
}

export class ButtonTableEvent {
  constructor(
    public action: ActionType,
    public rowItem?: object,
    public data?: object[]
  ) {}
}

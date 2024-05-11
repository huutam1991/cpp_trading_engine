export interface IDropList<T> {
  value: T;
  label: string;
}

export class DropList<T> implements IDropList<T> {
  constructor(public value: T, public label: string) {}
}

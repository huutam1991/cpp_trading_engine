export type ToastType = 'success' | 'error' | 'warning' | 'info';

export interface IToast {
  type: ToastType;
  title: string;
  body: string;
  delay: number;
}
export class Toast implements IToast {
  type: ToastType;
  title: string;
  body: string;
  delay: number;
  constructor(
    type: ToastType,
    title: string = '',
    body: string = '',
    delay: number = 6000
  ) {
    this.type = type;
    this.title = title;
    this.body = body;
    this.delay = delay;
  }
}

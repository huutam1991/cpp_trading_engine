export interface IDialogInformation {
  type: string;
  title?: string;
  content?: string;
  param?: string;
  width?: number;
  positive?: {
    title: string;
    click(data?: object): void;
  };
  negative: {
    title: string;
    click(): void;
  };
}

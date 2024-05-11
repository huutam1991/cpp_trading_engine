export interface BaseResponseModel<T> {
  data: T;
  msg: string;
  status: number;
  error: boolean;
}

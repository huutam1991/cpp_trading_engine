import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ApiPath } from '@core/config';
import { LoginModelRequest, LoginModelResponse } from '@core/models';
import { BaseResponseModel } from '@core/models/base';
import { BehaviorSubject, Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class LoginService {
  public isLoginAsync$ = new BehaviorSubject<boolean>(false);

  public userId$ = new BehaviorSubject<string>('');

  public constructor(protected http: HttpClient) {}

  public userLogin(
    data: LoginModelRequest
  ): Observable<BaseResponseModel<LoginModelResponse>> {
    return this.http.post<BaseResponseModel<LoginModelResponse>>(
      ApiPath.LOGIN,
      data
    );
  }

  public getCurrentUser(): string {
    return this.userId$.getValue();
  }
}

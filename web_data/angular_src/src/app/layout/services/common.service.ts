import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ApiPath } from '@core/config';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class CommonService {
  public constructor(protected http: HttpClient) {}

  // public getCurrentUser(): Observable<UserModelResponse> {
  //   return this.http.get(ApiPath.LOGIN) as Observable<UserModelResponse>;
  // }

  // public changePassword(request: ChangePasswordRequest): Observable<any> {
  //   return this.http.post(ApiPath.CHANGE_PASSWORD, request) as Observable<any>;
  // }

  // public logout(): Observable<any> {
  //   return this.http.delete(ApiPath.LOGOUT) as Observable<any>;
  // }
}

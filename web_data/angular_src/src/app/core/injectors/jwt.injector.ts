import {
  HttpErrorResponse,
  HttpEvent,
  HttpHandler,
  HttpInterceptor,
  HttpRequest,
} from '@angular/common/http';
import { Injectable } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { LoginService } from '@auth/services/login.service';
import { StorageKey } from '@common/enum/storage-key.enum';
import { DialogConfirmService } from '@common/services/dialog-confirm.service';
import { BaseResponseModel } from '@core/models/base';
import { CookieService } from 'ngx-cookie-service';
import { Observable, throwError } from 'rxjs';
import { catchError } from 'rxjs/operators';

@Injectable()
export class JsonTokenWebInterceptor implements HttpInterceptor {
  private dialogConfirmRef: any;

  constructor(
    private dialogService: DialogConfirmService,
    private router: Router,
    private cookieService: CookieService,
    private loginService: LoginService
  ) {}

  public intercept(
    request: HttpRequest<any>,
    next: HttpHandler
  ): Observable<HttpEvent<BaseResponseModel<null>>> {
    const reqClone = this.addToHeader(request);

    return next.handle(reqClone).pipe(
      catchError((reason: HttpErrorResponse) => {
        if (this.dialogConfirmRef) return throwError(() => reason);

        if (
          reason.error !== undefined &&
          (reason.error === 'invalid_token' || reason.status === 401)
        ) {
          this.dialogConfirmRef = this.dialogService
            .customMessage(
              'error',
              'Your login session has expired. Please log in to continue working!'
            )
            .afterClosed()
            .subscribe((res) => {
              if (res) {
                this.loginService.isLoginAsync$.next(false);
                this.cookieService.delete(StorageKey.TOKEN);
                this.router.navigate(['auth'], {
                  queryParams: { returnUrl: this.router.url },
                });
                this.dialogConfirmRef = undefined;
              }
            });
          return throwError(() => reason);
        }

        if (reason.error) {
          this.dialogConfirmRef = this.dialogService
            .customMessage(
              'error',
              'Service(s) has been broken. Please contact to administrator.'
            )
            .afterClosed()
            .subscribe((res) => {
              if (res) {
                this.dialogConfirmRef = undefined;
              }
            });
          return throwError(() => reason);
        }

        return throwError(() => reason);
      })
    );
  }

  /**
   * Method to add the Authorization token in header. Returns the new request
   */
  private addToHeader(request: HttpRequest<any>): HttpRequest<any> {
    const token = this.cookieService.get(StorageKey.TOKEN);

    if (token) {
      request = request.clone({
        setHeaders: {
          Authorization: token,
        },
      });
    }

    return request;
  }
}

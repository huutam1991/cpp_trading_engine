import { Component, OnDestroy, OnInit } from '@angular/core';
import {
  FormBuilder,
  FormControl,
  FormGroup,
  Validators,
} from '@angular/forms';
import { Router } from '@angular/router';
import { LoginService } from '@auth/services/login.service';
import { StorageKey } from '@common/enum';
import { DialogConfirmService } from '@common/services/dialog-confirm.service';
import { NotificationService } from '@common/services/notification.service';
import { LoginModelRequest, LoginModelResponse } from '@core/models';
import { CookieService } from 'ngx-cookie-service';
import { Subject, finalize, takeUntil } from 'rxjs';

@Component({
  selector: 'app-login',
  templateUrl: './login.component.html',
  styleUrls: ['./login.component.scss'],
})
export class LoginComponent implements OnInit, OnDestroy {
  public showPassword: boolean = false;
  public loginForm: FormGroup = new FormGroup({});
  public user!: LoginModelResponse;
  public isLoading = false;

  private readonly destroyed$ = new Subject();

  public constructor(
    private fb: FormBuilder,
    private loginService: LoginService,
    private router: Router,
    private dialogService: DialogConfirmService,
    private notificationService: NotificationService,
    private cookieService: CookieService
  ) {}

  public ngOnInit(): void {
    this.initialForm();
  }

  ngOnDestroy(): void {
    this.destroyed$.next(null);
    this.destroyed$.complete();
  }

  public togglePassWord(): void {
    this.showPassword = !this.showPassword;
  }
  public submitForm(): void {
    const data: LoginModelRequest = this.loginForm.value;
    if (this.loginForm.invalid) {
      this.dialogService.customMessage(
        'error',
        'Please input your account and password!'
      );
      return;
    }
    this.isLoading = true;

    this.loginService
      .userLogin(data)
      .pipe(
        takeUntil(this.destroyed$),
        finalize(() => (this.isLoading = false))
      )
      .subscribe((res) => {
        if (res && res.data) {
          let date = new Date();
          const expiredTime = 1 * 24;
          date.setTime(date.getTime() + expiredTime * 60 * 60 * 1000);

          this.cookieService.set(StorageKey.TOKEN, res.data.token, date);
          this.cookieService.set(StorageKey.USER, res.data.user_id, date);
          this.loginService.isLoginAsync$.next(true);
          this.router.navigate(['']);
        }
      });
  }

  private initialForm(): void {
    this.loginForm = this.fb.group({
      username: new FormControl(null, [Validators.required]),
      password: new FormControl(null, [Validators.required]),
    });
  }
}

import { Component, OnInit } from '@angular/core';
import { FormBuilder, FormControl, FormGroup, Validators } from '@angular/forms';

@Component({
  selector: 'app-forgot-password',
  templateUrl: './forgot-password.component.html',
  styleUrls: ['./forgot-password.component.scss']
})
export class ForgotPasswordComponent implements OnInit {
  public showConfirmPassword: boolean = false;
  public showNewPassword: boolean = false;
  public invalidEmail: boolean = false;;
  public forgotPasswordByEmailForm: FormGroup = new FormGroup({});

  public constructor(
    private fb: FormBuilder
  ) { }

  public ngOnInit(): void {
    this.initialForm();
  }
  private initialForm(): void {
    this.forgotPasswordByEmailForm = this.fb.group({
      username: new FormControl(null, [Validators.required]),
      email: new FormControl(null, [Validators.required, Validators.email])
    });
  }
  public toggleNewPassWord(): void {
    this.showNewPassword = !this.showNewPassword;
  }
  public toggleConfirmPassWord(): void {
    this.showConfirmPassword = !this.showConfirmPassword;
  }

  public submitFormByEmail(): void { }
}

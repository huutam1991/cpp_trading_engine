import { Component, OnInit } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { ActivatedRoute, Router } from '@angular/router';
import { LoginService } from '@auth/services/login.service';
import { StorageKey } from '@common/enum';
import { DialogConfirmService } from '@common/services';
import { MENU_DATA } from '@core/config';
import { SidenavService } from '@layout/services';
import { CookieService } from 'ngx-cookie-service';

export interface DataHeader {
  parent: string;
  children: string;
}

@Component({
  selector: 'app-header',
  templateUrl: './header.component.html',
  styleUrls: ['./header.component.scss'],
})
export class HeaderComponent implements OnInit {
  public dataMenu = MENU_DATA;
  public titleHeader: string = '';
  public label: string = '';
  public userName: string = '';

  public constructor(
    private loginService: LoginService,
    private router: Router,
    private dialogConfirmService: DialogConfirmService,
    private cookieService: CookieService,
    private activatedRoute: ActivatedRoute,
    public sidenavService: SidenavService
  ) {}
  ngOnInit(): void {
    const date = new Date();
    if (date.getHours() < 12) {
      this.label = 'Morning!';
    } else if (date.getHours() >= 12 && date.getHours() <= 18) {
      this.label = 'Afternoon!';
    } else {
      this.label = 'Evening!';
    }
    this.loginService.userId$.subscribe((res) => {
      this.userName = res;
    });
  }

  changePassword(): void {
    // let dialog = this.dialog.open(ChangePasswordComponent, {
    //   width: '520px',
    // });
  }

  onMenuClick(route: string): void {
    this.router.navigate([route]);
  }

  activeRouter(route: string): boolean {
    return this.router.url.includes(route);
  }

  logout(): void {
    this.dialogConfirmService
      .confirmDialog('Do you want to logout?')
      .afterClosed()
      .subscribe((res) => {
        if (res) {
          this.cookieService.delete(StorageKey.TOKEN);
          this.loginService.isLoginAsync$.next(false);
          this.router.navigate(['auth/login']);
        }
      });
  }

  getPaddingSize(): string {
    if (
      this.sidenavService.showSidenav &&
      !this.sidenavService.showDetailSidenav
    ) {
      return '8rem';
    }

    if (
      this.sidenavService.showSidenav &&
      this.sidenavService.showDetailSidenav
    ) {
      return '11rem';
    }
    return '';
  }
}

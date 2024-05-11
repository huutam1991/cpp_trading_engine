import { Component, OnInit } from '@angular/core';
import { LoginService } from '@auth/services/login.service';
import { StorageKey } from '@common/enum';
import { IconService } from '@core/services/icon/icon.service';
import { SettingService, SidenavService } from '@layout/services';
import { CookieService } from 'ngx-cookie-service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss'],
})
export class AppComponent implements OnInit {
  constructor(
    private icon: IconService,
    public authService: LoginService,
    public sidenavService: SidenavService,
    private settingService: SettingService,
    private cookieService: CookieService
  ) {}

  ngOnInit(): void {
    this.icon.init();
    this.authService.isLoginAsync$.subscribe((res) => {
      if (res) {
        this._connectToBinanceSimulator();
        this._checkShowBackTestingMode();
      }
    });
  }

  onShowSidenav(): void {
    this.sidenavService.showDetailSidenav =
      !this.sidenavService.showDetailSidenav;
  }

  private _connectToBinanceSimulator(): void {
    this.settingService.connectToBinanceSimulator().subscribe((res) => {
      if (res && !res.error) {
        this.sidenavService.showSidenavOnSetting = res.data.is_connected;
        if (this.sidenavService.showSidenavOnSetting === false) {
          this.sidenavService.showSidenav = false;
        }
      }
    });
  }

  private _checkShowBackTestingMode(): void {
    this.settingService.getBackTestingMode().subscribe((res) => {
      if (res && !res.error) {
        this.sidenavService.showSidenav = res.data.back_testing_mode === 'on';
        if (this.sidenavService.showSidenavOnSetting === false) {
          this.sidenavService.showSidenav = false;
        }
      }
    });
  }
}

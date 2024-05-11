import { Component, OnInit } from '@angular/core';
import { SettingService, SidenavService } from '@layout/services';

@Component({
  selector: 'app-setting',
  templateUrl: './setting.component.html',
  styleUrls: ['./setting.component.scss'],
})
export class SettingComponent implements OnInit {
  public isLoading = false;
  public isSwitchedOn: boolean = false;

  constructor(
    private _httpService: SettingService,
    private sidenavService: SidenavService
  ) {}

  ngOnInit(): void {
    this.sidenavService.isShowSidenav$.subscribe((res) => {
      this.isSwitchedOn = res;
    });
  }

  onModeChanged(event: boolean): void {
    this._httpService
      .switchBackTestingMode(event ? 'on' : 'off')
      .subscribe((res) => {
        if (res && !res.error) {
          this.sidenavService.isShowSidenav$.next(event);
        }
      });
  }
}

import { Component, OnInit } from '@angular/core';
import {
  FormBuilder,
  FormControl,
  FormGroup,
  Validators,
} from '@angular/forms';
import { ConfigSidenavModel } from '@core/models';
import { BackTestingRequest } from '@core/models/request';
import { SettingService, SidenavService } from '@layout/services';

@Component({
  selector: 'app-sidenav',
  templateUrl: './sidenav.component.html',
  styleUrls: ['./sidenav.component.scss'],
})
export class SidenavComponent implements OnInit {
  public config!: ConfigSidenavModel;
  public dbLists: Array<string> = [];
  public supportSymbolList: Array<string> = [];
  public configGroup: FormGroup = new FormGroup({});
  public isStart: boolean = false;
  public isWaitingResposne: boolean = false;
  constructor(
    public sidenavService: SidenavService,
    private settingService: SettingService,
    private formBuilder: FormBuilder
  ) {}

  ngOnInit(): void {
    if (this.sidenavService.showSidenav) {
      this._getDataSidenav();
    }

    this.sidenavService.isShowDetailSidenav$.subscribe((res) => {
      if (res) {
        this._initGroup();
        this._getDbList();
      }
    });
  }

  onStartOrStopBackTesting(isStart: boolean): void {
    if (this.configGroup.invalid) return;
    const request: BackTestingRequest = this.configGroup.value;
    request.is_start = isStart;
    this.isWaitingResposne = true;
    this.settingService.setConfig(request).subscribe((res) => {
      if (res && !res.error) {
        this.config = res.data;
        this.isStart = res.data.is_start;
        this.isWaitingResposne = false;
        this._checkGetSymbolList();
      }
    });
  }

  onCleanBackTestingData(): void {
    this.isWaitingResposne = true;
    this.settingService.cleanBackTestingData().subscribe((res) => {
      if (res && !res.error) {
        this.isWaitingResposne = false;
      }
    });
  }

  private _getDataSidenav(): void {
    this.settingService.getConfig().subscribe((res) => {
      if (res && !res.error) {
        this.config = res.data;
        this.isStart = res.data.is_start;
        this._checkGetSymbolList();
      }
    });
  }

  private _checkGetSymbolList(): void {
    if (this.isStart == true) {
      this._getSymbolList(this.config.db_name);
    } else {
      this.supportSymbolList = [];
    }
  }

  private _getDbList(): void {
    this.settingService.getDbNames().subscribe((res) => {
      if (res && !res.error) {
        this.dbLists = res.data;
        this._initGroup();
      }
    });
  }

  private _getSymbolList(dbName: string): void {
    this.settingService.getSymbolNames(dbName).subscribe((res) => {
      if (res && !res.error) {
        this.supportSymbolList = res.data.sort();
      }
    });

  }

  private _initGroup(): void {
    this.configGroup = this.formBuilder.group({
      db_name: new FormControl(this.config ? this.config.db_name : null, [
        Validators.required,
      ]),
      speed_time: new FormControl(this.config ? this.config.speed_time : null, [
        Validators.required,
      ]),
    });
  }
}

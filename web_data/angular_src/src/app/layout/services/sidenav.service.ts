import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class SidenavService {
  constructor() {}

  public isShowSidenavOnSetting$: BehaviorSubject<boolean> =
    new BehaviorSubject<boolean>(false);

  get showSidenavOnSetting(): boolean {
    return this.isShowSidenavOnSetting$.getValue();
  }

  set showSidenavOnSetting(value: boolean) {
    this.isShowSidenavOnSetting$.next(value);
  }

  public isShowSidenav$: BehaviorSubject<boolean> =
    new BehaviorSubject<boolean>(false);

  get showSidenav(): boolean {
    return this.isShowSidenav$.getValue();
  }

  set showSidenav(value: boolean) {
    this.isShowSidenav$.next(value);
  }

  public isShowDetailSidenav$: BehaviorSubject<boolean> =
    new BehaviorSubject<boolean>(false);

  get showDetailSidenav(): boolean {
    return this.isShowDetailSidenav$.getValue();
  }

  set showDetailSidenav(value: boolean) {
    this.isShowDetailSidenav$.next(value);
  }
}

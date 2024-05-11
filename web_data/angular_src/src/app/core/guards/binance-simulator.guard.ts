import { Injectable } from '@angular/core';
import { CanLoad, Route, UrlSegment, UrlTree } from '@angular/router';
import { SidenavService } from '@layout/services';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class BinanceSimulatorGuard implements CanLoad {
  public constructor(private sidenavService: SidenavService) {}
  canLoad(
    route: Route,
    segments: UrlSegment[]
  ):
    | Observable<boolean | UrlTree>
    | Promise<boolean | UrlTree>
    | boolean
    | UrlTree {
    return this.sidenavService.showSidenavOnSetting;
  }
}

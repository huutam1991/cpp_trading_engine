import { Injectable } from '@angular/core';
import {
  ActivatedRouteSnapshot,
  CanActivate,
  CanActivateChild,
  Router,
  RouterStateSnapshot,
} from '@angular/router';
import { LoginService } from '@auth/services/login.service';
import { StorageKey } from '@common/enum/storage-key.enum';
import { CookieService } from 'ngx-cookie-service';

@Injectable({
  providedIn: 'root',
})
export class AuthGuard implements CanActivate, CanActivateChild {
  public constructor(
    private router: Router,
    private loginService: LoginService,
    private cookieService: CookieService
  ) {}

  public canActivate(
    route: ActivatedRouteSnapshot,
    state: RouterStateSnapshot
  ): boolean {
    const currentUser = this.cookieService.get(StorageKey.TOKEN);

    if (!currentUser) {
      this.loginService.isLoginAsync$.next(false);
      this.router.navigate(['auth']);

      return false;
    }
    this.loginService.isLoginAsync$.next(true);
    return true;
  }

  public canActivateChild(
    route: ActivatedRouteSnapshot,
    state: RouterStateSnapshot
  ): boolean {
    return this.canActivate(route, state);
  }
}

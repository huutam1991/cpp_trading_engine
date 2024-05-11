import { CommonModule } from '@angular/common';
import { HTTP_INTERCEPTORS } from '@angular/common/http';
import {
  ModuleWithProviders,
  NgModule,
  Optional,
  SkipSelf,
} from '@angular/core';
import { CommonAppModule } from '@common/common.module';
import { LoadingSpinnerDialogComponent } from '@layout/components/loading-spinner-dialog/loading-spinner-dialog.component';
import { AuthModule } from './../auth/auth.module';
import { AuthGuard } from './guards';
import { JsonTokenWebInterceptor } from './injectors';
import { IconService } from './services/icon/icon.service';
import { CookieService } from 'ngx-cookie-service';

@NgModule({
  imports: [CommonModule, AuthModule, CommonAppModule.forRoot()],
})
export class CoreModule {
  public constructor(@Optional() @SkipSelf() core: CoreModule) {
    if (core) {
      throw new Error('The CoreModule has been already created');
    }
  }

  public static forRoot(): ModuleWithProviders<CoreModule> {
    return {
      ngModule: CoreModule,
      providers: [
        AuthGuard,
        LoadingSpinnerDialogComponent,
        IconService,
        {
          provide: HTTP_INTERCEPTORS,
          useClass: JsonTokenWebInterceptor,
          multi: true,
        },
        CookieService,
      ],
    };
  }
}

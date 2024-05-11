import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { SettingComponent } from './setting.component';

import { UiSwitchModule } from 'ngx-ui-switch';
import { SettingService } from '@layout/services';

@NgModule({
  declarations: [SettingComponent],
  imports: [
    RouterModule.forChild([
      {
        path: '',
        component: SettingComponent,
      },
    ]),
    CommonAppModule.forRoot(),
    UiSwitchModule,
  ],
  providers: [SettingService],
})
export class SettingModule {}

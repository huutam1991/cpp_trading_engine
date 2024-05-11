import { NgModule } from '@angular/core';
import { CommonAppModule } from '@common/common.module';
import { SuggestedOrderComponent } from './suggested-order.component';
import { NgxMaskModule } from 'ngx-mask';

@NgModule({
  declarations: [SuggestedOrderComponent],
  imports: [CommonAppModule.forRoot(), NgxMaskModule.forRoot()],
  exports: [SuggestedOrderComponent],
})
export class SuggestedOrderModule {}

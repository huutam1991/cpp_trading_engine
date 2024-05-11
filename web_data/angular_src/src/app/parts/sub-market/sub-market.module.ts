import { NgModule } from '@angular/core';
import { CommonAppModule } from '@common/common.module';
import { SubMarketComponent } from './sub-market.component';

@NgModule({
  declarations: [SubMarketComponent],
  imports: [CommonAppModule.forRoot()],
  exports: [SubMarketComponent],
})
export class SubMarketModule {}

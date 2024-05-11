import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { PriceMarketComponent } from './price-market.component';
import { CommonAppModule } from '@common/common.module';

@NgModule({
  declarations: [PriceMarketComponent],
  imports: [CommonAppModule.forRoot()],
  exports: [PriceMarketComponent],
})
export class PriceMarketModule {}

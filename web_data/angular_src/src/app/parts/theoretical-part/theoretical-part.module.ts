import { NgModule } from '@angular/core';
import { CommonAppModule } from '@common/common.module';
import { TheoreticalPartComponent } from './theoretical-part.component';

@NgModule({
  declarations: [TheoreticalPartComponent],
  imports: [CommonAppModule.forRoot()],
  exports: [TheoreticalPartComponent],
})
export class TheoreticalPartModule {}

import { NgModule } from '@angular/core';
import { CommonAppModule } from '@common/common.module';
import { TestimonialGroupComponent } from './testimonial-group.component';

@NgModule({
  declarations: [TestimonialGroupComponent],
  imports: [CommonAppModule.forRoot()],
  exports: [TestimonialGroupComponent],
})
export class TestimonialGroupModule {}

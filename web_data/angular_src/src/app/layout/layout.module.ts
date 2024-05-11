import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';
import { AngularMaterialModule } from '@common/angular-material.module';
import { AuthModule } from './../auth/auth.module';
import { CommonService } from './services/common.service';

import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { RouterModule } from '@angular/router';
import { CommonAppModule } from '@common/common.module';
import { FooterComponent } from './components/footer/footer.component';
import { HeaderComponent } from './components/header/header.component';
import { LoadingSpinnerDialogComponent } from './components/loading-spinner-dialog/loading-spinner-dialog.component';
import { LoadingSpinnerDialogService } from './services';
import { SidenavComponent } from './components/sidenav/sidenav.component';
import { NgxMaskModule } from 'ngx-mask';

@NgModule({
  declarations: [
    HeaderComponent,
    FooterComponent,
    LoadingSpinnerDialogComponent,
    SidenavComponent,
  ],
  imports: [
    CommonModule,
    AngularMaterialModule,
    ReactiveFormsModule,
    FormsModule,
    RouterModule,
    CommonAppModule.forRoot(),
    AuthModule,
    NgxMaskModule.forRoot(),
  ],
  exports: [HeaderComponent, FooterComponent, FormsModule, SidenavComponent],
  providers: [CommonService, LoadingSpinnerDialogService],
})
export class LayoutModule {}

import { OverlayModule } from '@angular/cdk/overlay';
import { CommonModule } from '@angular/common';
import { ModuleWithProviders, NgModule } from '@angular/core';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { MAT_FORM_FIELD_DEFAULT_OPTIONS } from '@angular/material/form-field';
import { MAT_RADIO_DEFAULT_OPTIONS } from '@angular/material/radio';
import { AngularMaterialModule } from './angular-material.module';
import { DatatableComponent } from './components/datatable/datatable.component';
import { DialogInformationComponent } from './components/dialog-information/dialog-information.component';
import { NotificationComponent } from './components/notification/notification.component';
import { SpinnerComponent } from './components/spinner/spinner.component';
import {
  DateTimeUtc2LocalFormat,
  DateTimeUtcFormat,
  DateTimeformat2Pipe,
  DateTimeformat3Pipe,
  DateTimeformatPipe,
} from './pipe/date-time-format.pipe';
import {
  CurrencyFormatPipe,
  DecimalFormatPipe,
  TooltipListPipe,
} from './pipe/string-format.pipe';
import { DialogConfirmService } from './services/dialog-confirm.service';
import { NotificationService } from './services/notification.service';
import { StorageService } from './services/storage.service';
import { ToasterContainerComponent } from './components/toaster-container/toaster-container.component';

const CommonComponentsExport = [
  DatatableComponent,
  DialogInformationComponent,
  SpinnerComponent,
  NotificationComponent,
  ToasterContainerComponent,
];

const CustomPipes = [
  DateTimeformatPipe,
  DateTimeformat2Pipe,
  DateTimeformat3Pipe,
  DateTimeUtcFormat,
  DateTimeUtc2LocalFormat,
  TooltipListPipe,
  DecimalFormatPipe,
  CurrencyFormatPipe,
];

@NgModule({
  declarations: [...CommonComponentsExport, ...CustomPipes],
  imports: [
    CommonModule,
    ReactiveFormsModule,
    AngularMaterialModule,
    FormsModule,
    OverlayModule,
  ],
  exports: [
    ...CommonComponentsExport,
    ...CustomPipes,
    AngularMaterialModule,
    ReactiveFormsModule,
    CommonModule,
    FormsModule,
  ],
  bootstrap: [...CommonComponentsExport],
})
export class CommonAppModule {
  public static forRoot(): ModuleWithProviders<CommonAppModule> {
    return {
      ngModule: CommonAppModule,
      providers: [
        /* ALL SERVICES HERE! */
        {
          provide: MAT_FORM_FIELD_DEFAULT_OPTIONS,
          useValue: { floatLabel: 'always', appearance: 'outline' },
        },
        {
          provide: MAT_RADIO_DEFAULT_OPTIONS,
          useValue: { color: 'primary' },
        },
        DialogConfirmService,
        StorageService,
        NotificationService,
      ],
    };
  }
}

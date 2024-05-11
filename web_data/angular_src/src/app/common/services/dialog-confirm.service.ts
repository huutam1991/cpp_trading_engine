import { Injectable } from '@angular/core';
import {
  MatDialog,
  MatDialogConfig,
  MatDialogRef,
} from '@angular/material/dialog';
import { DialogInformationComponent } from '@common/components/dialog-information/dialog-information.component';
import { IDialogInformation } from '@common/models/dialog/dialog-information.model';

@Injectable({
  providedIn: 'root',
})
export class DialogConfirmService {
  public dialogConfirmRef!: MatDialogRef<DialogInformationComponent>;
  public constructor(private matDialog: MatDialog) {}

  public confirmDialog(
    messages: string,
    param?: string
  ): MatDialogRef<DialogInformationComponent> {
    const dialogData: IDialogInformation = {
      type: 'info',
      content: messages,
      param: param,
      positive: {
        title: 'Yes',
        click: () => {
          dialogConfirmRef.close(true);
        },
      },
      negative: {
        title: 'NO',
        click: () => {
          dialogConfirmRef.close(false);
        },
      },
    };
    const dialogConfig = new MatDialogConfig();

    dialogConfig.autoFocus = true;
    dialogConfig.data = dialogData;
    dialogConfig.disableClose = true;
    const dialogConfirmRef: MatDialogRef<DialogInformationComponent> =
      this.matDialog.open(DialogInformationComponent, dialogConfig);

    return dialogConfirmRef;
  }

  public successRegisterMessage(message: string, param?: string): void {
    const dialogData: IDialogInformation = {
      type: 'success',
      content: message,
      param: param,
      negative: {
        title: 'OK',
        click: () => {
          dialogConfirmRef.close();
        },
      },
    };
    const dialogConfig = new MatDialogConfig();

    dialogConfig.autoFocus = true;
    dialogConfig.data = dialogData;
    dialogConfig.disableClose = false;
    const dialogConfirmRef: MatDialogRef<DialogInformationComponent> =
      this.matDialog.open(DialogInformationComponent, dialogConfig);
  }

  public errorRegisterMessage(message: string): void {
    const dialogData: IDialogInformation = {
      type: 'error',
      content: 'common.message.error',
      negative: {
        title: 'OK',
        click: () => {
          dialogConfirmRef.close();
        },
      },
    };
    const dialogConfig = new MatDialogConfig();

    dialogConfig.autoFocus = true;
    dialogConfig.data = dialogData;
    dialogConfig.disableClose = true;
    const dialogConfirmRef: MatDialogRef<DialogInformationComponent> =
      this.matDialog.open(DialogInformationComponent, dialogConfig);
  }

  public customMessage(
    type: 'error' | 'success',
    message: string
  ): MatDialogRef<DialogInformationComponent> {
    const dialogData: IDialogInformation = {
      type: type,
      content: message,
      negative: {
        title: 'OK',
        click: () => {
          this.dialogConfirmRef.close(true);
        },
      },
    };
    const dialogConfig = new MatDialogConfig();

    dialogConfig.autoFocus = true;
    dialogConfig.data = dialogData;
    dialogConfig.disableClose = true;
    this.dialogConfirmRef = this.matDialog.open(
      DialogInformationComponent,
      dialogConfig
    );

    return this.dialogConfirmRef;
  }
}

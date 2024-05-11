import { Injectable } from '@angular/core';
import { MatDialogRef, MatDialog, MatDialogConfig } from '@angular/material/dialog';
import { LoadingSpinnerDialogComponent } from '@layout/components/loading-spinner-dialog/loading-spinner-dialog.component';
import { each, isEqual } from 'lodash';

@Injectable({
  providedIn: 'root'
})
export class LoadingSpinnerDialogService {
  private spinnerDialogId = 'globalSpinLoader';
  private spinnerRef: MatDialogRef<LoadingSpinnerDialogComponent> | undefined;

  public constructor(private matDialog: MatDialog) { }

  public getSpinnerRef(): MatDialogRef<LoadingSpinnerDialogComponent> | undefined {
    return this.spinnerRef;
  }

  public showSpinner(shudShow: boolean): void {
    if (shudShow) {
      const dialogConfig = new MatDialogConfig();

      dialogConfig.id = this.spinnerDialogId;
      dialogConfig.disableClose = true;

      setTimeout(() => {
        if (this.spinnerIsOpen()) {
          return;
        }

        this.spinnerRef = this.matDialog.open(LoadingSpinnerDialogComponent, dialogConfig);
      }, 100);
    } else {
      // fix timing issue when show and hide too fast
      setTimeout(() => {
        if (this.spinnerRef && this.spinnerIsOpen()) {
          this.spinnerRef.close();
        }
      }, 200);
    }
  }

  private spinnerIsOpen(): boolean {
    let isOpen = false;

    each(this.matDialog.openDialogs, (val: MatDialogRef<undefined>) => {
      if (isEqual(val.id, this.spinnerDialogId)) {
        isOpen = true;

        return false;
      } else {
        return true;
      }
    });

    return isOpen;
  }
}

import { Component, Inject } from '@angular/core';
import { MAT_DIALOG_DATA, MatDialogRef } from '@angular/material/dialog';

@Component({
  selector: 'app-loading-spinner-dialog',
  templateUrl: './loading-spinner-dialog.component.html',
  styleUrls: ['./loading-spinner-dialog.component.scss'],
})
export class LoadingSpinnerDialogComponent {
  public constructor(
    public dialogRef: MatDialogRef<LoadingSpinnerDialogComponent>,
    @Inject(MAT_DIALOG_DATA) public data: undefined
  ) {}
}

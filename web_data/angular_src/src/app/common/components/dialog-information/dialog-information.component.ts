import { Component, Inject, OnInit } from '@angular/core';
import { MatDialogRef, MAT_DIALOG_DATA } from '@angular/material/dialog';
import { IDialogInformation } from '@common/models/dialog/dialog-information.model';

@Component({
  selector: 'app-dialog-information',
  templateUrl: './dialog-information.component.html',
  styleUrls: ['./dialog-information.component.scss']
})
export class DialogInformationComponent implements OnInit {
  public icon: string = '';
  public color: string = '';
  public value: string = '';

  public constructor(
    public dialogRef: MatDialogRef<DialogInformationComponent>,
    @Inject(MAT_DIALOG_DATA) public data: IDialogInformation
  ) { }

  public ngOnInit(): void {
    if (this.data) {
      this.icon = this.getIcon(this.data.type);
      this.color = this.getColor(this.data.type);
      this.value = this.data.param as string;
    }
  }

  public dissmiss(): void {
    this.dialogRef.close();
  }

  public getIcon(type: string): string {
    switch (type) {
      case 'success':
        return 'icon-success';

      case 'info':
        return 'icon-notification';

      case 'error':
        return 'icon-error';

      default:
        return 'icon-notification';
    }
  }

  public getColor(type: string): string {
    switch (type) {
      case 'success':
        return 'mat-success';

      case 'info':
        return 'mat-info';

      case 'error':
        return 'mat-error';

      default:
        return 'mat-info';
    }
  }
}

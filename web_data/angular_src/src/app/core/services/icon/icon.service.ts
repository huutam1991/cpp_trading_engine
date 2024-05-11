import { isPlatformServer } from '@angular/common';
import { Inject, Injectable, PLATFORM_ID } from '@angular/core';
import { MatIconRegistry } from '@angular/material/icon';
import { DomSanitizer } from '@angular/platform-browser';

@Injectable({
  providedIn: 'root',
})
export class IconService {
  public constructor(
    private iconRegistry: MatIconRegistry,
    private sanitizer: DomSanitizer,
    @Inject(PLATFORM_ID) private platformId: string
  ) {}

  public init(): void {
    const domain = isPlatformServer(this.platformId)
      ? 'http://localhost:5500/'
      : '';
    this.iconRegistry.addSvgIcon(
      'icon-close-dialog',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/close-dialog.svg'
      )
    );
    this.iconRegistry.addSvgIcon(
      'icon-error',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/icon-error.svg'
      )
    );
    this.iconRegistry.addSvgIcon(
      'icon-notification',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/icon-notification.svg'
      )
    );
    this.iconRegistry.addSvgIcon(
      'icon-success',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/icon-success.svg'
      )
    );
    this.iconRegistry.addSvgIcon(
      'icon-export',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/export.svg'
      )
    );

    this.iconRegistry.addSvgIcon(
      'database',
      this.sanitizer.bypassSecurityTrustResourceUrl(
        domain + '/assets/icon-svg/database.svg'
      )
    );
  }
}

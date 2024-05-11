import { AfterViewInit, Component, ElementRef, Input, OnInit, Renderer2 } from '@angular/core';

@Component({
  selector: 'app-spinner',
  templateUrl: './spinner.component.html',
  styleUrls: ['./spinner.component.scss']
})
export class SpinnerComponent  implements AfterViewInit {

  @Input() loading: boolean = false;
  @Input() fullHeight: boolean = true;
  @Input() rounded: boolean = true;
  constructor(private renderer: Renderer2, private elementRef: ElementRef) { }

  ngAfterViewInit(): void {
    if (this.fullHeight) {
      this.renderer.setStyle(this.elementRef.nativeElement, 'height', '100%');
    }
  }
}

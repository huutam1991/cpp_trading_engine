import { ComponentFixture, TestBed } from '@angular/core/testing';

import { ScanningMarketComponent } from './scanning-market.component';

describe('ScanningMarketComponent', () => {
  let component: ScanningMarketComponent;
  let fixture: ComponentFixture<ScanningMarketComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ ScanningMarketComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(ScanningMarketComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

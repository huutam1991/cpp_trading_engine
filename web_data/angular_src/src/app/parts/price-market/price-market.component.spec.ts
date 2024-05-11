import { ComponentFixture, TestBed } from '@angular/core/testing';

import { PriceMarketComponent } from './price-market.component';

describe('PriceMarketComponent', () => {
  let component: PriceMarketComponent;
  let fixture: ComponentFixture<PriceMarketComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ PriceMarketComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(PriceMarketComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

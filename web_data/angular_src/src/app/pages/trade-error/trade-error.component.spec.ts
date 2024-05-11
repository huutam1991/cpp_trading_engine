import { ComponentFixture, TestBed } from '@angular/core/testing';

import { TradeErrorComponent } from './trade-error.component';

describe('TradeErrorComponent', () => {
  let component: TradeErrorComponent;
  let fixture: ComponentFixture<TradeErrorComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ TradeErrorComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(TradeErrorComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

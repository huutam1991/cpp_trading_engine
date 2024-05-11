import { ComponentFixture, TestBed } from '@angular/core/testing';

import { OneDayOrderComponent } from './one-day-order.component';

describe('OneDayOrderComponent', () => {
  let component: OneDayOrderComponent;
  let fixture: ComponentFixture<OneDayOrderComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ OneDayOrderComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(OneDayOrderComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

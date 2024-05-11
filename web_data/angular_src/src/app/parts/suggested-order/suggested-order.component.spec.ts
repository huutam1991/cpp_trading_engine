import { ComponentFixture, TestBed } from '@angular/core/testing';

import { SuggestedOrderComponent } from './suggested-order.component';

describe('SuggestedOrderComponent', () => {
  let component: SuggestedOrderComponent;
  let fixture: ComponentFixture<SuggestedOrderComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ SuggestedOrderComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(SuggestedOrderComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

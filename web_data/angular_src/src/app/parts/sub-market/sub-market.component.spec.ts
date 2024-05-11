import { ComponentFixture, TestBed } from '@angular/core/testing';

import { SubMarketComponent } from './sub-market.component';

describe('SubMarketComponent', () => {
  let component: SubMarketComponent;
  let fixture: ComponentFixture<SubMarketComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ SubMarketComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(SubMarketComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

import { ComponentFixture, TestBed } from '@angular/core/testing';

import { MmArbitrageComponent } from './mm-arbitrage.component';

describe('MmArbitrageComponent', () => {
  let component: MmArbitrageComponent;
  let fixture: ComponentFixture<MmArbitrageComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ MmArbitrageComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(MmArbitrageComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

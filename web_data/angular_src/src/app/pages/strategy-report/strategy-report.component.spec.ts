import { ComponentFixture, TestBed } from '@angular/core/testing';

import { StrategyReportComponent } from './strategy-report.component';

describe('StrategyReportComponent', () => {
  let component: StrategyReportComponent;
  let fixture: ComponentFixture<StrategyReportComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ StrategyReportComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(StrategyReportComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

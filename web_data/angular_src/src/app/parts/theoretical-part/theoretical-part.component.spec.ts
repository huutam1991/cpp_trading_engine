import { ComponentFixture, TestBed } from '@angular/core/testing';

import { TheoreticalPartComponent } from './theoretical-part.component';

describe('TheoreticalPartComponent', () => {
  let component: TheoreticalPartComponent;
  let fixture: ComponentFixture<TheoreticalPartComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ TheoreticalPartComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(TheoreticalPartComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

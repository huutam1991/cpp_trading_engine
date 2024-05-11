import { ComponentFixture, TestBed } from '@angular/core/testing';

import { TestimonialGroupComponent } from './testimonial-group.component';

describe('TestimonialGroupComponent', () => {
  let component: TestimonialGroupComponent;
  let fixture: ComponentFixture<TestimonialGroupComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ TestimonialGroupComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(TestimonialGroupComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

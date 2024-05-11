import { ComponentFixture, TestBed } from '@angular/core/testing';

import { ToasterContainerComponent } from './toaster-container.component';

describe('ToasterContainerComponent', () => {
  let component: ToasterContainerComponent;
  let fixture: ComponentFixture<ToasterContainerComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ ToasterContainerComponent ]
    })
    .compileComponents();

    fixture = TestBed.createComponent(ToasterContainerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});

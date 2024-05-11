import { TestBed } from '@angular/core/testing';

import { LoadingSpinnerDialogService } from './loading-spinner-dialog.service';

describe('LoadingSpinnerDialogService', () => {
  let service: LoadingSpinnerDialogService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(LoadingSpinnerDialogService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});

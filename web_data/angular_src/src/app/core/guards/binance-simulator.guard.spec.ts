import { TestBed } from '@angular/core/testing';

import { BinanceSimulatorGuard } from './binance-simulator.guard';

describe('BinanceSimulatorGuard', () => {
  let guard: BinanceSimulatorGuard;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    guard = TestBed.inject(BinanceSimulatorGuard);
  });

  it('should be created', () => {
    expect(guard).toBeTruthy();
  });
});

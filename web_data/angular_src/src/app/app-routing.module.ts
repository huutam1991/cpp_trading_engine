import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { CONFIG_ROUTING } from '@core/config';
import { AuthGuard, BinanceSimulatorGuard } from '@core/guards';

const routes: Routes = [
  { path: '', pathMatch: 'full', redirectTo: CONFIG_ROUTING.scanMarket.path },
  {
    path: 'auth',
    loadChildren: () => import('./auth/auth.module').then((m) => m.AuthModule),
    title: 'Login',
  },
  {
    path: CONFIG_ROUTING.orderPerDay.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/one-day-order/one-day-order.module').then(
        (m) => m.OneDayOrderModule
      ),
    title: CONFIG_ROUTING.orderPerDay.title,
  },
  {
    path: CONFIG_ROUTING.scanMarket.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/scanning-market/scanning-market.module').then(
        (m) => m.ScanningMarketModule
      ),
    title: CONFIG_ROUTING.scanMarket.title,
  },
  {
    path: CONFIG_ROUTING.tradeError.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/trade-error/trade-error.module').then(
        (m) => m.TradeErrorModule
      ),
    title: CONFIG_ROUTING.tradeError.title,
  },
  {
    path: CONFIG_ROUTING.excelReport.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/excel-report/excel-report.module').then(
        (m) => m.ExcelReportModule
      ),
    title: CONFIG_ROUTING.excelReport.title,
  },

  {
    path: CONFIG_ROUTING.strategyReport.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/strategy-report/strategy-report.module').then(
        (m) => m.StrategyReportModule
      ),
    title: CONFIG_ROUTING.strategyReport.title,
  },

  {
    path: CONFIG_ROUTING.mmArbitrage.path,
    canActivate: [AuthGuard],
    loadChildren: () =>
      import('./pages/mm-arbitrage/mm-arbitrage.module').then(
        (m) => m.MmArbitrageModule
      ),
    title: CONFIG_ROUTING.mmArbitrage.title,
  },

  {
    path: CONFIG_ROUTING.setting.path,
    canActivate: [AuthGuard],
    canLoad: [BinanceSimulatorGuard],
    loadChildren: () =>
      import('./pages/setting/setting.module').then((m) => m.SettingModule),
    title: CONFIG_ROUTING.setting.title,
  },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule],
})
export class AppRoutingModule {}

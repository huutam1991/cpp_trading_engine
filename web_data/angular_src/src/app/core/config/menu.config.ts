import { MenuModel } from '@core/models/common';

export const CONFIG_ROUTING = {
  scanMarket: {
    title: 'Scanning Market',
    path: 'scanning-market',
  },
  mmArbitrage: {
    title: 'MM Arbitrage',
    path: 'mm-arbitrage',
  },
  orderPerDay: {
    title: '24h Orders',
    path: '24h-orders',
  },
  tradeError: {
    title: 'Trade Errors',
    path: 'trade-errors',
  },
  excelReport: {
    title: 'Excel Report',
    path: 'excel-report',
  },
  strategyReport: {
    title: 'Strategy Report',
    path: 'strategy-report',
  },
  setting: {
    title: 'Settings',
    path: 'setting',
  },
};

export const MENU_DATA: Array<MenuModel> = [
  {
    order: 0,
    label: CONFIG_ROUTING.scanMarket.title,
    route: CONFIG_ROUTING.scanMarket.path,
  },
  {
    order: 1,
    label: CONFIG_ROUTING.mmArbitrage.title,
    route: CONFIG_ROUTING.mmArbitrage.path,
  },
  {
    order: 2,
    label: CONFIG_ROUTING.orderPerDay.title,
    route: CONFIG_ROUTING.orderPerDay.path,
  },
  {
    order: 3,
    label: CONFIG_ROUTING.tradeError.title,
    route: CONFIG_ROUTING.tradeError.path,
  },
  {
    order: 4,
    label: CONFIG_ROUTING.excelReport.title,
    route: CONFIG_ROUTING.excelReport.path,
  },
  {
    order: 5,
    label: CONFIG_ROUTING.strategyReport.title,
    route: CONFIG_ROUTING.strategyReport.path,
  },
];

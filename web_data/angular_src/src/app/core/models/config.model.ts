export interface ConfigModel {
  arm_ask: boolean;
  arm_bid: boolean;
  auto_hedge: boolean;
  is_arming: boolean;
  placed_order: {
    ask: number;
    bid: number;
  };
}

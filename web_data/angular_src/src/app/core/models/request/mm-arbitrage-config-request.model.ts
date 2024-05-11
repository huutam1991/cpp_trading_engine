export interface UpdateConfigRequest {
  sub_market?: string;
  bid_offset?: number;
  ask_offset?: number;

  bid_update_offset?:number;
  ask_update_offset?: number;

  bid_volumn?:number;
  ask_volumn?:number;

  arm_bid?:boolean;
  arm_ask?:boolean;
  auto_hedge?:boolean;

  is_arming?:boolean;
}

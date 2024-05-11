import { SideType } from "./type";

export interface OrderModel {
    orderId: number; 
    last_executed_quantity: string;
    price: string;
    quantity: string;
    side: SideType;
    status: string;
    symbol: string;
    transactTime: number;
    type: string;
}
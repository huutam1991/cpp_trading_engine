import { BaseModel } from './base';

export interface UserModel extends BaseModel {
  code: string;
  username: string;
  firstName: string;
  lastName: string;
  fullname: string;
  idNumber: string;
  phone: string;
  birthday: Date;
  bankAccount: string;
  address: string;
  position: string;
  departmentId: string;
  roleId: string;
  email: string;
  other: string;
  gender: number;
  avatar: string;
}

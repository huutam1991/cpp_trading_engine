export interface LoginModelResponse {
  registered_sources: any;
  token: string;
  user_id: string;
}

export interface LoginModelRequest {
  username: string;
  password: string;
}

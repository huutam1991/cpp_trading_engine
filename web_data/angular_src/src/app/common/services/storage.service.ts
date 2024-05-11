import { Injectable } from '@angular/core';
import { StorageKey } from '@common/enum/storage-key.enum';
import { LoginModelResponse } from '@core/models';

@Injectable({
  providedIn: 'root',
})
export class StorageService {
  constructor() {}

  getLocalStorage<T>(key: StorageKey, defaultValue = null): T | null {
    const value = localStorage.getItem(String(key));
    if (!value) {
      return defaultValue;
    }
    const object = JSON.parse(value);
    return object;
  }

  setLocalStorage<T>(key: StorageKey, value: T): void {
    localStorage.setItem(key, JSON.stringify(value));
  }

  removeLocalStorage(name: StorageKey): void {
    localStorage.removeItem(name);
  }

  getSessionStorage<T>(key: StorageKey, defaultValue = null): T | null {
    const value = sessionStorage.getItem(String(key));
    if (!value) {
      return defaultValue;
    }
    const object = JSON.parse(value);
    return object;
  }

  setSessionStorage<T>(key: StorageKey, value: T): void {
    sessionStorage.setItem(key, JSON.stringify(value));
  }

  removeSessionStorage(name: StorageKey): void {
    sessionStorage.removeItem(name);
  }

  getCurrentUser(): string {
    const user = this.getLocalStorage<LoginModelResponse>(StorageKey.TOKEN);
    return user ? user.user_id : '';
  }
}

import { defineStore } from 'pinia'
import router from '@/router'

export const useAuthStore = defineStore('auth', {
  state: () => ({
    token: localStorage.getItem('auth_token') ?? '',
  }),

  actions: {
    setToken(token: string) {
      this.token = token
      localStorage.setItem('auth_token', token)
    },

    logout() {
      this.token = ''
      localStorage.removeItem('auth_token')
      router.push('/login')
    },
  },
})
<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'

import { useAuthStore } from '@/stores/auth'
import { API_BASE_URL } from '@/config/env'

const router = useRouter()
const auth = useAuthStore()

const username = ref('')
const password = ref('')

const loading = ref(false)
const errorMessage = ref('')

type LoginResponse = {
  error: boolean
  status_code: number
  msg: string

  data: {
    user_id: string
    token: string
  }
}

async function login() {
  errorMessage.value = ''
  loading.value = true

  try {
    const response = await fetch(
      `${API_BASE_URL}/login`,
      {
        method: 'POST',
        credentials: 'include',

        headers: {
          'Content-Type': 'application/json',
        },

        body: JSON.stringify({
          username: username.value,
          password: password.value,
        }),
      },
    )

    const result: LoginResponse =
      await response.json()

    console.log(result)

    if (!response.ok || result.error) {
      errorMessage.value =
        result.msg || 'Login failed'

      return
    }

    const token = result.data.token

    console.log('TOKEN:', token)

    auth.setToken(token)

    router.push('/orderbook')
  } catch (error) {
    console.error(error)

    errorMessage.value =
      'Network error'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <main class="login-page">
    <div class="login-card">
      <h1>Login</h1>

      <div class="form-group">
        <label>Username</label>

        <input
          v-model="username"
          type="text"
          placeholder="Enter username"
        />
      </div>

      <div class="form-group">
        <label>Password</label>

        <input
          v-model="password"
          type="password"
          placeholder="Enter password"
        />
      </div>

      <button
        class="login-button"
        :disabled="loading"
        @click="login"
      >
        {{ loading ? 'Logging in...' : 'Login' }}
      </button>

      <p v-if="errorMessage" class="error-message">
        {{ errorMessage }}
      </p>
    </div>
  </main>
</template>

<style scoped>
.login-page {
  min-height: 100vh;
  width: 100%;

  display: flex;
  align-items: center;
  justify-content: center;

  background: #111827;
  color: white;

  font-family: Arial, sans-serif;

  padding: 24px;

  box-sizing: border-box;
}

.login-card {
  width: 100%;
  max-width: 420px;

  background: #1f2937;

  padding: 40px;

  border-radius: 16px;

  border: 1px solid #374151;

  box-shadow:
    0 0 40px rgba(0, 0, 0, 0.4);
}

h1 {
  margin-bottom: 24px;
  text-align: center;
}

.form-group {
  margin-bottom: 16px;
}

label {
  display: block;

  margin-bottom: 8px;

  font-size: 14px;
}

input {
  width: 100%;

  padding: 10px 12px;

  border-radius: 8px;

  border: 1px solid #374151;

  background: #111827;

  color: white;

  box-sizing: border-box;
}

input:focus {
  outline: none;
  border-color: #2563eb;
}

.login-button {
  width: 100%;

  padding: 12px;

  border: none;

  border-radius: 8px;

  background: #2563eb;

  color: white;

  font-weight: bold;

  cursor: pointer;
}

.login-button:hover {
  background: #1d4ed8;
}

.login-button:disabled {
  opacity: 0.7;
  cursor: not-allowed;
}

.error-message {
  margin-top: 16px;

  color: #ef4444;

  text-align: center;
}
</style>
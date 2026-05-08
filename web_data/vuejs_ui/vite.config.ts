import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import basicSsl from '@vitejs/plugin-basic-ssl'

export default defineConfig({
  plugins: [
    vue(),
    basicSsl(),
  ],

  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },

  server: {
    host: 'localhost',
    port: 5173,

    proxy: {
      '/vite/api': {
        target: 'https://localhost:8080',
        changeOrigin: true,
        secure: false,

        rewrite: (path) =>
          path.replace(/^\/vite\/api/, ''),
      },

      '/vite/ws': {
        target: 'wss://localhost:8083',
        ws: true,
        changeOrigin: true,
        secure: false,

        rewrite: (path) =>
          path.replace(/^\/vite\/ws/, ''),
      },
    }
  },

})
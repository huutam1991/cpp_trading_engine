import { createRouter, createWebHashHistory } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: () => {
        const auth = useAuthStore()
        return auth.isLoggedIn()
          ? '/gateway'
          : '/login'
      },
    },
    {
      path: '/login',
      name: 'login',
      component: () => import('../views/LoginView.vue'),
    },
    {
      path: '/',
      component: () => import('@/layout/MainLayout.vue'),
      children: [
        {
          path: 'gateway',
          name: 'gateway',
          component: () => import('@/views/GatewayView.vue'),
        },
        {
          path: 'orderbook',
          name: 'orderbook',
          component: () => import('@/views/OrderBookView.vue'),
        },
        {
          path: 'order',
          name: 'order',
          component: () => import('@/views/OrderView.vue'),
        },
        {
          path: 'position',
          name: 'position',
          component: () => import('@/views/PositionView.vue'),
        },
        {
          path: 'system',
          name: 'system',
          component: () => import('@/views/SystemView.vue'),
        },
      ],
    },
  ],
})

export default router

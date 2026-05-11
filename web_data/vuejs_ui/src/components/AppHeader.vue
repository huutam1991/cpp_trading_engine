<script setup lang="ts">
</script>

<template>
  <header class="app-header">
    <div class="logo">
      Tam Nguyen - C++ Trading Engine
    </div>

    <nav class="nav-menu">
      <RouterLink to="/gateway">
        Gateway
      </RouterLink>

      <RouterLink to="/orderbook">
        OrderBook
      </RouterLink>

      <RouterLink to="/order">
        Order
      </RouterLink>

      <RouterLink to="/position">
        Position
      </RouterLink>

      <RouterLink to="/system">
        System
      </RouterLink>
    </nav>

    <div class="right-panel">
      <span class="status connected">
        Connected
      </span>
    </div>
  </header>
</template>

<style scoped>
.app-header {
  height: 56px;

  display: flex;
  align-items: center;
  justify-content: space-between;

  padding: 0 20px;

  background: #111827;

  border-bottom: 1px solid #374151;

  color: white;
}

.logo {
  font-size: 18px;
  font-weight: bold;
}

.nav-menu {
  display: flex;
  gap: 20px;
}

.nav-menu a {
  color: #d1d5db;

  text-decoration: none;

  font-size: 14px;
}

.nav-menu a.router-link-active {
  color: white;
  font-weight: bold;
}

.right-panel {
  display: flex;
  align-items: center;
}

.status.connected {
  color: #16a085;
}
</style>
<script setup lang="ts">
const gateways = [
  {
    name: 'Binance Futures',
    status: 'Connected',
    instruments: 320,
    accounts: 2,
  },

  {
    name: 'Coinbase',
    status: 'Disconnected',
    instruments: 120,
    accounts: 1,
  },
]
</script>

<template>
  <div class="page">
    <h1>Gateway</h1>

    <div class="gateway-grid">
      <div
        v-for="gateway in gateways"
        :key="gateway.name"
        class="card"
      >
        <h2>{{ gateway.name }}</h2>

        <p>Status: {{ gateway.status }}</p>

        <p>
          Instruments:
          {{ gateway.instruments }}
        </p>

        <p>
          Accounts:
          {{ gateway.accounts }}
        </p>
      </div>
    </div>
  </div>
</template>

<style scoped>
.page {
  color: white;
}

.gateway-grid {
  display: grid;

  grid-template-columns:
    repeat(auto-fit, minmax(280px, 1fr));

  gap: 16px;
}

.card {
  background: #1f2937;

  border: 1px solid #374151;

  border-radius: 12px;

  padding: 20px;
}
</style>
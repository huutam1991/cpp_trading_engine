<script setup lang="ts">
const positions = [
  {
    instrument: 'BTC-USDC-PERP',
    quantity: 1.5,
    pnl: 1250,
  },

  {
    instrument: 'ETH-USDT-PERP',
    quantity: -2,
    pnl: -180,
  },
]
</script>

<template>
  <div class="page">
    <h1>Positions</h1>

    <div class="position-grid">
      <div
        v-for="position in positions"
        :key="position.instrument"
        class="card"
      >
        <h2>{{ position.instrument }}</h2>

        <p>
          Quantity:
          {{ position.quantity }}
        </p>

        <p>
          PnL:
          {{ position.pnl }}
        </p>
      </div>
    </div>
  </div>
</template>

<style scoped>
.position-grid {
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
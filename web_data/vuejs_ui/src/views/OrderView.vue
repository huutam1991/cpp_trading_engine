<script setup lang="ts">
const orders = [
  {
    instrument: 'BTC-USDC-PERP',
    side: 'BUY',
    price: 79850,
    quantity: 0.5,
  },

  {
    instrument: 'ETH-USDT-PERP',
    side: 'SELL',
    price: 4200,
    quantity: 1.2,
  },
]
</script>

<template>
  <div class="page">
    <h1>Orders</h1>

    <table>
      <thead>
        <tr>
          <th>Instrument</th>
          <th>Side</th>
          <th>Price</th>
          <th>Quantity</th>
        </tr>
      </thead>

      <tbody>
        <tr
          v-for="order in orders"
          :key="order.instrument"
        >
          <td>{{ order.instrument }}</td>

          <td>{{ order.side }}</td>

          <td>{{ order.price }}</td>

          <td>{{ order.quantity }}</td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<style scoped>
.page {
  color: white;
}

table {
  width: 100%;

  border-collapse: collapse;

  background: #1f2937;
}

th,
td {
  padding: 12px;

  border-bottom: 1px solid #374151;
}
</style>
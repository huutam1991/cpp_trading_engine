<script setup lang="ts">
const logs = [
  '[INFO] Gateway connected',
  '[INFO] OrderBook subscribed',
  '[WARN] Reconnecting websocket',
]
</script>

<template>
  <div class="page">
    <h1>System</h1>

    <div class="log-panel">
      <div
        v-for="log in logs"
        :key="log"
        class="log-line"
      >
        {{ log }}
      </div>
    </div>
  </div>
</template>

<style scoped>
.log-panel {
  background: #1f2937;

  border: 1px solid #374151;

  border-radius: 12px;

  padding: 20px;

  font-family: monospace;
}

.log-line {
  margin-bottom: 8px;
}
</style>
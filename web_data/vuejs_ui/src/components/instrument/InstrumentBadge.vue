<script setup lang="ts">
type Props = {
  symbol: string
  status?: 'active' | 'halted' | 'disabled'
  compact?: boolean
}

defineProps<Props>()
</script>

<template>
  <span
    class="instrument-badge"
    :class="[status ?? 'active', { compact }]"
  >
    <span class="dot" />
    {{ symbol }}
  </span>
</template>

<style scoped>
.instrument-badge {
  display: inline-flex;
  align-items: center;
  gap: 7px;

  padding: 5px 10px;
  border-radius: 999px;

  font-size: 13px;
  font-weight: 700;

  color: #e5e7eb;
  background: #111827;
  border: 1px solid #374151;
}

.instrument-badge.compact {
  padding: 3px 8px;
  font-size: 12px;
}

.dot {
  width: 7px;
  height: 7px;
  border-radius: 999px;
}

.active .dot {
  background: #34d399;
}

.halted .dot {
  background: #fbbf24;
}

.disabled .dot {
  background: #f87171;
}
</style>
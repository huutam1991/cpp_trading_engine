<script setup lang="ts">
import InstrumentBadge from './InstrumentBadge.vue'

export type Instrument = {
  symbol: string
  base: string
  quote: string
  type: 'spot' | 'perpetual' | 'futures'
  status: 'active' | 'halted' | 'disabled'
  tickSize: number
  lotSize: number
  minQty: number
}

defineProps<{
  instrument: Instrument
  mode?: 'gateway' | 'orderbook' | 'order'
}>()

defineEmits<{
  open: [instrument: Instrument]
  select: [instrument: Instrument]
}>()
</script>

<template>
  <tr>
    <td>
      <InstrumentBadge
        :symbol="instrument.symbol"
        :status="instrument.status"
      />
    </td>

    <td>{{ instrument.base }}</td>
    <td>{{ instrument.quote }}</td>
    <td class="capitalize">{{ instrument.type }}</td>

    <td>
      <span
        class="status"
        :class="instrument.status"
      >
        {{ instrument.status }}
      </span>
    </td>

    <td>{{ instrument.tickSize }}</td>
    <td>{{ instrument.lotSize }}</td>
    <td>{{ instrument.minQty }}</td>

    <td class="actions">
      <button
        class="button"
        @click="$emit('select', instrument)"
      >
        Select
      </button>

      <button
        class="button primary"
        @click="$emit('open', instrument)"
      >
        {{ mode === 'order' ? 'Trade' : 'Open OrderBook' }}
      </button>
    </td>
  </tr>
</template>

<style scoped>
td {
  padding: 12px;

  border-bottom: 1px solid rgba(55, 65, 81, 0.75);

  color: #e5e7eb;
}

.capitalize {
  text-transform: capitalize;
}

.status {
  text-transform: capitalize;
  font-weight: 700;
}

.status.active {
  color: #34d399;
}

.status.halted {
  color: #fbbf24;
}

.status.disabled {
  color: #f87171;
}

.actions {
  display: flex;
  gap: 8px;
  justify-content: flex-end;
}

.button {
  padding: 7px 10px;

  color: #d1d5db;
  background: #111827;

  border: 1px solid #374151;
  border-radius: 8px;

  cursor: pointer;
}

.button.primary {
  color: #93c5fd;
  background: rgba(37, 99, 235, 0.18);
  border-color: rgba(59, 130, 246, 0.45);
}

.button:hover {
  background: #273449;
}
</style>
<script setup lang="ts">
import InstrumentRow, { type Instrument } from './InstrumentRow.vue'

defineProps<{
  instruments: Instrument[]
  mode?: 'gateway' | 'orderbook' | 'order'
}>()

defineEmits<{
  open: [instrument: Instrument]
  select: [instrument: Instrument]
}>()
</script>

<template>
  <div class="table-wrap">
    <table>
      <thead>
        <tr>
          <th>Instrument</th>
          <th>Base</th>
          <th>Quote</th>
          <th>Type</th>
          <th>Status</th>
          <th>Tick</th>
          <th>Lot</th>
          <th>Min Qty</th>
          <th class="right">Action</th>
        </tr>
      </thead>

      <tbody>
        <InstrumentRow
          v-for="instrument in instruments"
          :key="instrument.symbol"
          :instrument="instrument"
          :mode="mode"
          @open="$emit('open', $event)"
          @select="$emit('select', $event)"
        />
      </tbody>
    </table>
  </div>
</template>

<style scoped>
.table-wrap {
  overflow-x: auto;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 12px;
}

table {
  width: 100%;
  border-collapse: collapse;
  min-width: 900px;
}

th {
  padding: 12px;

  color: #9ca3af;
  background: #111827;

  border-bottom: 1px solid #374151;

  text-align: left;
  font-size: 13px;
}

.right {
  text-align: right;
}
</style>
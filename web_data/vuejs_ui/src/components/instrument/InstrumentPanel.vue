<script setup lang="ts">
import { computed, ref } from 'vue'

import InstrumentSearch from './InstrumentSearch.vue'
import InstrumentFilter from './InstrumentFilter.vue'
import InstrumentTable from './InstrumentTable.vue'
import type { Instrument } from './InstrumentRow.vue'

type InstrumentType = 'all' | 'spot' | 'perpetual' | 'futures'
type StatusType = 'all' | 'active' | 'halted' | 'disabled'

const props = defineProps<{
  title?: string
  subtitle?: string
  instruments: Instrument[]
  mode?: 'gateway' | 'orderbook' | 'order'
}>()

const emit = defineEmits<{
  open: [instrument: Instrument]
  select: [instrument: Instrument]
}>()

const search = ref('')
const typeFilter = ref<InstrumentType>('all')
const statusFilter = ref<StatusType>('all')

const filteredInstruments = computed(() => {
  const keyword = search.value.trim().toLowerCase()

  return props.instruments.filter((instrument) => {
    const matchKeyword =
      keyword.length === 0 ||
      instrument.symbol.toLowerCase().includes(keyword) ||
      instrument.base.toLowerCase().includes(keyword) ||
      instrument.quote.toLowerCase().includes(keyword)

    const matchType =
      typeFilter.value === 'all' ||
      instrument.type === typeFilter.value

    const matchStatus =
      statusFilter.value === 'all' ||
      instrument.status === statusFilter.value

    return matchKeyword && matchType && matchStatus
  })
})
</script>

<template>
  <section class="instrument-panel">
    <div class="panel-header">
      <div>
        <h2>{{ title ?? 'Instruments' }}</h2>

        <p>
          {{ subtitle ?? 'Search, filter and open market instruments.' }}
        </p>
      </div>

      <div class="counter">
        {{ filteredInstruments.length }} / {{ instruments.length }}
      </div>
    </div>

    <div class="toolbar">
      <InstrumentSearch
        v-model="search"
        placeholder="Search BTC, ETH, USDC..."
      />

      <InstrumentFilter
        v-model:type="typeFilter"
        v-model:status="statusFilter"
      />
    </div>

    <InstrumentTable
      :instruments="filteredInstruments"
      :mode="mode"
      @open="emit('open', $event)"
      @select="emit('select', $event)"
    />
  </section>
</template>

<style scoped>
.instrument-panel {
  padding: 18px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 14px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.panel-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;

  margin-bottom: 16px;
}

.panel-header h2 {
  margin: 0 0 6px;

  color: white;
  font-size: 24px;
}

.panel-header p {
  margin: 0;

  color: #9ca3af;
}

.counter {
  padding: 6px 10px;

  color: #93c5fd;
  background: rgba(37, 99, 235, 0.18);

  border: 1px solid rgba(59, 130, 246, 0.45);
  border-radius: 999px;

  font-size: 13px;
  font-weight: 700;
}

.toolbar {
  display: grid;
  grid-template-columns: 320px 1fr;
  gap: 16px;

  margin-bottom: 16px;
}

@media (max-width: 900px) {
  .toolbar {
    grid-template-columns: 1fr;
  }

  .panel-header {
    flex-direction: column;
    gap: 12px;
  }
}
</style>
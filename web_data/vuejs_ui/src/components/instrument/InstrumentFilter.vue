<script setup lang="ts">
type InstrumentType = 'all' | 'spot' | 'perpetual' | 'futures'
type StatusType = 'all' | 'active' | 'halted' | 'disabled'

defineProps<{
  type: InstrumentType
  status: StatusType
}>()

defineEmits<{
  'update:type': [value: InstrumentType]
  'update:status': [value: StatusType]
}>()

const types: InstrumentType[] = ['all', 'spot', 'perpetual', 'futures']
const statuses: StatusType[] = ['all', 'active', 'halted', 'disabled']
</script>

<template>
  <div class="filters">
    <div class="filter-group">
      <span>Type</span>

      <button
        v-for="item in types"
        :key="item"
        :class="{ active: item === type }"
        @click="$emit('update:type', item)"
      >
        {{ item }}
      </button>
    </div>

    <div class="filter-group">
      <span>Status</span>

      <button
        v-for="item in statuses"
        :key="item"
        :class="{ active: item === status }"
        @click="$emit('update:status', item)"
      >
        {{ item }}
      </button>
    </div>
  </div>
</template>

<style scoped>
.filters {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.filter-group span {
  color: #9ca3af;
  font-size: 13px;
}

button {
  padding: 6px 10px;

  color: #d1d5db;
  background: #111827;

  border: 1px solid #374151;
  border-radius: 999px;

  cursor: pointer;
  text-transform: capitalize;
}

button.active {
  color: #93c5fd;
  background: rgba(37, 99, 235, 0.18);
  border-color: rgba(59, 130, 246, 0.5);
}
</style>
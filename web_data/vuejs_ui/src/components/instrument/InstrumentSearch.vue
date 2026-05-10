<script setup lang="ts">
defineProps<{
  modelValue: string
  placeholder?: string
}>()

defineEmits<{
  'update:modelValue': [value: string]
}>()
</script>

<template>
  <div class="search-box">
    <input
      :value="modelValue"
      :placeholder="placeholder ?? 'Search instrument...'"
      @input="$emit('update:modelValue', ($event.target as HTMLInputElement).value)"
    />
  </div>
</template>

<style scoped>
.search-box {
  width: 100%;
}

input {
  width: 100%;
  box-sizing: border-box;

  padding: 11px 13px;

  color: white;
  background: #111827;

  border: 1px solid #374151;
  border-radius: 10px;
}

input:focus {
  outline: none;
  border-color: #3b82f6;
}
</style>
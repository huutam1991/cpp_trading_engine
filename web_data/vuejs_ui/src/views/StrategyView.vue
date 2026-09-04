<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

type JsonPrimitive = string | number | boolean | null

interface JsonObject {
  [key: string]: JsonValue
}

interface JsonArray extends Array<JsonValue> {}

type JsonValue = JsonPrimitive | JsonObject | JsonArray

type StrategyListResponse = {
  error: boolean
  status_code: number
  msg: string
  data: string[]
}

type StrategyConfigResponse = {
  error: boolean
  status_code: number
  msg: string
  data: JsonObject
}

type ConfigRow = {
  key: string
  path: (string | number)[]
  depth: number
  kind: 'group' | 'value'
  value?: JsonPrimitive
  groupType?: 'object' | 'array'
  childCount?: number
}

const strategies = ref<string[]>([])
const selectedStrategy = ref<string | null>(null)
const strategyConfig = ref<JsonObject | null>(null)
const originalStrategyConfig = ref<JsonObject | null>(null)

const listLoading = ref(false)
const configLoading = ref(false)
const listErrorMessage = ref('')
const configErrorMessage = ref('')

const hasStrategies = computed(() => strategies.value.length > 0)

const configRows = computed<ConfigRow[]>(() => {
  if (!strategyConfig.value) {
    return []
  }

  return flattenConfig(strategyConfig.value)
})

const isDirty = computed(() => {
  if (!strategyConfig.value || !originalStrategyConfig.value) {
    return false
  }

  return JSON.stringify(strategyConfig.value) !== JSON.stringify(originalStrategyConfig.value)
})

function cloneJson<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T
}

function isPlainObject(value: JsonValue): value is JsonObject {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function flattenConfig(root: JsonObject): ConfigRow[] {
  const rows: ConfigRow[] = []

  const visit = (
    value: JsonValue,
    key: string,
    path: (string | number)[],
    depth: number,
  ) => {
    if (isPlainObject(value)) {
      rows.push({
        key,
        path,
        depth,
        kind: 'group',
        groupType: 'object',
        childCount: Object.keys(value).length,
      })

      for (const [childKey, childValue] of Object.entries(value)) {
        visit(childValue, childKey, [...path, childKey], depth + 1)
      }

      return
    }

    if (Array.isArray(value)) {
      rows.push({
        key,
        path,
        depth,
        kind: 'group',
        groupType: 'array',
        childCount: value.length,
      })

      value.forEach((childValue, index) => {
        visit(childValue, `[${index}]`, [...path, index], depth + 1)
      })

      return
    }

    rows.push({
      key,
      path,
      depth,
      kind: 'value',
      value,
    })
  }

  for (const [key, value] of Object.entries(root)) {
    visit(value, key, [key], 0)
  }

  return rows
}

async function fetchStrategyList() {
  listLoading.value = true
  listErrorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/strategy_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: StrategyListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      listErrorMessage.value = result.msg || 'Failed to fetch strategy list.'
      strategies.value = []
      return
    }

    strategies.value = Array.isArray(result.data) ? result.data : []

    if (
      selectedStrategy.value &&
      !strategies.value.includes(selectedStrategy.value)
    ) {
      selectedStrategy.value = null
      strategyConfig.value = null
      originalStrategyConfig.value = null
      configErrorMessage.value = ''
    }
  } catch (error) {
    console.error('Fetch strategy list error:', error)
    listErrorMessage.value = 'Fetch strategy list error.'
    strategies.value = []
  } finally {
    listLoading.value = false
  }
}

async function fetchStrategyConfig(strategyName: string) {
  selectedStrategy.value = strategyName
  configLoading.value = true
  configErrorMessage.value = ''
  strategyConfig.value = null
  originalStrategyConfig.value = null

  try {
    const query = new URLSearchParams({
      strategy_name: strategyName,
    })

    const response = await fetch(
      `${API_BASE_URL}/strategy_config?${query.toString()}`,
      {
        method: 'GET',
        credentials: 'include',
      },
    )

    const result: StrategyConfigResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      configErrorMessage.value = result.msg || 'Failed to fetch strategy config.'
      return
    }

    strategyConfig.value = cloneJson(result.data ?? {})
    originalStrategyConfig.value = cloneJson(result.data ?? {})
  } catch (error) {
    console.error('Fetch strategy config error:', error)
    configErrorMessage.value = 'Fetch strategy config error.'
  } finally {
    configLoading.value = false
  }
}

function selectStrategy(strategyName: string) {
  if (configLoading.value && selectedStrategy.value === strategyName) {
    return
  }

  fetchStrategyConfig(strategyName)
}

function refreshCurrentStrategy() {
  if (!selectedStrategy.value) {
    return
  }

  fetchStrategyConfig(selectedStrategy.value)
}

function resetChanges() {
  if (!originalStrategyConfig.value) {
    return
  }

  strategyConfig.value = cloneJson(originalStrategyConfig.value)
}

function getValueAtPath(path: (string | number)[]): JsonValue | undefined {
  let current: JsonValue | undefined = strategyConfig.value ?? undefined

  for (const segment of path) {
    if (current === undefined || current === null) {
      return undefined
    }

    if (Array.isArray(current) && typeof segment === 'number') {
      current = current[segment]
      continue
    }

    if (isPlainObject(current) && typeof segment === 'string') {
      current = current[segment]
      continue
    }

    return undefined
  }

  return current
}

function setValueAtPath(path: (string | number)[], value: JsonPrimitive) {
  if (!strategyConfig.value || path.length === 0) {
    return
  }

  let current: JsonValue = strategyConfig.value

  for (let index = 0; index < path.length - 1; index += 1) {
    const segment = path[index]!

    if (Array.isArray(current) && typeof segment === 'number') {
      current = current[segment]!
      continue
    }

    if (isPlainObject(current) && typeof segment === 'string') {
      current = current[segment]!
      continue
    }

    return
  }

  const finalSegment = path[path.length - 1]!

  if (Array.isArray(current) && typeof finalSegment === 'number') {
    current[finalSegment] = value
    return
  }

  if (isPlainObject(current) && typeof finalSegment === 'string') {
    current[finalSegment] = value
  }
}

function updateTextValue(row: ConfigRow, event: Event) {
  const target = event.target as HTMLInputElement
  const currentValue = getValueAtPath(row.path)

  if (typeof currentValue === 'number') {
    if (target.value === '') {
      return
    }

    const parsed = Number(target.value)

    if (Number.isFinite(parsed)) {
      setValueAtPath(row.path, parsed)
    }

    return
  }

  if (currentValue === null) {
    setValueAtPath(row.path, target.value)
    return
  }

  setValueAtPath(row.path, target.value)
}

function updateBooleanValue(row: ConfigRow, event: Event) {
  const target = event.target as HTMLSelectElement
  setValueAtPath(row.path, target.value === 'true')
}

function rowValueType(value: JsonPrimitive | undefined) {
  if (value === null) return 'null'
  return typeof value
}

function formatStrategyName(name: string) {
  return name.replaceAll('_', ' ')
}

onMounted(() => {
  fetchStrategyList()
})
</script>

<template>
  <main class="strategy-page">
    <section class="strategy-layout">
      <aside class="strategy-sidebar">
        <div class="sidebar-header">
          <div>
            <h2>Strategies</h2>
            <p>Running strategies</p>
          </div>

          <span class="strategy-total">{{ strategies.length }}</span>
        </div>

        <div
          v-if="listLoading"
          class="sidebar-message"
        >
          Loading strategies...
        </div>

        <div
          v-else-if="listErrorMessage"
          class="sidebar-message error-message"
        >
          {{ listErrorMessage }}
        </div>

        <div
          v-else-if="!hasStrategies"
          class="sidebar-empty"
        >
          No strategy is currently running.
        </div>

        <template v-else>
          <button
            v-for="strategy in strategies"
            :key="strategy"
            class="strategy-card"
            :class="{ active: selectedStrategy === strategy }"
            @click="selectStrategy(strategy)"
          >
            <span class="strategy-status-dot" />

            <span class="strategy-card-content">
              <strong>{{ formatStrategyName(strategy) }}</strong>
              <small>{{ strategy }}</small>
            </span>
          </button>
        </template>
      </aside>

      <section class="strategy-panel">
        <div class="strategy-header">
          <div>
            <h1>Strategy</h1>
            <p v-if="selectedStrategy">
              {{ selectedStrategy }}
            </p>
            <p v-else>
              Select a running strategy to inspect its configuration.
            </p>
          </div>

          <div class="header-actions">
            <span
              v-if="isDirty"
              class="dirty-badge"
            >
              Modified
            </span>

            <button
              v-if="isDirty"
              class="reset-button"
              @click="resetChanges"
            >
              Reset
            </button>

            <button
              class="refresh-button"
              :disabled="listLoading || configLoading"
              title="Refresh"
              @click="selectedStrategy ? refreshCurrentStrategy() : fetchStrategyList()"
            >
              ↻
            </button>
          </div>
        </div>

        <div
          v-if="!selectedStrategy"
          class="empty-content"
        >
          <div class="empty-content-icon">{ }</div>
          <h2>Strategy Config</h2>
          <p>
            Choose a strategy from the left sidebar. Its JSON configuration will appear here.
          </p>
        </div>

        <div
          v-else-if="configLoading"
          class="panel-message"
        >
          Loading strategy config...
        </div>

        <div
          v-else-if="configErrorMessage"
          class="panel-message error-message"
        >
          {{ configErrorMessage }}
        </div>

        <div
          v-else-if="strategyConfig"
          class="config-card"
        >
          <div class="config-card-header">
            <div>
              <h2>Data</h2>
              <p>Strategy configuration</p>
            </div>

            <span class="config-row-count">
              {{ configRows.filter((row) => row.kind === 'value').length }} values
            </span>
          </div>

          <div
            v-if="configRows.length === 0"
            class="config-empty"
          >
            Configuration is empty.
          </div>

          <div
            v-else
            class="config-tree"
          >
            <div
              v-for="row in configRows"
              :key="row.path.join('.')"
              class="config-row"
              :class="[
                row.kind === 'group' ? 'group-row' : 'value-row',
                { 'root-row': row.depth === 0 },
              ]"
              :style="{ '--depth': row.depth }"
            >
              <template v-if="row.kind === 'group'">
                <div class="group-key">
                  <span class="tree-branch">⌄</span>
                  <strong>{{ row.key }}</strong>
                  <span class="group-type">
                    {{ row.groupType === 'array' ? 'array' : 'object' }} · {{ row.childCount }}
                  </span>
                </div>
              </template>

              <template v-else>
                <div class="config-key-cell">
                  <span class="tree-guide" />
                  <span class="config-key">{{ row.key }}</span>
                </div>

                <div class="config-value-cell">
                  <select
                    v-if="typeof row.value === 'boolean'"
                    class="config-input config-select"
                    :value="String(getValueAtPath(row.path))"
                    @change="updateBooleanValue(row, $event)"
                  >
                    <option value="true">true</option>
                    <option value="false">false</option>
                  </select>

                  <input
                    v-else
                    class="config-input"
                    :class="{ 'number-input': typeof row.value === 'number' }"
                    :type="typeof row.value === 'number' ? 'number' : 'text'"
                    :step="typeof row.value === 'number' ? 'any' : undefined"
                    :value="getValueAtPath(row.path) ?? ''"
                    @input="updateTextValue(row, $event)"
                  >

                  <span class="value-type">
                    {{ rowValueType(row.value) }}
                  </span>
                </div>
              </template>
            </div>
          </div>
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.strategy-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.strategy-layout {
  display: grid;
  grid-template-columns: 220px minmax(760px, 1fr);
  gap: 12px;
}

.strategy-sidebar,
.strategy-panel {
  background: #111827;
  border: 1px solid #374151;
  border-radius: 12px;
  box-shadow: none;
}

.strategy-sidebar {
  min-height: 720px;
  padding: 14px 12px;
}

.strategy-panel {
  min-height: 720px;
  padding: 22px;
}

.sidebar-header,
.strategy-header,
.config-card-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.sidebar-header {
  margin-bottom: 18px;
}

.sidebar-header h2,
.strategy-header h1,
.config-card-header h2 {
  margin: 0;
  color: #ffffff;
  font-size: 18px;
  font-weight: 800;
  letter-spacing: -0.02em;
}

.sidebar-header p,
.strategy-header p,
.config-card-header p {
  margin: 5px 0 0;
  color: #9ca3af;
  font-size: 13px;
}

.strategy-total {
  min-width: 36px;
  height: 36px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: #bfdbfe;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
  border-radius: 999px;
  font-weight: 800;
}

.strategy-card {
  width: 100%;
  min-height: 72px;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 12px;
  margin-bottom: 12px;
  color: #e5e7eb;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 12px;
  text-align: left;
  cursor: pointer;
  transition: none;
}

.strategy-card:hover {
  background: #273449;
  border-color: #4b5563;
}

.strategy-card.active {
  background: #1e3a5f;
  border-color: #3b82f6;
}

.strategy-status-dot {
  width: 9px;
  height: 9px;
  flex: 0 0 9px;
  border-radius: 999px;
  background: #34d399;
  box-shadow: 0 0 0 3px #1f3a35;
}

.strategy-card-content {
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.strategy-card-content strong {
  overflow: hidden;
  color: #60a5fa;
  font-size: 13px;
  font-weight: 800;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.strategy-card-content small {
  overflow: hidden;
  color: #9ca3af;
  font-size: 10px;
  font-weight: 700;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.sidebar-message,
.sidebar-empty,
.panel-message,
.config-empty {
  padding: 18px 14px;
  color: #9ca3af;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
  font-size: 12px;
  line-height: 1.5;
}

.error-message {
  color: #fca5a5;
  background: #1f2937;
  border-color: #7f1d1d;
}

.strategy-header {
  margin-bottom: 18px;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.refresh-button,
.reset-button {
  height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
  cursor: pointer;
  transition: none;
}

.refresh-button {
  width: 42px;
  font-size: 24px;
}

.reset-button {
  padding: 0 14px;
  font-size: 12px;
  font-weight: 800;
}

.refresh-button:hover,
.reset-button:hover {
  color: #ffffff;
  background: #1f2937;
  border-color: #4b5563;
}

.refresh-button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.dirty-badge {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 9px;
  color: #fbbf24;
  background: #3a3320;
  border: 1px solid #8a6d1f;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.empty-content {
  min-height: 560px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px;
  color: #9ca3af;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
  text-align: center;
}

.empty-content-icon {
  width: 58px;
  height: 58px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 14px;
  color: #60a5fa;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
  border-radius: 12px;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 19px;
  font-weight: 900;
}

.empty-content h2 {
  margin: 0 0 8px;
  color: #ffffff;
  font-size: 18px;
  font-weight: 800;
}

.empty-content p {
  max-width: 410px;
  margin: 0;
  font-size: 13px;
  line-height: 1.6;
}

.config-card {
  overflow: hidden;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
}

.config-card-header {
  padding: 16px 18px;
  border-bottom: 1px solid #374151;
}

.config-card-header h2 {
  font-size: 16px;
  font-weight: 900;
}

.config-row-count {
  min-height: 28px;
  display: inline-flex;
  align-items: center;
  padding: 0 9px;
  color: #bfdbfe;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 800;
}

.config-tree {
  overflow-x: auto;
}

.config-row {
  --indent-size: 24px;
  min-width: 620px;
  border-bottom: 1px solid #374151;
}

.config-row:last-child {
  border-bottom: 0;
}

.group-row {
  display: flex;
  align-items: center;
  min-height: 46px;
  padding: 7px 16px 7px calc(16px + (var(--depth) * var(--indent-size)));
  background: #182235;
}

.group-row.root-row {
  background: #172033;
}

.group-key {
  display: flex;
  align-items: center;
  gap: 8px;
}

.group-key strong {
  color: #ffffff;
  font-size: 13px;
  font-weight: 900;
}

.tree-branch {
  color: #60a5fa;
  font-size: 14px;
  font-weight: 900;
}

.group-type,
.value-type {
  display: inline-flex;
  align-items: center;
  min-height: 22px;
  padding: 0 7px;
  color: #9ca3af;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 5px;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 10px;
  font-weight: 700;
}

.value-row {
  display: grid;
  grid-template-columns: minmax(230px, 0.8fr) minmax(320px, 1.2fr);
  align-items: center;
  min-height: 52px;
  background: #1f2937;
}

.value-row:hover {
  background: #273449;
}

.config-key-cell {
  min-width: 0;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px 8px calc(16px + (var(--depth) * var(--indent-size)));
}

.tree-guide {
  width: 8px;
  height: 22px;
  flex: 0 0 8px;
  border-left: 1px solid #4b5563;
  border-bottom: 1px solid #4b5563;
  border-bottom-left-radius: 4px;
}

.config-key {
  overflow: hidden;
  color: #cbd5e1;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 12px;
  font-weight: 800;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.config-value-cell {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  border-left: 1px solid #374151;
}

.config-input {
  width: min(100%, 520px);
  height: 34px;
  box-sizing: border-box;
  padding: 0 10px;
  color: #f8fafc;
  background: #111827;
  border: 1px solid #4b5563;
  border-radius: 6px;
  outline: none;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 12px;
  font-weight: 700;
}

.config-input:hover {
  border-color: #6b7280;
}

.config-input:focus {
  background: #0f172a;
  border-color: #3b82f6;
  box-shadow: 0 0 0 1px #3b82f6;
}

.config-select {
  cursor: pointer;
}

.number-input {
  color: #93c5fd;
}

@media (max-width: 1050px) {
  .strategy-layout {
    grid-template-columns: 1fr;
  }

  .strategy-sidebar,
  .strategy-panel {
    min-height: unset;
  }

  .strategy-sidebar {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
  }

  .sidebar-header,
  .sidebar-message,
  .sidebar-empty {
    grid-column: 1 / -1;
  }

  .strategy-card {
    margin-bottom: 0;
  }
}

@media (max-width: 700px) {
  .strategy-sidebar {
    grid-template-columns: 1fr;
  }

  .strategy-panel {
    padding: 14px;
  }

  .strategy-header {
    align-items: flex-start;
  }

  .value-row {
    grid-template-columns: 1fr;
  }

  .config-value-cell {
    padding-left: calc(40px + (var(--depth) * var(--indent-size)));
    border-top: 1px solid #374151;
    border-left: 0;
  }
}
</style>

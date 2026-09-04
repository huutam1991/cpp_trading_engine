<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
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

type StrategyCurrentInfoResponse = {
  error: boolean
  status_code: number
  msg: string
  data: JsonObject
}

type InstrumentItem = {
  price_precision: number
  tick_size: number
  lot_size: number
  exchange_symbol: string
  symbol: string
  instrument_type: string
  exchange_id: string
}

type InstrumentSubscribeResponse = {
  error: boolean
  status_code: number
  msg: string
  data: InstrumentItem[]
}

type AccountItem = {
  exchange_id: string
  is_active: boolean
  key: string
}

type AccountListResponse = {
  error: boolean
  status_code: number
  msg: string
  data: AccountItem[]
}

const SPECIAL_ROOT_KEYS = new Set(['is_running', 'symbol', 'account'])

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
const currentStrategyInfo = ref<JsonObject | null>(null)

const subscribedInstruments = ref<InstrumentItem[]>([])
const accounts = ref<AccountItem[]>([])

const listLoading = ref(false)
const configLoading = ref(false)
const instrumentsLoading = ref(false)
const accountsLoading = ref(false)
const controlLoading = ref(false)
const currentInfoLoading = ref(false)

const listErrorMessage = ref('')
const configErrorMessage = ref('')
const instrumentsErrorMessage = ref('')
const accountsErrorMessage = ref('')
const controlErrorMessage = ref('')
const currentInfoErrorMessage = ref('')

// Read-only current-info tree expansion state. Paths in this set are collapsed.
const collapsedCurrentInfoGroups = ref<Set<string>>(new Set())

let currentInfoPollTimer: ReturnType<typeof setInterval> | null = null
let currentInfoRequestInFlight = false

const hasStrategies = computed(() => strategies.value.length > 0)

const strategyIsRunning = computed(() => strategyConfig.value?.is_running === true)

const strategySymbol = computed(() => {
  const value = strategyConfig.value?.symbol
  return typeof value === 'string' ? value : ''
})

const strategyAccount = computed(() => {
  const value = strategyConfig.value?.account
  return typeof value === 'string' ? value : ''
})

const activeAccounts = computed(() => accounts.value.filter((account) => account.is_active))
const inactiveAccounts = computed(() => accounts.value.filter((account) => !account.is_active))

const selectedAccountInfo = computed(() =>
  accounts.value.find((account) => account.key === strategyAccount.value) ?? null,
)

const configRows = computed<ConfigRow[]>(() => {
  if (!strategyConfig.value) {
    return []
  }

  return flattenConfig(strategyConfig.value)
})

const currentInfoRows = computed<ConfigRow[]>(() => {
  if (!currentStrategyInfo.value) {
    return []
  }

  return flattenJson(currentStrategyInfo.value)
})

const visibleCurrentInfoRows = computed<ConfigRow[]>(() =>
  currentInfoRows.value.filter((row) => {
    // A row is hidden when any of its ancestor groups is collapsed.
    for (let depth = 1; depth < row.path.length; depth += 1) {
      const ancestorPath = row.path.slice(0, depth)
      if (collapsedCurrentInfoGroups.value.has(jsonPathKey(ancestorPath))) {
        return false
      }
    }

    return true
  }),
)

const isDirty = computed(() => {
  if (!strategyConfig.value || !originalStrategyConfig.value) {
    return false
  }

  return JSON.stringify(strategyConfig.value) !== JSON.stringify(originalStrategyConfig.value)
})

const visibleValueCount = computed(() => {
  const normalValueCount = configRows.value.filter((row) => row.kind === 'value').length
  const specialEditableCount = strategyConfig.value ? 2 : 0
  return normalValueCount + specialEditableCount
})

const currentInfoValueCount = computed(() =>
  currentInfoRows.value.filter((row) => row.kind === 'value').length,
)

function jsonPathKey(path: (string | number)[]): string {
  return JSON.stringify(path)
}

function isCurrentInfoGroupExpanded(row: ConfigRow): boolean {
  return !collapsedCurrentInfoGroups.value.has(jsonPathKey(row.path))
}

function toggleCurrentInfoGroup(row: ConfigRow) {
  if (row.kind !== 'group') {
    return
  }

  const key = jsonPathKey(row.path)
  const next = new Set(collapsedCurrentInfoGroups.value)

  if (next.has(key)) {
    next.delete(key)
  } else {
    next.add(key)
  }

  collapsedCurrentInfoGroups.value = next
}

function collapseAllCurrentInfoGroups() {
  collapsedCurrentInfoGroups.value = new Set(
    currentInfoRows.value
      .filter((row) => row.kind === 'group')
      .map((row) => jsonPathKey(row.path)),
  )
}

function expandAllCurrentInfoGroups() {
  collapsedCurrentInfoGroups.value = new Set()
}

function cloneJson<T>(value: T): T {
  return JSON.parse(JSON.stringify(value)) as T
}

function isPlainObject(value: JsonValue): value is JsonObject {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}

function flattenJson(root: JsonObject, excludedRootKeys?: Set<string>): ConfigRow[] {
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
    if (excludedRootKeys?.has(key)) {
      continue
    }

    visit(value, key, [key], 0)
  }

  return rows
}

function flattenConfig(root: JsonObject): ConfigRow[] {
  return flattenJson(root, SPECIAL_ROOT_KEYS)
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
      currentStrategyInfo.value = null
      configErrorMessage.value = ''
    }

    // Auto-activate the first strategy when entering the view.
    // This mirrors a user click: load its config immediately and, if it is
    // already running, fetchStrategyConfig() will start current-info polling.
    if (!selectedStrategy.value && strategies.value.length > 0) {
      const firstStrategy = strategies.value[0]

      if (firstStrategy) {
        await fetchStrategyConfig(firstStrategy)
      }
    }

  } catch (error) {
    console.error('Fetch strategy list error:', error)
    listErrorMessage.value = 'Fetch strategy list error.'
    strategies.value = []
  } finally {
    listLoading.value = false
  }
}

async function fetchSubscribedInstruments() {
  instrumentsLoading.value = true
  instrumentsErrorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/instrument_subscribe`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: InstrumentSubscribeResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      instrumentsErrorMessage.value = result.msg || 'Failed to fetch subscribed instruments.'
      subscribedInstruments.value = []
      return
    }

    subscribedInstruments.value = Array.isArray(result.data) ? result.data : []
  } catch (error) {
    console.error('Fetch subscribed instruments error:', error)
    instrumentsErrorMessage.value = 'Fetch subscribed instruments error.'
    subscribedInstruments.value = []
  } finally {
    instrumentsLoading.value = false
  }
}

async function fetchAccountList() {
  accountsLoading.value = true
  accountsErrorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/account_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: AccountListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      accountsErrorMessage.value = result.msg || 'Failed to fetch account list.'
      accounts.value = []
      return
    }

    accounts.value = Array.isArray(result.data) ? result.data : []
  } catch (error) {
    console.error('Fetch account list error:', error)
    accountsErrorMessage.value = 'Fetch account list error.'
    accounts.value = []
  } finally {
    accountsLoading.value = false
  }
}

async function fetchStrategyCurrentInfo(strategyName: string, showLoading = false) {
  if (currentInfoRequestInFlight) {
    return
  }

  currentInfoRequestInFlight = true

  if (showLoading) {
    currentInfoLoading.value = true
  }

  currentInfoErrorMessage.value = ''

  try {
    const query = new URLSearchParams({
      strategy_name: strategyName,
    })

    const response = await fetch(
      `${API_BASE_URL}/strategy_current_info?${query.toString()}`,
      {
        method: 'POST',
        credentials: 'include',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({}),
      },
    )

    const result: StrategyCurrentInfoResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      stopCurrentInfoPolling()
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      currentInfoErrorMessage.value = result.msg || 'Failed to fetch current strategy info.'
      return
    }

    // Ignore a response that belongs to a strategy that is no longer selected
    // or has already been stopped while this request was in flight.
    if (selectedStrategy.value !== strategyName || !strategyIsRunning.value) {
      return
    }

    currentStrategyInfo.value = cloneJson(result.data ?? {})
  } catch (error) {
    console.error('Fetch current strategy info error:', error)
    currentInfoErrorMessage.value = 'Fetch current strategy info error.'
  } finally {
    currentInfoRequestInFlight = false
    currentInfoLoading.value = false
  }
}

function stopCurrentInfoPolling() {
  if (currentInfoPollTimer !== null) {
    clearInterval(currentInfoPollTimer)
    currentInfoPollTimer = null
  }
}

function startCurrentInfoPolling(strategyName: string) {
  stopCurrentInfoPolling()
  currentStrategyInfo.value = null
  currentInfoErrorMessage.value = ''
  collapsedCurrentInfoGroups.value = new Set()

  // Load immediately, then refresh once every second while the strategy is running.
  void fetchStrategyCurrentInfo(strategyName, true)

  currentInfoPollTimer = setInterval(() => {
    if (selectedStrategy.value !== strategyName || !strategyIsRunning.value) {
      stopCurrentInfoPolling()
      return
    }

    void fetchStrategyCurrentInfo(strategyName)
  }, 1000)
}

async function fetchStrategyConfig(strategyName: string) {
  stopCurrentInfoPolling()
  selectedStrategy.value = strategyName
  configLoading.value = true
  configErrorMessage.value = ''
  controlErrorMessage.value = ''
  strategyConfig.value = null
  originalStrategyConfig.value = null
  currentStrategyInfo.value = null
  currentInfoErrorMessage.value = ''
  collapsedCurrentInfoGroups.value = new Set()

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

    if (strategyConfig.value.is_running === true) {
      startCurrentInfoPolling(strategyName)
    }
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

  if (strategyIsRunning.value) {
    void fetchStrategyCurrentInfo(selectedStrategy.value, true)
    return
  }

  void fetchStrategyConfig(selectedStrategy.value)
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

function updateSpecialStringValue(key: 'symbol' | 'account', event: Event) {
  const target = event.target as HTMLSelectElement

  if (!strategyConfig.value) {
    return
  }

  strategyConfig.value[key] = target.value
}

async function toggleStrategyRunning() {
  if (!selectedStrategy.value || !strategyConfig.value || controlLoading.value) {
    return
  }

  controlLoading.value = true
  controlErrorMessage.value = ''

  try {
    const query = new URLSearchParams({
      strategy_name: selectedStrategy.value,
    })

    // Always send the complete CURRENT config. Only is_running is changed
    // according to the requested Start/Stop action.
    const payload = cloneJson(strategyConfig.value)
    payload.is_running = !strategyIsRunning.value

    const response = await fetch(
      `${API_BASE_URL}/strategy_config?${query.toString()}`,
      {
        method: 'POST',
        credentials: 'include',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload),
      },
    )

    const result: StrategyConfigResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      controlErrorMessage.value = result.msg || 'Failed to update strategy config.'
      return
    }

    // The server response is the new source of truth. Refresh the whole UI
    // from it so all normalized/updated values returned by the backend appear.
    strategyConfig.value = cloneJson(result.data ?? {})
    originalStrategyConfig.value = cloneJson(result.data ?? {})

    if (strategyConfig.value.is_running === true) {
      startCurrentInfoPolling(selectedStrategy.value)
    } else {
      stopCurrentInfoPolling()
      currentStrategyInfo.value = null
      currentInfoErrorMessage.value = ''
    }
  } catch (error) {
    console.error('Update strategy config error:', error)
    controlErrorMessage.value = 'Update strategy config error.'
  } finally {
    controlLoading.value = false
  }
}

function formatReadOnlyValue(value: JsonPrimitive | undefined) {
  if (value === null) return 'null'
  if (value === undefined) return ''
  if (typeof value === 'boolean') return value ? 'true' : 'false'
  return String(value)
}

function rowValueType(value: JsonPrimitive | undefined) {
  if (value === null) return 'null'
  return typeof value
}

function formatStrategyName(name: string) {
  return name.replaceAll('_', ' ')
}

onMounted(() => {
  void Promise.all([
    fetchStrategyList(),
    fetchSubscribedInstruments(),
    fetchAccountList(),
  ])
})

onBeforeUnmount(() => {
  stopCurrentInfoPolling()
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
          <div class="strategy-title-block">
            <h1>Strategy</h1>

            <div
              v-if="selectedStrategy && strategyConfig"
              class="strategy-title-line"
            >
              <p>{{ selectedStrategy }}</p>

              <span
                class="running-state"
                :class="strategyIsRunning ? 'running' : 'stopping'"
              >
                <span class="running-state-dot" />
                {{ strategyIsRunning ? 'is Running' : 'is Stopping' }}
              </span>

              <button
                class="strategy-control-button"
                :class="strategyIsRunning ? 'stop-button' : 'start-button'"
                :disabled="controlLoading"
                @click="toggleStrategyRunning"
              >
                {{ controlLoading ? 'Processing...' : strategyIsRunning ? 'Stop' : 'Start' }}
              </button>
            </div>

            <p v-else-if="selectedStrategy">
              {{ selectedStrategy }}
            </p>
            <p v-else>
              Select a running strategy to inspect its configuration.
            </p>

            <p
              v-if="controlErrorMessage"
              class="control-error"
            >
              {{ controlErrorMessage }}
            </p>
          </div>

          <div class="header-actions">
            <span
              v-if="isDirty && !strategyIsRunning"
              class="dirty-badge"
            >
              Modified
            </span>

            <button
              v-if="isDirty && !strategyIsRunning"
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
          v-else-if="strategyConfig && strategyIsRunning"
          class="config-card"
        >
          <div class="config-card-header">
            <div>
              <h2>Data</h2>
              <p>Current strategy information · updates every second</p>
            </div>

            <div class="current-info-header-meta">
              <button
                type="button"
                class="tree-action-button"
                title="Collapse all JSON groups"
                @click="collapseAllCurrentInfoGroups"
              >
                Collapse all
              </button>

              <button
                v-if="collapsedCurrentInfoGroups.size > 0"
                type="button"
                class="tree-action-button secondary"
                title="Expand all JSON groups"
                @click="expandAllCurrentInfoGroups"
              >
                Expand all
              </button>

              <span class="live-badge">
                <span class="live-badge-dot" />
                Live
              </span>

              <span class="config-row-count">
                {{ currentInfoValueCount }} values
              </span>
            </div>
          </div>

          <div
            v-if="currentInfoLoading && !currentStrategyInfo"
            class="current-info-message"
          >
            Loading current strategy info...
          </div>

          <div
            v-else-if="currentInfoErrorMessage && !currentStrategyInfo"
            class="current-info-message error-message"
          >
            {{ currentInfoErrorMessage }}
          </div>

          <template v-else-if="currentStrategyInfo">
            <div
              v-if="currentInfoErrorMessage"
              class="current-info-inline-error"
            >
              {{ currentInfoErrorMessage }} · Retrying automatically...
            </div>

            <div
              v-if="currentInfoRows.length === 0"
              class="config-empty config-empty-after-special"
            >
              No current strategy information.
            </div>

            <div
              v-else
              class="config-tree current-info-tree"
            >
              <div
                v-for="row in visibleCurrentInfoRows"
                :key="`current.${jsonPathKey(row.path)}`"
                class="config-row"
                :class="[
                  row.kind === 'group' ? 'group-row' : 'value-row',
                  { 'root-row': row.depth === 0 },
                ]"
                :style="{ '--depth': row.depth }"
              >
                <template v-if="row.kind === 'group'">
                  <button
                    type="button"
                    class="group-key group-toggle-button"
                    :aria-expanded="isCurrentInfoGroupExpanded(row)"
                    @click="toggleCurrentInfoGroup(row)"
                  >
                    <span
                      class="tree-branch collapsible-branch"
                      :class="{ collapsed: !isCurrentInfoGroupExpanded(row) }"
                    >⌄</span>
                    <strong>{{ row.key }}</strong>
                    <span class="group-type">
                      {{ row.groupType === 'array' ? 'array' : 'object' }} · {{ row.childCount }}
                    </span>
                  </button>
                </template>

                <template v-else>
                  <div class="config-key-cell">
                    <span class="tree-guide" />
                    <span class="config-key">{{ row.key }}</span>
                  </div>

                  <div class="config-value-cell readonly-value-cell">
                    <span
                      class="readonly-value"
                      :class="`readonly-${rowValueType(row.value)}`"
                    >
                      {{ formatReadOnlyValue(row.value) }}
                    </span>

                    <span class="value-type">
                      {{ rowValueType(row.value) }}
                    </span>
                  </div>
                </template>
              </div>
            </div>
          </template>
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
              {{ visibleValueCount }} values
            </span>
          </div>

          <div class="special-config-section">
            <div class="special-config-row">
              <div class="special-config-label">
                <span class="special-key">symbol</span>
                <span class="special-description">Subscribed instrument</span>
              </div>

              <div class="special-config-control">
                <select
                  class="config-input config-select special-select"
                  :value="strategySymbol"
                  :disabled="instrumentsLoading"
                  @change="updateSpecialStringValue('symbol', $event)"
                >
                  <option
                    v-if="strategySymbol && !subscribedInstruments.some((item) => item.symbol === strategySymbol)"
                    :value="strategySymbol"
                  >
                    {{ strategySymbol }}
                  </option>
                  <option
                    v-if="instrumentsLoading"
                    disabled
                  >
                    Loading instruments...
                  </option>
                  <option
                    v-for="instrument in subscribedInstruments"
                    :key="`${instrument.exchange_id}:${instrument.symbol}`"
                    :value="instrument.symbol"
                  >
                    {{ instrument.symbol }}
                  </option>
                </select>

                <span class="value-type">select</span>
                <span
                  v-if="instrumentsErrorMessage"
                  class="inline-error"
                >
                  {{ instrumentsErrorMessage }}
                </span>
              </div>
            </div>

            <div class="special-config-row">
              <div class="special-config-label">
                <span class="special-key">account</span>
                <span class="special-description">Trading account</span>
              </div>

              <div class="special-config-control">
                <select
                  class="config-input config-select special-select"
                  :class="{ 'inactive-selection': selectedAccountInfo && !selectedAccountInfo.is_active }"
                  :value="strategyAccount"
                  :disabled="accountsLoading"
                  @change="updateSpecialStringValue('account', $event)"
                >
                  <option
                    v-if="strategyAccount && !accounts.some((item) => item.key === strategyAccount)"
                    :value="strategyAccount"
                  >
                    {{ strategyAccount }}
                  </option>

                  <option
                    v-if="accountsLoading"
                    disabled
                  >
                    Loading accounts...
                  </option>

                  <optgroup
                    v-if="activeAccounts.length"
                    label="Active accounts"
                  >
                    <option
                      v-for="account in activeAccounts"
                      :key="account.key"
                      :value="account.key"
                    >
                      {{ account.key }} · Active
                    </option>
                  </optgroup>

                  <optgroup
                    v-if="inactiveAccounts.length"
                    label="Inactive accounts — unavailable"
                  >
                    <option
                      v-for="account in inactiveAccounts"
                      :key="account.key"
                      :value="account.key"
                      disabled
                    >
                      {{ account.key }} · Inactive
                    </option>
                  </optgroup>
                </select>

                <span
                  v-if="selectedAccountInfo"
                  class="account-state-badge"
                  :class="selectedAccountInfo.is_active ? 'active' : 'inactive'"
                >
                  {{ selectedAccountInfo.is_active ? 'Active' : 'Inactive' }}
                </span>

                <span class="value-type">select</span>

                <span
                  v-if="accountsErrorMessage"
                  class="inline-error"
                >
                  {{ accountsErrorMessage }}
                </span>
              </div>
            </div>
          </div>

          <div
            v-if="configRows.length === 0"
            class="config-empty config-empty-after-special"
          >
            No additional configuration fields.
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
  color: #60a5fa;
  font-size: 13px;
  font-weight: 800;
  line-height: 1.35;
  white-space: normal;
  overflow-wrap: anywhere;
  word-break: break-word;
}

.strategy-card-content small {
  color: #9ca3af;
  font-size: 10px;
  font-weight: 700;
  line-height: 1.35;
  white-space: normal;
  overflow-wrap: anywhere;
  word-break: break-word;
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

.strategy-title-block {
  min-width: 0;
}

.strategy-title-line {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 5px;
}

.strategy-title-line p {
  margin: 0;
}

.running-state {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  min-height: 26px;
  padding: 0 9px;
  border: 1px solid #4b5563;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 800;
}

.running-state.running {
  color: #86efac;
  background: #15342b;
  border-color: #2f765a;
}

.running-state.stopping {
  color: #fbbf24;
  background: #3a3320;
  border-color: #8a6d1f;
}

.running-state-dot {
  width: 8px;
  height: 8px;
  border-radius: 999px;
  background: currentColor;
}

.strategy-control-button {
  min-width: 72px;
  height: 30px;
  padding: 0 14px;
  border-radius: 7px;
  font-size: 12px;
  font-weight: 900;
  cursor: pointer;
}

.start-button {
  color: #d1fae5;
  background: #166534;
  border: 1px solid #22c55e;
}

.stop-button {
  color: #fee2e2;
  background: #7f1d1d;
  border: 1px solid #ef4444;
}

.strategy-control-button:disabled {
  cursor: not-allowed;
  opacity: 0.55;
}

.control-error {
  margin-top: 7px !important;
  color: #fca5a5 !important;
  font-size: 11px !important;
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

.config-card-header p {
  font-size: 14px;
}

.config-card-header h2 {
  font-size: 17px;
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


.current-info-header-meta {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 8px;
}

.tree-action-button {
  min-height: 28px;
  padding: 0 10px;
  color: #bfdbfe;
  background: #172033;
  border: 1px solid #3b82f6;
  border-radius: 6px;
  cursor: pointer;
  font-size: 11px;
  font-weight: 900;
  transition: background 120ms ease, border-color 120ms ease, color 120ms ease;
}

.tree-action-button:hover {
  color: #ffffff;
  background: #1e3a5f;
  border-color: #60a5fa;
}

.tree-action-button.secondary {
  color: #cbd5e1;
  border-color: #4b5563;
}

.tree-action-button.secondary:hover {
  background: #273449;
  border-color: #6b7280;
}

.live-badge {
  min-height: 28px;
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 0 9px;
  color: #86efac;
  background: #15342b;
  border: 1px solid #2f765a;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 900;
}

.live-badge-dot {
  width: 7px;
  height: 7px;
  border-radius: 999px;
  background: #34d399;
}

.current-info-message {
  padding: 22px 18px;
  color: #9ca3af;
  font-size: 13px;
  font-weight: 700;
}

.current-info-inline-error {
  padding: 9px 18px;
  color: #fca5a5;
  background: #2b2024;
  border-bottom: 1px solid #7f1d1d;
  font-size: 11px;
  font-weight: 700;
}

.readonly-value-cell {
  min-height: 52px;
}

.readonly-value {
  width: min(100%, 520px);
  min-height: 34px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  padding: 6px 10px;
  color: #f8fafc;
  background: #172033;
  border: 1px solid #374151;
  border-radius: 6px;
  overflow-wrap: anywhere;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 14px;
  font-weight: 700;
  line-height: 1.4;
}

.readonly-number {
  color: #93c5fd;
}

.readonly-boolean {
  color: #c4b5fd;
}

.readonly-null {
  color: #9ca3af;
  font-style: italic;
}

.special-config-section {
  border-bottom: 1px solid #374151;
}

.special-config-row {
  display: grid;
  grid-template-columns: minmax(230px, 0.8fr) minmax(320px, 1.2fr);
  align-items: center;
  min-height: 62px;
  background: #1f2937;
  border-bottom: 1px solid #374151;
}

.special-config-row:last-child {
  border-bottom: 0;
}

.special-config-row:hover {
  background: #273449;
}

.special-config-label {
  display: flex;
  flex-direction: column;
  gap: 3px;
  padding: 10px 16px 10px 32px;
}

.special-key {
  color: #cbd5e1;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  font-size: 14px;
  font-weight: 900;
}

.special-description {
  color: #6b7280;
  font-size: 11px;
  font-weight: 700;
}

.special-config-control {
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 8px;
  padding: 10px 16px;
  border-left: 1px solid #374151;
}

.special-select {
  min-width: 280px;
}

.account-state-badge {
  display: inline-flex;
  align-items: center;
  min-height: 22px;
  padding: 0 7px;
  border-radius: 5px;
  font-size: 11px;
  font-weight: 800;
}

.account-state-badge.active {
  color: #86efac;
  background: #15342b;
  border: 1px solid #2f765a;
}

.account-state-badge.inactive {
  color: #fca5a5;
  background: #3a2020;
  border: 1px solid #7f1d1d;
}

.inactive-selection {
  color: #fca5a5;
  border-color: #7f1d1d;
}

.inline-error {
  width: 100%;
  color: #fca5a5;
  font-size: 11px;
  font-weight: 700;
}

.config-empty-after-special {
  margin: 14px;
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

.group-toggle-button {
  width: fit-content;
  padding: 0;
  color: inherit;
  background: transparent;
  border: 0;
  cursor: pointer;
  font: inherit;
  text-align: left;
}

.group-toggle-button:hover strong {
  color: #bfdbfe;
}

.collapsible-branch {
  display: inline-block;
  transform: rotate(0deg);
  transform-origin: center;
  transition: transform 120ms ease;
}

.collapsible-branch.collapsed {
  transform: rotate(-90deg);
}

.group-key strong {
  color: #ffffff;
  font-size: 15px;
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
  font-size: 11px;
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
  font-size: 14px;
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
  font-size: 14px;
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

  .value-row,
  .special-config-row {
    grid-template-columns: 1fr;
  }

  .special-config-control {
    padding-left: 32px;
    border-top: 1px solid #374151;
    border-left: 0;
  }

  .special-select {
    width: 100%;
    min-width: 0;
  }

  .config-value-cell {
    padding-left: calc(40px + (var(--depth) * var(--indent-size)));
    border-top: 1px solid #374151;
    border-left: 0;
  }
}
</style>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

type Instrument = {
  price_precision: number
  tick_size: number
  lot_size: number
  exchange_symbol: string
  symbol: string
  instrument_type: string
  exchange_id: string
}

type InstrumentListResponse = {
  error: boolean
  status_code: number
  msg: string
  instruments: Instrument[]
}

type InstrumentSubscribeResponse = {
  error: boolean
  status_code: number
  msg: string
  data: Instrument[]
}

type SubscribeResponse = {
  error: boolean
  status_code: number
  msg: string
  data?: Instrument[]
}

type ExchangeTab = {
  label: string
  value: string
  tone: 'all' | 'binance' | 'coinbase' | 'gemini' | 'default'
}

type SortKey =
  | 'symbol'
  | 'exchange_id'
  | 'instrument_type'
  | 'exchange_symbol'
  | 'lot_size'
  | 'tick_size'
  | 'price_precision'

type SortDirection = 'asc' | 'desc'

const loading = ref(false)
const subscribing = ref(false)
const errorMessage = ref('')
const subscribeMessage = ref('')
const instruments = ref<Instrument[]>([])
const subscribedInstruments = ref<Instrument[]>([])
const selectedInstrument = ref<Instrument | null>(null)
const activeExchange = ref('all')
const searchText = ref('')
const sortKey = ref<SortKey | null>(null)
const sortDirection = ref<SortDirection>('asc')

const exchangeTabs = computed<ExchangeTab[]>(() => {
  const exchanges = [...new Set(instruments.value.map((item) => item.exchange_id))]
    .filter(Boolean)
    .sort()

  return [
    { label: 'All', value: 'all', tone: 'all' },
    ...exchanges.map((exchange) => ({
      label: exchange,
      value: exchange,
      tone: exchangeTone(exchange),
    })),
  ]
})

const activeTabInfo = computed(() => {
  return exchangeTabs.value.find((tab) => tab.value === activeExchange.value) ?? exchangeTabs.value[0]
})

const filteredInstruments = computed(() => {
  const query = searchText.value.trim().toUpperCase()

  return instruments.value.filter((instrument) => {
    const exchangeMatched =
      activeExchange.value === 'all' || instrument.exchange_id === activeExchange.value

    if (!exchangeMatched) {
      return false
    }

    if (query.length === 0) {
      return true
    }

    return [
      instrument.symbol,
      instrument.exchange_symbol,
      instrument.exchange_id,
      instrument.instrument_type,
    ].some((value) => value.toUpperCase().includes(query))
  })
})

const sortedInstruments = computed(() => {
  if (!sortKey.value) {
    return filteredInstruments.value
  }

  return [...filteredInstruments.value].sort((left, right) => {
    const leftValue = getSortValue(left, sortKey.value!)
    const rightValue = getSortValue(right, sortKey.value!)
    const result = compareSortValues(leftValue, rightValue)

    return sortDirection.value === 'asc' ? result : -result
  })
})

const isDetailOpen = computed(() => selectedInstrument.value !== null)

const selectedIsSubscribed = computed(() => {
  if (!selectedInstrument.value) {
    return false
  }

  return isSubscribed(selectedInstrument.value)
})

function exchangeTone(exchange: string): ExchangeTab['tone'] {
  const normalized = exchange.toUpperCase()

  if (normalized === 'BINANCE') return 'binance'
  if (normalized === 'COINBASE') return 'coinbase'
  if (normalized === 'GEMINI') return 'gemini'

  return 'default'
}

function getExchangeCount(tab: ExchangeTab) {
  if (tab.value === 'all') {
    return instruments.value.length
  }

  return instruments.value.filter((instrument) => instrument.exchange_id === tab.value).length
}

function getSortValue(instrument: Instrument, key: SortKey): string | number {
  return instrument[key]
}

function compareSortValues(left: string | number, right: string | number) {
  const leftNumber = Number(left)
  const rightNumber = Number(right)

  if (Number.isFinite(leftNumber) && Number.isFinite(rightNumber)) {
    return leftNumber - rightNumber
  }

  return String(left).localeCompare(String(right), undefined, {
    numeric: true,
    sensitivity: 'base',
  })
}

function sortInstruments(key: SortKey) {
  if (sortKey.value === key) {
    sortDirection.value = sortDirection.value === 'asc' ? 'desc' : 'asc'
    return
  }

  sortKey.value = key
  sortDirection.value = 'asc'
}

function selectExchange(exchange: string) {
  activeExchange.value = exchange

  if (
    selectedInstrument.value &&
    !filteredInstruments.value.some((instrument) => isSameInstrument(instrument, selectedInstrument.value!))
  ) {
    selectedInstrument.value = null
  }
}

function selectInstrument(instrument: Instrument) {
  selectedInstrument.value = instrument
  subscribeMessage.value = ''
}

function closeDetail() {
  selectedInstrument.value = null
}

function isSameInstrument(left: Instrument, right: Instrument) {
  return left.exchange_id === right.exchange_id && left.symbol === right.symbol
}

function isSubscribed(instrument: Instrument) {
  return subscribedInstruments.value.some((item) => isSameInstrument(item, instrument))
}

function formatNumber(value: number, digits = 10) {
  if (!Number.isFinite(value)) {
    return '-'
  }

  return Number(value.toFixed(digits)).toString()
}

async function fetchInstruments() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/instrument_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: InstrumentListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch instrument list.'
      return
    }

    instruments.value = result.instruments ?? []

    if (
      selectedInstrument.value &&
      !instruments.value.some((instrument) => isSameInstrument(instrument, selectedInstrument.value!))
    ) {
      selectedInstrument.value = null
    }
  } catch (error) {
    console.error('Fetch instrument list error:', error)
    errorMessage.value = 'Fetch instrument list error.'
  } finally {
    loading.value = false
  }
}

async function fetchSubscribedInstruments() {
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
      console.error('Failed to fetch subscribed instruments:', result.msg)
      return
    }

    subscribedInstruments.value = result.data ?? []
  } catch (error) {
    console.error('Fetch subscribed instruments error:', error)
  }
}

async function subscribeInstrument() {
  if (!selectedInstrument.value || selectedIsSubscribed.value) {
    return
  }

  subscribing.value = true
  subscribeMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/instrument_subscribe`, {
      method: 'POST',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        instrument: selectedInstrument.value.symbol,
        symbol: selectedInstrument.value.symbol,
        exchange_id: selectedInstrument.value.exchange_id,
        instrument_type: selectedInstrument.value.instrument_type,
      }),
    })

    const result: SubscribeResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      subscribeMessage.value = result.msg || 'Failed to subscribe instrument.'
      return
    }

    subscribedInstruments.value.push(selectedInstrument.value)
    subscribeMessage.value = result.msg || 'Instrument subscribed.'
  } catch (error) {
    console.error('Subscribe instrument error:', error)
    subscribeMessage.value = 'Subscribe instrument error.'
  } finally {
    subscribing.value = false
  }
}

async function startInstrumentView() {
  await Promise.all([
    fetchInstruments(),
    fetchSubscribedInstruments(),
  ])
}

onMounted(() => {
  startInstrumentView()
})
</script>

<template>
  <main class="instruments-page">
    <section class="instruments-layout" :class="{ 'detail-open': isDetailOpen }">
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>Filters</h2>
          <span class="filter-total">{{ filteredInstruments.length }}</span>
        </div>

        <button
          v-for="tab in exchangeTabs"
          :key="tab.value"
          class="filter-card"
          :class="[`tone-${tab.tone}`, { active: activeExchange === tab.value }]"
          @click="selectExchange(tab.value)"
        >
          <span class="filter-content">
            <strong>{{ tab.label }}</strong>
            <small>{{ getExchangeCount(tab) }} instruments</small>
          </span>
        </button>
      </aside>

      <section class="instruments-panel">
        <div class="instruments-header">
          <div>
            <h1>Instruments</h1>
            <p>{{ activeTabInfo?.label ?? 'All' }} instruments</p>
          </div>

          <button
            class="refresh-button"
            :disabled="loading"
            @click="startInstrumentView"
          >
            ↻
          </button>
        </div>

        <div class="search-row">
          <input
            v-model="searchText"
            class="search-input"
            placeholder="Search symbol, exchange symbol, exchange, type..."
          >
        </div>

        <div v-if="loading" class="panel-message">
          Loading instruments...
        </div>

        <div v-else-if="errorMessage" class="panel-message error-message">
          {{ errorMessage }}
        </div>

        <div v-else class="instruments-table-card">
          <table v-if="filteredInstruments.length > 0">
            <thead>
              <tr>
                <th class="sortable-header" @click="sortInstruments('symbol')">Symbol</th>
                <th class="sortable-header" @click="sortInstruments('exchange_id')">Exchange</th>
                <th class="sortable-header" @click="sortInstruments('instrument_type')">Type</th>
                <th class="sortable-header" @click="sortInstruments('exchange_symbol')">Exchange Symbol</th>
                <th class="sortable-header" @click="sortInstruments('lot_size')">Lot Size</th>
                <th class="sortable-header" @click="sortInstruments('tick_size')">Tick Size</th>
                <th class="sortable-header" @click="sortInstruments('price_precision')" v-if="!isDetailOpen">Price Precision</th>
                <th v-if="!isDetailOpen">Subscribed</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="instrument in sortedInstruments"
                :key="`${instrument.exchange_id}-${instrument.symbol}`"
                class="instrument-row"
                :class="{ selected: selectedInstrument && isSameInstrument(selectedInstrument, instrument) }"
                @click="selectInstrument(instrument)"
              >
                <td>
                  <div class="instrument-cell">
                    <strong>{{ instrument.symbol }}</strong>
                    <small>
                      <span>{{ instrument.exchange_id }}</span>
                      <i>·</i>
                      {{ instrument.instrument_type }}
                    </small>
                  </div>
                </td>

                <td><span class="exchange-text">{{ instrument.exchange_id }}</span></td>
                <td><span class="type-badge">{{ instrument.instrument_type }}</span></td>
                <td class="mono-text">{{ instrument.exchange_symbol }}</td>
                <td>{{ instrument.lot_size }}</td>
                <td>{{ instrument.tick_size }}</td>
                <td v-if="!isDetailOpen">{{ formatNumber(instrument.price_precision) }}</td>
                <td v-if="!isDetailOpen">
                  <span
                    class="status-badge"
                    :class="isSubscribed(instrument) ? 'tone-filled' : 'tone-default'"
                  >
                    {{ isSubscribed(instrument) ? 'YES' : 'NO' }}
                  </span>
                </td>
              </tr>
            </tbody>
          </table>

          <div v-else class="empty-table">
            No instruments found.
          </div>
        </div>

        <div class="table-footer">
          Showing {{ filteredInstruments.length > 0 ? 1 : 0 }} to {{ filteredInstruments.length }} of
          {{ filteredInstruments.length }} instruments
        </div>
      </section>

      <aside v-if="selectedInstrument" class="detail-panel">
        <div class="instrument-detail">
          <div class="detail-header">
            <div>
              <h2>Instrument Details</h2>
              <p>
                <span class="mono-text">{{ selectedInstrument.symbol }}</span>
                <span class="type-badge">{{ selectedInstrument.instrument_type }}</span>
              </p>
            </div>

            <button class="detail-close" @click="closeDetail">
              ×
            </button>
          </div>

          <section class="detail-card">
            <h3>Instrument</h3>

            <div class="detail-row"><span>Symbol</span><strong>{{ selectedInstrument.symbol }}</strong></div>
            <div class="detail-row"><span>Exchange</span><strong class="exchange-text">{{ selectedInstrument.exchange_id }}</strong></div>
            <div class="detail-row"><span>Instrument Type</span><strong>{{ selectedInstrument.instrument_type }}</strong></div>
            <div class="detail-row"><span>Exchange Symbol</span><strong class="mono-text">{{ selectedInstrument.exchange_symbol }}</strong></div>
            <div class="detail-row"><span>Lot Size</span><strong>{{ selectedInstrument.lot_size }}</strong></div>
            <div class="detail-row"><span>Tick Size</span><strong>{{ selectedInstrument.tick_size }}</strong></div>
            <div class="detail-row"><span>Price Precision</span><strong>{{ formatNumber(selectedInstrument.price_precision) }}</strong></div>
          </section>

          <section class="detail-card">
            <h3>Subscription</h3>

            <div class="detail-row">
              <span>Status</span>
              <strong>
                <span
                  class="status-badge"
                  :class="selectedIsSubscribed ? 'tone-filled' : 'tone-default'"
                >
                  {{ selectedIsSubscribed ? 'SUBSCRIBED' : 'NOT SUBSCRIBED' }}
                </span>
              </strong>
            </div>

            <button
              class="subscribe-button"
              :disabled="selectedIsSubscribed || subscribing"
              @click="subscribeInstrument"
            >
              {{ selectedIsSubscribed ? 'Subscribed' : subscribing ? 'Subscribing...' : 'Subscribe' }}
            </button>

            <p v-if="subscribeMessage" class="subscribe-message">
              {{ subscribeMessage }}
            </p>
          </section>
        </div>
      </aside>
    </section>
  </main>
</template>

<style scoped>
.instruments-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.instruments-layout {
  display: grid;
  grid-template-columns: 220px minmax(860px, 1fr);
  gap: 12px;
}

.instruments-layout.detail-open {
  grid-template-columns: 220px minmax(860px, 1fr) 430px;
}

.filter-panel,
.instruments-panel,
.detail-panel {
  background: #111827;
  border: 1px solid #374151;
  border-radius: 12px;
  box-shadow: none;
}

.filter-panel {
  min-height: 720px;
  padding: 14px 12px;
}

.instruments-panel {
  min-height: 720px;
  padding: 22px;
}

.detail-panel {
  min-height: 720px;
  padding: 22px;
}

.filter-header,
.instruments-header,
.detail-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.filter-header {
  margin-bottom: 18px;
}

.filter-header h2,
.instruments-header h1,
.detail-header h2 {
  margin: 0;
  color: #ffffff;
  font-size: 18px;
  font-weight: 800;
  letter-spacing: -0.02em;
}

.filter-total {
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

.filter-card {
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

.filter-card:hover {
  background: #273449;
  border-color: #4b5563;
}

.filter-card.active {
  background: #1e3a5f;
  border-color: #3b82f6;
}

.filter-content {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.filter-content strong {
  font-size: 14px;
  font-weight: 800;
}

.filter-content small {
  color: #9ca3af;
  font-size: 12px;
}

.filter-card.tone-all .filter-content strong { color: #e5e7eb; }
.filter-card.tone-binance .filter-content strong { color: #facc15; }
.filter-card.tone-coinbase .filter-content strong { color: #60a5fa; }
.filter-card.tone-gemini .filter-content strong { color: #34d399; }
.filter-card.tone-default .filter-content strong { color: #c084fc; }

.instruments-header {
  margin-bottom: 18px;
}

.instruments-header p {
  margin: 5px 0 0;
  color: #9ca3af;
  font-size: 13px;
}

.search-row {
  margin-bottom: 18px;
}

.search-input {
  width: 100%;
  height: 42px;
  padding: 0 14px;
  color: #f8fafc;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 700;
  outline: none;
}

.search-input::placeholder {
  color: #6b7280;
}

.search-input:focus {
  border-color: #3b82f6;
  background: #273449;
}

.refresh-button,
.detail-close {
  width: 42px;
  height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
  font-size: 24px;
  cursor: pointer;
  transition: none;
}

.refresh-button:hover,
.detail-close:hover {
  color: #ffffff;
  background: #1f2937;
  border-color: #4b5563;
}

.refresh-button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.panel-message,
.empty-table {
  padding: 24px;
  color: #9ca3af;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
  font-size: 12px;
}

.error-message {
  color: #fca5a5;
  background: #1f2937;
  border-color: #7f1d1d;
}

.instruments-table-card {
  min-height: 420px;
  overflow: auto;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th,
td {
  padding: 5px 16px;
  border-bottom: 1px solid #374151;
  text-align: right;
  white-space: nowrap;
}

th:first-child,
td:first-child {
  text-align: left;
}

th {
  color: #9ca3af;
  background: #1f2937;
  font-size: 13px;
  font-weight: 800;
}

th.sortable-header {
  cursor: pointer;
  user-select: none;
  transition: none;
}

th.sortable-header:hover {
  color: #ffffff;
  background: #273449;
}

th.sortable-header:active {
  background: #1e3a5f;
}

td {
  color: #f8fafc;
  font-size: 13px;
}

.instrument-row {
  cursor: pointer;
  transition: none;
}

.instrument-row:hover {
  background: #273449;
}

.instrument-row.selected {
  background: #1e3a5f;
}

.instrument-cell {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.instrument-cell strong {
  color: #ffffff;
  font-size: 13px;
  font-weight: 900;
}

.instrument-cell small {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  color: #9ca3af;
  font-size: 11px;
  font-weight: 700;
}

.instrument-cell small span,
.exchange-text {
  color: #facc15;
}

.status-badge,
.type-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 46px;
  padding: 3px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.status-badge.tone-filled {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.status-badge.tone-default,
.type-badge {
  color: #e5e7eb;
  background: #111827;
  border: 1px solid #374151;
}

.mono-text {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
}

.table-footer {
  margin-top: 18px;
  color: #9ca3af;
  font-size: 13px;
}

.instrument-detail {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.detail-header {
  padding-bottom: 14px;
  border-bottom: 1px solid #374151;
}

.detail-header p {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 6px 0 0;
  color: #9ca3af;
  font-size: 13px;
}

.detail-card {
  padding: 14px 16px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
}

.detail-card h3 {
  margin: 0 0 14px;
  color: #ffffff;
  font-size: 15px;
  font-weight: 900;
}

.detail-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  min-height: 32px;
  padding: 3px 0;
}

.detail-row span {
  color: #9ca3af;
  font-size: 13px;
}

.detail-row strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
  text-align: right;
  word-break: break-all;
}

.subscribe-button {
  width: 100%;
  height: 42px;
  margin-top: 14px;
  color: #d1fae5;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 900;
  cursor: pointer;
}

.subscribe-button:hover:not(:disabled) {
  color: #ffffff;
  background: #245247;
}

.subscribe-button:disabled {
  color: #9ca3af;
  background: #111827;
  border-color: #374151;
  cursor: not-allowed;
  opacity: 0.75;
}

.subscribe-message {
  margin: 10px 0 0;
  color: #9ca3af;
  font-size: 12px;
}

@media (max-width: 1500px) {
  .instruments-layout {
    grid-template-columns: 260px minmax(620px, 1fr);
  }

  .detail-panel {
    grid-column: 1 / -1;
  }
}

@media (max-width: 1050px) {
  .instruments-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .instruments-panel,
  .detail-panel {
    min-height: unset;
  }
}
</style>

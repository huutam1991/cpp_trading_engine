<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

let positionPollTimer: ReturnType<typeof window.setInterval> | null = null
let isFetchingPositions = false

type Instrument = {
  price_precision: number
  tick_size: number
  lot_size: number
  exchange_symbol: string
  symbol: string
  instrument_type: string
  exchange_id: string
}

type Position = {
  side: string
  position_amt: number
  mark_price: string | number
  entry_price: string | number
  pnl: string | number
  instrument: Instrument
}

type PositionListResponse = {
  error: boolean
  status_code: number
  msg: string
  positions: Position[]
}

type PositionTab = {
  label: string
  value: string
  count: number
  tone: 'all' | 'exchange'
}

type SortKey =
  | 'instrument'
  | 'side'
  | 'position_amt'
  | 'entry_price'
  | 'mark_price'
  | 'pnl'

type SortDirection = 'asc' | 'desc'

const loading = ref(false)
const errorMessage = ref('')
const positions = ref<Position[]>([])
const selectedPosition = ref<Position | null>(null)
const activeTab = ref('all')
const sortKey = ref<SortKey | null>(null)
const sortDirection = ref<SortDirection>('asc')

const tabs = computed<PositionTab[]>(() => {
  const exchangeIds = Array.from(
    new Set(
      positions.value
        .map((position) => position.instrument.exchange_id)
        .filter((exchangeId): exchangeId is string => Boolean(exchangeId)),
    ),
  ).sort((left, right) => left.localeCompare(right))

  return [
    {
      label: 'All',
      value: 'all',
      count: positions.value.length,
      tone: 'all',
    },
    ...exchangeIds.map((exchangeId) => ({
      label: exchangeId,
      value: exchangeId,
      count: positions.value.filter(
        (position) => position.instrument.exchange_id === exchangeId,
      ).length,
      tone: 'exchange' as const,
    })),
  ]
})

const filteredPositions = computed(() => {
  if (activeTab.value === 'all') {
    return positions.value
  }

  return positions.value.filter(
    (position) => position.instrument.exchange_id === activeTab.value,
  )
})

const sortedPositions = computed(() => {
  if (!sortKey.value) {
    return filteredPositions.value
  }

  return [...filteredPositions.value].sort((left, right) => {
    const leftValue = getSortValue(left, sortKey.value!)
    const rightValue = getSortValue(right, sortKey.value!)

    const result = compareSortValues(leftValue, rightValue)
    return sortDirection.value === 'asc' ? result : -result
  })
})

const isDetailOpen = computed(() => selectedPosition.value !== null)

const activeTabInfo = computed<PositionTab>(() => {
  return tabs.value.find((tab) => tab.value === activeTab.value) ?? tabs.value[0] ?? {
    label: 'All',
    value: 'all',
    count: 0,
    tone: 'all',
  }
})

const totalPnl = computed(() => {
  return positions.value.reduce((sum, position) => sum + toNumber(position.pnl), 0)
})

function getPositionKey(position: Position) {
  return `${position.instrument.exchange_id}:${position.instrument.symbol}:${position.side}`
}

function getSortValue(position: Position, key: SortKey): string | number {
  switch (key) {
    case 'instrument':
      return position.instrument.symbol
    case 'side':
      return position.side
    case 'position_amt':
      return toNumber(position.position_amt)
    case 'entry_price':
      return toNumber(position.entry_price)
    case 'mark_price':
      return toNumber(position.mark_price)
    case 'pnl':
      return toNumber(position.pnl)
  }
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

function sortPositions(key: SortKey) {
  if (sortKey.value === key) {
    sortDirection.value = sortDirection.value === 'asc' ? 'desc' : 'asc'
    return
  }

  sortKey.value = key
  sortDirection.value = 'asc'
}

async function fetchPositions(showLoading = false) {
  if (isFetchingPositions) {
    return
  }

  isFetchingPositions = true

  if (showLoading) {
    loading.value = true
  }

  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/position_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: PositionListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch position list.'
      return
    }

    positions.value = result.positions ?? []

    if (
      activeTab.value !== 'all' &&
      !positions.value.some(
        (position) => position.instrument.exchange_id === activeTab.value,
      )
    ) {
      activeTab.value = 'all'
    }

    if (
      selectedPosition.value &&
      !positions.value.some(
        (position) => getPositionKey(position) === getPositionKey(selectedPosition.value!),
      )
    ) {
      selectedPosition.value = null
    }

    if (selectedPosition.value) {
      const updatedPosition = positions.value.find(
        (position) => getPositionKey(position) === getPositionKey(selectedPosition.value!),
      )

      if (updatedPosition) {
        selectedPosition.value = updatedPosition
      }
    }
  } catch (error) {
    console.error('Fetch position list error:', error)
    errorMessage.value = 'Fetch position list error.'
  } finally {
    loading.value = false
    isFetchingPositions = false
  }
}

function startPositionPolling() {
  stopPositionPolling()

  fetchPositions(true)

  positionPollTimer = window.setInterval(() => {
    fetchPositions(false)
  }, 1000)
}

function stopPositionPolling() {
  if (positionPollTimer) {
    window.clearInterval(positionPollTimer)
    positionPollTimer = null
  }
}

function refreshPositions() {
  fetchPositions(true)
}

function selectTab(tab: string) {
  activeTab.value = tab

  if (
    selectedPosition.value &&
    !filteredPositions.value.some(
      (position) => getPositionKey(position) === getPositionKey(selectedPosition.value!),
    )
  ) {
    selectedPosition.value = null
  }
}

function selectPosition(position: Position) {
  selectedPosition.value = position
}

function closeDetail() {
  selectedPosition.value = null
}

function toNumber(value: string | number) {
  const numberValue = Number(value)
  return Number.isFinite(numberValue) ? numberValue : 0
}

function formatNumber(value: string | number, digits = 8) {
  const numberValue = Number(value)

  if (!Number.isFinite(numberValue)) {
    return '-'
  }

  return Number(numberValue.toFixed(digits)).toString()
}

function formatPrice(value: string | number) {
  const numberValue = Number(value)

  if (!Number.isFinite(numberValue)) {
    return '-'
  }

  return numberValue.toLocaleString(undefined, {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  })
}

function formatPnl(value: string | number) {
  const numberValue = Number(value)

  if (!Number.isFinite(numberValue)) {
    return '-'
  }

  return numberValue.toLocaleString(undefined, {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  })
}

function sideClass(side: string) {
  return side.toUpperCase() === 'LONG' ? 'long-text' : 'short-text'
}

function pnlClass(pnl: string | number) {
  const pnlValue = toNumber(pnl)

  if (pnlValue > 0) return 'profit-text'
  if (pnlValue < 0) return 'loss-text'

  return 'flat-text'
}

onMounted(() => {
  startPositionPolling()
})

onBeforeUnmount(() => {
  stopPositionPolling()
})
</script>

<template>
  <main class="positions-page">
    <section class="positions-layout" :class="{ 'detail-open': isDetailOpen }">
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>Filters</h2>
          <span class="filter-total">{{ filteredPositions.length }}</span>
        </div>

        <button
          v-for="tab in tabs"
          :key="tab.value"
          class="filter-card"
          :class="[`tone-${tab.tone}`, { active: activeTab === tab.value }]"
          @click="selectTab(tab.value)"
        >
          <span class="filter-content">
            <strong>{{ tab.label }}</strong>
            <small>{{ tab.count }} positions</small>
          </span>
        </button>
      </aside>

      <section class="positions-panel">
        <div class="positions-header">
          <div>
            <h1>Positions</h1>
            <p>{{ activeTabInfo.label }} positions</p>
          </div>

          <button
            class="refresh-button"
            :disabled="loading"
            @click="refreshPositions"
          >
            ↻
          </button>
        </div>

        <div
          v-if="loading"
          class="panel-message"
        >
          Loading positions...
        </div>

        <div
          v-else-if="errorMessage"
          class="panel-message error-message"
        >
          {{ errorMessage }}
        </div>

        <div
          v-else
          class="positions-table-card"
        >
          <table v-if="filteredPositions.length > 0">
            <thead>
              <tr>
                <th class="sortable-header" @click="sortPositions('instrument')">Instrument</th>
                <th class="sortable-header" @click="sortPositions('side')">Side</th>
                <th class="sortable-header" @click="sortPositions('position_amt')">Position Amt</th>
                <th class="sortable-header" @click="sortPositions('entry_price')">Entry Price</th>
                <th class="sortable-header" @click="sortPositions('mark_price')">Mark Price</th>
                <th class="sortable-header" @click="sortPositions('pnl')">PnL</th>
                <th v-if="!isDetailOpen">Exchange Symbol</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="position in sortedPositions"
                :key="getPositionKey(position)"
                class="position-row"
                :class="{
                  selected: selectedPosition && getPositionKey(selectedPosition) === getPositionKey(position),
                  long: position.side.toUpperCase() === 'LONG',
                  short: position.side.toUpperCase() === 'SHORT',
                }"
                @click="selectPosition(position)"
              >
                <td>
                  <div class="instrument-cell">
                    <strong>{{ position.instrument.symbol }}</strong>
                    <small>
                      <span>{{ position.instrument.exchange_id }}</span>
                      <i>·</i>
                      {{ position.instrument.instrument_type }}
                    </small>
                  </div>
                </td>

                <td>
                  <span
                    class="side-badge"
                    :class="sideClass(position.side)"
                  >
                    {{ position.side }}
                  </span>
                </td>

                <td>{{ formatNumber(position.position_amt) }}</td>
                <td>{{ formatPrice(position.entry_price) }}</td>
                <td>{{ formatPrice(position.mark_price) }}</td>

                <td>
                  <span
                    class="pnl-badge"
                    :class="pnlClass(position.pnl)"
                  >
                    {{ formatPnl(position.pnl) }}
                  </span>
                </td>

                <td v-if="!isDetailOpen" class="mono-text">
                  {{ position.instrument.exchange_symbol }}
                </td>
              </tr>
            </tbody>
          </table>

          <div
            v-else
            class="empty-table"
          >
            No positions found.
          </div>
        </div>

        <div class="table-footer">
          Showing {{ filteredPositions.length > 0 ? 1 : 0 }} to {{ filteredPositions.length }} of
          {{ filteredPositions.length }} positions
          <span class="footer-pnl" :class="pnlClass(totalPnl)">
            Total PnL: {{ formatPnl(totalPnl) }}
          </span>
        </div>
      </section>

      <aside v-if="selectedPosition" class="detail-panel">
        <div
          v-if="!selectedPosition"
          class="empty-detail"
        >
          <h2>Position Details</h2>
          <p>Click a position row to inspect all fields.</p>
        </div>

        <div
          v-else
          class="position-detail"
        >
          <div class="detail-header">
            <div>
              <h2>Position Details</h2>
              <p>
                <span class="mono-text">{{ selectedPosition.instrument.symbol }}</span>
                <span
                  class="side-badge"
                  :class="sideClass(selectedPosition.side)"
                >
                  {{ selectedPosition.side }}
                </span>
              </p>
            </div>

            <button
              class="detail-close"
              @click="closeDetail"
            >
              ×
            </button>
          </div>

          <section class="detail-card">
            <h3>Position Info</h3>

            <div class="detail-row">
              <span>Side</span>
              <strong>
                <span
                  class="side-badge"
                  :class="sideClass(selectedPosition.side)"
                >
                  {{ selectedPosition.side }}
                </span>
              </strong>
            </div>

            <div class="detail-row"><span>Position Amt</span><strong>{{ formatNumber(selectedPosition.position_amt) }}</strong></div>
            <div class="detail-row"><span>Entry Price</span><strong>{{ formatPrice(selectedPosition.entry_price) }}</strong></div>
            <div class="detail-row"><span>Mark Price</span><strong>{{ formatPrice(selectedPosition.mark_price) }}</strong></div>

            <div class="detail-row">
              <span>PnL</span>
              <strong>
                <span
                  class="pnl-badge"
                  :class="pnlClass(selectedPosition.pnl)"
                >
                  {{ formatPnl(selectedPosition.pnl) }}
                </span>
              </strong>
            </div>
          </section>

          <section class="detail-card">
            <h3>Instrument</h3>

            <div class="detail-row"><span>Symbol</span><strong>{{ selectedPosition.instrument.symbol }}</strong></div>
            <div class="detail-row"><span>Exchange</span><strong class="exchange-text">{{ selectedPosition.instrument.exchange_id }}</strong></div>
            <div class="detail-row"><span>Instrument Type</span><strong>{{ selectedPosition.instrument.instrument_type }}</strong></div>
            <div class="detail-row"><span>Exchange Symbol</span><strong>{{ selectedPosition.instrument.exchange_symbol }}</strong></div>
            <div class="detail-row"><span>Lot Size</span><strong>{{ selectedPosition.instrument.lot_size }}</strong></div>
            <div class="detail-row"><span>Tick Size</span><strong>{{ selectedPosition.instrument.tick_size }}</strong></div>
            <div class="detail-row"><span>Price Precision</span><strong>{{ selectedPosition.instrument.price_precision }}</strong></div>
          </section>
        </div>
      </aside>
    </section>
  </main>
</template>

<style scoped>
.positions-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.positions-layout {
  display: grid;
  grid-template-columns: 220px minmax(860px, 1fr);
  gap: 12px;
}

.positions-layout.detail-open {
  grid-template-columns: 220px minmax(860px, 1fr) 430px;
}

.filter-panel,
.positions-panel,
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

.positions-panel {
  min-height: 720px;
  padding: 22px;
}

.detail-panel {
  min-height: 720px;
  padding: 22px;
}

.filter-header,
.positions-header,
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
.positions-header h1,
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

.tone-all .filter-icon,
.filter-card.tone-all .filter-content strong {
  color: #e5e7eb;
}

.tone-exchange .filter-icon,
.filter-card.tone-exchange .filter-content strong {
  color: #facc15;
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

.positions-header {
  margin-bottom: 18px;
}

.positions-header p {
  margin: 5px 0 0;
  color: #9ca3af;
  font-size: 13px;
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

.positions-table-card {
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

.position-row {
  cursor: pointer;
  transition: none;
}

.position-row:hover {
  background: #273449;
}

.position-row.selected {
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

.side-badge,
.pnl-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 46px;
  padding: 3px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.long-text,
.profit-text {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.short-text,
.loss-text {
  color: #f87171;
  background: #3b2b2b;
  border: 1px solid #7f1d1d;
}

.flat-text {
  color: #cbd5e1;
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

.footer-pnl {
  margin-left: 16px;
  padding: 3px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.empty-detail {
  min-height: 660px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: #9ca3af;
  text-align: center;
}

.empty-detail h2 {
  margin: 0 0 10px;
  color: #ffffff;
  font-size: 18px;
  font-weight: 800;
}

.empty-detail p {
  max-width: 280px;
  margin: 0;
  line-height: 1.5;
}

.position-detail {
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

.detail-row span.long-text,
.detail-row span.profit-text {
  color: #34d399;
}

.detail-row span.short-text,
.detail-row span.loss-text {
  color: #f87171;
}

.detail-row strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
  text-align: right;
  word-break: break-all;
}

@media (max-width: 1500px) {
  .positions-layout {
    grid-template-columns: 260px minmax(620px, 1fr);
  }

  .detail-panel {
    grid-column: 1 / -1;
  }
}

@media (max-width: 1050px) {
  .positions-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .positions-panel,
  .detail-panel {
    min-height: unset;
  }
}
</style>

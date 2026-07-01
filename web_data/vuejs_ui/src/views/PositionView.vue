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
  account: string
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

const loading = ref(false)
const errorMessage = ref('')
const positions = ref<Position[]>([])
const activeTab = ref('all')

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

const activeTabInfo = computed<PositionTab>(() => {
  return tabs.value.find((tab) => tab.value === activeTab.value) ?? tabs.value[0] ?? {
    label: 'All',
    value: 'all',
    count: 0,
    tone: 'all',
  }
})

const totalPnl = computed(() => {
  return filteredPositions.value.reduce((sum, position) => sum + toNumber(position.pnl), 0)
})

function getPositionKey(position: Position) {
  return `${position.account}:${position.instrument.exchange_id}:${position.instrument.symbol}:${position.side}`
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
    <section class="positions-layout">
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
          v-else-if="filteredPositions.length === 0"
          class="empty-table"
        >
          No positions found.
        </div>

        <div v-else class="position-card-grid">
          <article
            v-for="position in filteredPositions"
            :key="getPositionKey(position)"
            class="position-card"
          >
            <header class="position-card-header">
              <div class="instrument-title">
                <strong>{{ position.instrument.symbol }}</strong>
                <small class="position-meta">
                  <span class="exchange-text">{{ position.instrument.exchange_id }}</span>
                  <i>·</i>
                  <span class="account-badge">{{ position.account }}</span>
                </small>
              </div>

              <span
                class="side-badge"
                :class="sideClass(position.side)"
              >
                {{ position.side }}
              </span>
            </header>

            <section class="position-metrics">
              <div class="metric-row">
                <span>Position</span>
                <strong>{{ formatNumber(position.position_amt) }}</strong>
              </div>

              <div class="metric-row">
                <span>PnL</span>
                <strong
                  class="pnl-badge"
                  :class="pnlClass(position.pnl)"
                >
                  {{ formatPnl(position.pnl) }}
                </strong>
              </div>

              <div class="metric-row">
                <span>Entry</span>
                <strong>{{ formatPrice(position.entry_price) }}</strong>
              </div>

              <div class="metric-row">
                <span>Mark</span>
                <strong>{{ formatPrice(position.mark_price) }}</strong>
              </div>
            </section>

          </article>
        </div>

        <div class="table-footer">
          Showing {{ filteredPositions.length > 0 ? 1 : 0 }} to {{ filteredPositions.length }} of
          {{ filteredPositions.length }} positions
          <span class="footer-pnl" :class="pnlClass(totalPnl)">
            Total PnL: {{ formatPnl(totalPnl) }}
          </span>
        </div>
      </section>
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

.filter-panel,
.positions-panel {
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

.filter-header,
.positions-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.filter-header {
  margin-bottom: 18px;
}

.filter-header h2,
.positions-header h1 {
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

.filter-card.tone-all .filter-content strong {
  color: #e5e7eb;
}

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

.refresh-button {
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

.refresh-button:hover {
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

.position-card-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.position-card {
  min-height: unset;
  padding: 16px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 12px;
}

.position-card-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  padding-bottom: 12px;
  border-bottom: 1px solid #374151;
}

.instrument-title {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.instrument-title strong {
  color: #ffffff;
  font-size: 15px;
  font-weight: 900;
}

.instrument-title small {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  color: #9ca3af;
  font-size: 11px;
  font-weight: 700;
}

.position-meta {
  flex-wrap: wrap;
}

.exchange-text {
  color: #facc15;
}

.account-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: fit-content;
  padding: 3px 9px;
  color: #93c5fd;
  background: rgba(96, 165, 250, 0.12);
  border: 1px solid rgba(96, 165, 250, 0.35);
  border-radius: 999px;
  font-size: 11px;
  font-weight: 900;
  line-height: 1.1;
  letter-spacing: 0.04em;
}

.account-badge.compact {
  padding: 2px 8px;
  font-size: 10px;
}

.side-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 52px;
  padding: 4px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.position-metrics {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
  margin-top: 12px;
}

.metric-row {
  min-height: 40px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 8px 10px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 9px;
}

.metric-row span {
  color: #9ca3af;
  font-size: 11px;
  font-weight: 700;
}

.metric-row strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 900;
  text-align: right;
}

.pnl-badge,
.footer-pnl {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: fit-content;
  padding: 6px 12px;
  border-radius: 8px;
  font-size: 13px !important;
  font-weight: 800;
}

.instrument-strip {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 7px;
  margin-top: 12px;
  padding-top: 12px;
  color: #cbd5e1;
  border-top: 1px solid #374151;
  font-size: 11px;
  font-weight: 800;
}

.instrument-strip i {
  color: #64748b;
  font-style: normal;
}

.long-text {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.short-text {
  color: #f87171;
  background: #3b2b2b;
  border: 1px solid #7f1d1d;
}

.profit-text {
    display: inline-flex;
    align-items: center;
    justify-content: center;

    padding: 4px 10px;

    border-radius: 8px;

    font-size: 13px;
    font-weight: 700;

    color: #22c55e !important;;

    background: rgba(34, 197, 94, 0.18);

    border: 1px solid rgba(34, 197, 94, 0.35);
}

.loss-text {
    display: inline-flex;
    align-items: center;
    justify-content: center;

    padding: 4px 10px;

    border-radius: 8px;

    font-size: 13px;
    font-weight: 700;

    color: #f87171 !important;

    background: rgba(239, 68, 68, 0.12);
    border: 1px solid rgba(239, 68, 68, 0.25);
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
  font-size: 11px !important;
}

@media (max-width: 1300px) {
  .position-card-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 1050px) {
  .positions-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .positions-panel {
    min-height: unset;
  }
}

@media (max-width: 700px) {
  .position-metrics {
    grid-template-columns: 1fr;
  }
}
</style>

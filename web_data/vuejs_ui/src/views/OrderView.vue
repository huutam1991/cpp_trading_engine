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

type OrderError = {
  message: string
  code: number
}

type Order = {
  commission_asset: string
  volumn_in_quote_currency: number
  instrument: Instrument
  error: OrderError
  order_id: number | string
  fee: number
  status: string
  type: string
  price: number
  quantity: number
  output_quantity: number
  side: string
  filled_quantity: number
  output_asset: string
}

type OrderListResponse = {
  error: boolean
  status_code: number
  msg: string
  orders: Order[]
}

type OrderTab = {
  label: string
  value: string
  statuses: string[]
  icon: string
  tone: 'all' | 'open' | 'partial' | 'filled' | 'cancel' | 'reject'
}

const loading = ref(false)
const errorMessage = ref('')
const orders = ref<Order[]>([])
const selectedOrder = ref<Order | null>(null)
const activeTab = ref('all')

const tabs: OrderTab[] = [
  { label: 'All', value: 'all', statuses: [], icon: '◇', tone: 'all' },
  { label: 'Open', value: 'open', statuses: ['NEW', 'OPEN'], icon: '↻', tone: 'open' },
  {
    label: 'Partially Filled',
    value: 'partially_filled',
    statuses: ['PARTIALLY_FILLED', 'PARTIAL_FILLED'],
    icon: '◔',
    tone: 'partial',
  },
  { label: 'Filled', value: 'filled', statuses: ['FILLED'], icon: '✓', tone: 'filled' },
  { label: 'Cancel', value: 'cancel', statuses: ['CANCELED', 'CANCELLED', 'CANCEL'], icon: '×', tone: 'cancel' },
  { label: 'Reject', value: 'reject', statuses: ['REJECTED', 'REJECT'], icon: '⊘', tone: 'reject' },
]

const filteredOrders = computed(() => {
  const tab = tabs.find((item) => item.value === activeTab.value)

  if (!tab || tab.statuses.length === 0) {
    return orders.value
  }

  return orders.value.filter((order) => tab.statuses.includes(order.status.toUpperCase()))
})

const activeTabInfo = computed<OrderTab>(() => {
  return tabs.find((tab) => tab.value === activeTab.value) ?? tabs[0]!
})

function getTabCount(tab: OrderTab) {
  if (tab.value === 'all') {
    return orders.value.length
  }

  return orders.value.filter((order) => tab.statuses.includes(order.status.toUpperCase())).length
}

async function fetchOrders() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/order_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: OrderListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch order list.'
      return
    }

    orders.value = result.orders ?? []

    if (
      selectedOrder.value &&
      !orders.value.some((order) => String(order.order_id) === String(selectedOrder.value?.order_id))
    ) {
      selectedOrder.value = null
    }
  } catch (error) {
    console.error('Fetch order list error:', error)
    errorMessage.value = 'Fetch order list error.'
  } finally {
    loading.value = false
  }
}

function selectTab(tab: string) {
  activeTab.value = tab

  if (
    selectedOrder.value &&
    !filteredOrders.value.some((order) => String(order.order_id) === String(selectedOrder.value?.order_id))
  ) {
    selectedOrder.value = null
  }
}

function selectOrder(order: Order) {
  selectedOrder.value = order
}

function closeDetail() {
  selectedOrder.value = null
}

function normalizeStatus(status: string) {
  return status.toUpperCase()
}

function formatNumber(value: number, digits = 8) {
  if (!Number.isFinite(value)) {
    return '-'
  }

  return Number(value.toFixed(digits)).toString()
}

function formatPrice(value: number) {
  if (!Number.isFinite(value)) {
    return '-'
  }

  return value.toLocaleString(undefined, {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  })
}

function statusTone(status: string): OrderTab['tone'] | 'default' {
  const normalized = normalizeStatus(status)

  if (['NEW', 'OPEN'].includes(normalized)) return 'open'
  if (['PARTIALLY_FILLED', 'PARTIAL_FILLED'].includes(normalized)) return 'partial'
  if (normalized === 'FILLED') return 'filled'
  if (['CANCELED', 'CANCELLED', 'CANCEL'].includes(normalized)) return 'cancel'
  if (['REJECTED', 'REJECT'].includes(normalized)) return 'reject'

  return 'default'
}

function sideClass(side: string) {
  return side.toUpperCase() === 'BUY' ? 'buy-text' : 'sell-text'
}

onMounted(() => {
  fetchOrders()
})
</script>

<template>
  <main class="orders-page">
    <section class="orders-layout">
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>Filters</h2>
          <span class="filter-total">{{ filteredOrders.length }}</span>
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
            <small>{{ getTabCount(tab) }} orders</small>
          </span>
        </button>
      </aside>

      <section class="orders-panel">
        <div class="orders-header">
          <div>
            <h1>Orders</h1>
            <p>{{ activeTabInfo.label }} orders</p>
          </div>

          <button
            class="refresh-button"
            :disabled="loading"
            @click="fetchOrders"
          >
            ↻
          </button>
        </div>

        <div
          v-if="loading"
          class="panel-message"
        >
          Loading orders...
        </div>

        <div
          v-else-if="errorMessage"
          class="panel-message error-message"
        >
          {{ errorMessage }}
        </div>

        <div
          v-else
          class="orders-table-card"
        >
          <table v-if="filteredOrders.length > 0">
            <thead>
              <tr>
                <th>Instrument</th>
                <th>Status</th>
                <th>Order ID</th>
                <th>Side</th>
                <th>Type</th>
                <th>Price</th>
                <th>Quantity</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="order in filteredOrders"
                :key="order.order_id"
                class="order-row"
                :class="{
                  selected: String(selectedOrder?.order_id) === String(order.order_id),
                  buy: order.side.toUpperCase() === 'BUY',
                  sell: order.side.toUpperCase() === 'SELL',
                }"
                @click="selectOrder(order)"
              >
                <td>
                  <div class="instrument-cell">
                    <strong>{{ order.instrument.symbol }}</strong>
                    <small>
                      <span>{{ order.instrument.exchange_id }}</span>
                      <i>·</i>
                      {{ order.instrument.instrument_type }}
                    </small>
                  </div>
                </td>

                <td>
                  <span
                    class="status-badge"
                    :class="`tone-${statusTone(order.status)}`"
                  >
                    {{ order.status }}
                  </span>
                </td>

                <td class="mono-text">{{ order.order_id }}</td>

                <td>
                  <span
                    class="side-badge"
                    :class="sideClass(order.side)"
                  >
                    {{ order.side }}
                  </span>
                </td>

                <td>
                  <span class="type-badge">{{ order.type }}</span>
                </td>

                <td>{{ formatPrice(order.price) }}</td>
                <td>{{ formatNumber(order.quantity) }}</td>
              </tr>
            </tbody>
          </table>

          <div
            v-else
            class="empty-table"
          >
            No orders found.
          </div>
        </div>

        <div class="table-footer">
          Showing {{ filteredOrders.length > 0 ? 1 : 0 }} to {{ filteredOrders.length }} of
          {{ filteredOrders.length }} orders
        </div>
      </section>

      <aside class="detail-panel">
        <div
          v-if="!selectedOrder"
          class="empty-detail"
        >
          <h2>Order Details</h2>
          <p>Click an order row to inspect all fields.</p>
        </div>

        <div
          v-else
          class="order-detail"
        >
          <div class="detail-header">
            <div>
              <h2>Order Details</h2>
              <p>
                <span class="mono-text">#{{ selectedOrder.order_id }}</span>
                <span
                  class="side-badge"
                  :class="sideClass(selectedOrder.side)"
                >
                  {{ selectedOrder.side }}
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
            <h3>Order Info</h3>

            <div class="detail-row">
              <span>Order ID</span>
              <strong class="mono-text">{{ selectedOrder.order_id }}</strong>
            </div>

            <div class="detail-row">
              <span>Status</span>
              <strong>
                <span
                  class="status-badge"
                  :class="`tone-${statusTone(selectedOrder.status)}`"
                >
                  {{ selectedOrder.status }}
                </span>
              </strong>
            </div>

            <div class="detail-row">
              <span>Side</span>
              <strong>
                <span
                  class="side-badge"
                  :class="sideClass(selectedOrder.side)"
                >
                  {{ selectedOrder.side }}
                </span>
              </strong>
            </div>

            <div class="detail-row"><span>Type</span><strong>{{ selectedOrder.type }}</strong></div>
            <div class="detail-row"><span>Price</span><strong>{{ formatPrice(selectedOrder.price) }}</strong></div>
            <div class="detail-row"><span>Quantity</span><strong>{{ formatNumber(selectedOrder.quantity) }}</strong></div>
            <div class="detail-row"><span>Filled Quantity</span><strong>{{ formatNumber(selectedOrder.filled_quantity) }}</strong></div>
            <div class="detail-row"><span>Output Quantity</span><strong>{{ formatNumber(selectedOrder.output_quantity) }}</strong></div>
            <div class="detail-row"><span>Volume in Quote Currency</span><strong>{{ formatNumber(selectedOrder.volumn_in_quote_currency) }}</strong></div>
            <div class="detail-row"><span>Fee</span><strong>{{ formatNumber(selectedOrder.fee) }}</strong></div>
            <div class="detail-row"><span>Commission Asset</span><strong>{{ selectedOrder.commission_asset || '–' }}</strong></div>
            <div class="detail-row"><span>Output Asset</span><strong>{{ selectedOrder.output_asset || '–' }}</strong></div>
          </section>

          <section class="detail-card">
            <h3>Instrument</h3>

            <div class="detail-row"><span>Symbol</span><strong>{{ selectedOrder.instrument.symbol }}</strong></div>
            <div class="detail-row"><span>Exchange</span><strong class="exchange-text">{{ selectedOrder.instrument.exchange_id }}</strong></div>
            <div class="detail-row"><span>Instrument Type</span><strong>{{ selectedOrder.instrument.instrument_type }}</strong></div>
            <div class="detail-row"><span>Exchange Symbol</span><strong>{{ selectedOrder.instrument.exchange_symbol }}</strong></div>
            <div class="detail-row"><span>Lot Size</span><strong>{{ selectedOrder.instrument.lot_size }}</strong></div>
            <div class="detail-row"><span>Tick Size</span><strong>{{ selectedOrder.instrument.tick_size }}</strong></div>
            <div class="detail-row"><span>Price Precision</span><strong>{{ selectedOrder.instrument.price_precision }}</strong></div>
          </section>

          <section class="detail-card">
            <h3>Error</h3>

            <div class="detail-row"><span>Code</span><strong>{{ selectedOrder.error.code }}</strong></div>
            <div class="detail-row"><span>Message</span><strong>{{ selectedOrder.error.message || '–' }}</strong></div>
          </section>
        </div>
      </aside>
    </section>
  </main>
</template>

<style scoped>
.orders-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.orders-layout {
  display: grid;
  grid-template-columns: 236px minmax(620px, 1fr) 390px;
  gap: 10px;
}

.filter-panel,
.orders-panel,
.detail-panel {
  background:
    radial-gradient(circle at top left, rgba(37, 99, 235, 0.08), transparent 36%),
    linear-gradient(145deg, #0b1020, #111827 62%, #0b1120);
  border: 1px solid rgba(71, 85, 105, 0.65);
  border-radius: 12px;
  box-shadow: 0 22px 60px rgba(0, 0, 0, 0.32);
}

.filter-panel {
  min-height: 620px;
  padding: 16px 14px;
}

.filter-header,
.orders-header,
.detail-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.filter-header {
  margin-bottom: 14px;
}

.filter-header h2,
.orders-header h1,
.detail-header h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 800;
  letter-spacing: -0.02em;
}

.filter-total {
  min-width: 40px;
  height: 40px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  color: #bfdbfe;
  background: rgba(37, 99, 235, 0.24);
  border: 1px solid rgba(96, 165, 250, 0.65);
  border-radius: 999px;
  font-weight: 800;
}

.filter-card {
  width: 100%;
  min-height: 74px;
  display: flex;
  align-items: center;
  gap: 13px;
  padding: 13px 16px;
  margin-bottom: 10px;
  color: #e5e7eb;
  background: linear-gradient(135deg, rgba(31, 41, 55, 0.92), rgba(17, 24, 39, 0.72));
  border: 1px solid rgba(71, 85, 105, 0.72);
  border-radius: 12px;
  text-align: left;
  cursor: pointer;
  transition: border-color 0.16s ease, background 0.16s ease, transform 0.16s ease;
}

.filter-card:hover {
  transform: translateY(-1px);
  background: linear-gradient(135deg, rgba(30, 41, 59, 0.98), rgba(15, 23, 42, 0.92));
}

.filter-card.active.tone-all {
  background: linear-gradient(135deg, rgba(71, 85, 105, 0.28), rgba(30, 41, 59, 0.72));
  border-color: #94a3b8;
}

.filter-card.active.tone-open {
  background: linear-gradient(135deg, rgba(37, 99, 235, 0.34), rgba(30, 58, 138, 0.56));
  border-color: #3b82f6;
}

.filter-card.active.tone-partial {
  background: linear-gradient(135deg, rgba(245, 158, 11, 0.26), rgba(120, 53, 15, 0.42));
  border-color: #f59e0b;
}

.filter-card.active.tone-filled {
  background: linear-gradient(135deg, rgba(16, 185, 129, 0.24), rgba(6, 78, 59, 0.42));
  border-color: #34d399;
}

.filter-card.active.tone-cancel {
  background: linear-gradient(135deg, rgba(239, 68, 68, 0.24), rgba(127, 29, 29, 0.44));
  border-color: #ef4444;
}

.filter-card.active.tone-reject {
  background: linear-gradient(135deg, rgba(168, 85, 247, 0.25), rgba(88, 28, 135, 0.44));
  border-color: #a855f7;
}

.filter-icon {
  width: 26px;
  height: 26px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  border-radius: 999px;
  font-size: 18px;
  font-weight: 900;
}

.tone-all .filter-icon,
.top-tab.tone-all strong {
  color: #cbd5e1;
  background: rgba(148, 163, 184, 0.18);
}

.tone-open .filter-icon,
.top-tab.tone-open strong {
  color: #60a5fa;
  background: rgba(59, 130, 246, 0.18);
}

.tone-partial .filter-icon,
.top-tab.tone-partial strong {
  color: #f59e0b;
  background: rgba(245, 158, 11, 0.17);
}

.tone-filled .filter-icon,
.top-tab.tone-filled strong {
  color: #34d399;
  background: rgba(52, 211, 153, 0.16);
}

.tone-cancel .filter-icon,
.top-tab.tone-cancel strong {
  color: #f87171;
  background: rgba(248, 113, 113, 0.16);
}

.tone-reject .filter-icon,
.top-tab.tone-reject strong {
  color: #c084fc;
  background: rgba(192, 132, 252, 0.16);
}

.filter-content {
  display: flex;
  flex-direction: column;
  gap: 3px;
}

.filter-content strong {
  font-size: 16px;
  font-weight: 800;
}

.filter-card.tone-partial .filter-content strong { color: #fbbf24; }
.filter-card.tone-filled .filter-content strong { color: #34d399; }
.filter-card.tone-cancel .filter-content strong { color: #f87171; }
.filter-card.tone-reject .filter-content strong { color: #c084fc; }

.filter-content small {
  color: #9fb0c8;
  font-size: 14px;
}

.orders-panel {
  min-height: 620px;
  padding: 18px;
}

.orders-header {
  margin-bottom: 18px;
}

.orders-header p {
  margin: 4px 0 0;
  color: #94a3b8;
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
  background: rgba(15, 23, 42, 0.78);
  border: 1px solid rgba(71, 85, 105, 0.8);
  border-radius: 9px;
  font-size: 24px;
  cursor: pointer;
}

.refresh-button:hover,
.detail-close:hover {
  color: white;
  border-color: #60a5fa;
}

.refresh-button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.top-tabs {
  display: grid;
  grid-template-columns: repeat(6, max-content);
  align-items: center;
  gap: 22px;
  padding: 14px 20px;
  margin-bottom: 18px;
  background: linear-gradient(180deg, rgba(15, 23, 42, 0.98), rgba(17, 24, 39, 0.92));
  border: 1px solid rgba(51, 65, 85, 0.9);
  border-radius: 10px;
}

.top-tab {
  position: relative;
  display: inline-flex;
  align-items: center;
  gap: 9px;
  padding: 8px 0;
  color: #e5e7eb;
  background: transparent;
  border: 0;
  cursor: pointer;
  font-size: 14px;
  font-weight: 800;
}

.top-tab strong {
  min-width: 28px;
  height: 28px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 999px;
  font-size: 13px;
}

.top-tab.active::after {
  content: '';
  position: absolute;
  left: 0;
  right: 0;
  bottom: -15px;
  height: 2px;
  border-radius: 999px;
  background: currentColor;
}

.top-tab.active.tone-open { color: #60a5fa; }
.top-tab.active.tone-partial { color: #f59e0b; }
.top-tab.active.tone-filled { color: #34d399; }
.top-tab.active.tone-cancel { color: #f87171; }
.top-tab.active.tone-reject { color: #c084fc; }

.panel-message,
.empty-table {
  padding: 24px;
  color: #94a3b8;
  background: rgba(15, 23, 42, 0.72);
  border: 1px solid rgba(51, 65, 85, 0.85);
  border-radius: 10px;
  font-size: 14px;
}

.error-message {
  color: #fca5a5;
  background: rgba(127, 29, 29, 0.18);
  border-color: rgba(248, 113, 113, 0.3);
}

.orders-table-card {
  min-height: 360px;
  overflow: auto;
  background:
    radial-gradient(circle at top left, rgba(37, 99, 235, 0.08), transparent 42%),
    rgba(15, 23, 42, 0.72);
  border: 1px solid rgba(51, 65, 85, 0.85);
  border-radius: 10px;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th,
td {
  padding: 14px 18px;
  border-bottom: 1px solid rgba(51, 65, 85, 0.72);
  text-align: right;
  white-space: nowrap;
}

th:first-child,
td:first-child {
  text-align: left;
}

th {
  color: #a9b8ce;
  font-size: 13px;
  font-weight: 800;
}

td {
  color: #f8fafc;
  font-size: 14px;
}

.order-row {
  cursor: pointer;
  transition: background 0.14s ease;
}

.order-row:hover {
  background: rgba(59, 130, 246, 0.07);
}

.order-row.selected {
  background: rgba(37, 99, 235, 0.16);
}

.order-row.buy:hover {
  background: rgba(16, 185, 129, 0.08);
}

.order-row.sell:hover {
  background: rgba(248, 113, 113, 0.08);
}

.instrument-cell {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.instrument-cell strong {
  font-size: 14px;
  font-weight: 900;
}

.instrument-cell small {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  color: #94a3b8;
  font-size: 12px;
  font-weight: 700;
}

.instrument-cell small span,
.exchange-text {
  color: #facc15;
}

.status-badge,
.side-badge,
.type-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 48px;
  padding: 5px 9px;
  border-radius: 6px;
  font-size: 12px;
  font-weight: 900;
}

.status-badge.tone-open {
  color: #34d399;
  background: rgba(16, 185, 129, 0.18);
  border: 1px solid rgba(52, 211, 153, 0.35);
}

.status-badge.tone-partial {
  color: #fbbf24;
  background: rgba(245, 158, 11, 0.18);
  border: 1px solid rgba(245, 158, 11, 0.35);
}

.status-badge.tone-filled {
  color: #34d399;
  background: rgba(16, 185, 129, 0.18);
  border: 1px solid rgba(52, 211, 153, 0.35);
}

.status-badge.tone-cancel {
  color: #f87171;
  background: rgba(248, 113, 113, 0.16);
  border: 1px solid rgba(248, 113, 113, 0.35);
}

.status-badge.tone-reject {
  color: #c084fc;
  background: rgba(192, 132, 252, 0.16);
  border: 1px solid rgba(192, 132, 252, 0.35);
}

.status-badge.tone-default {
  color: #cbd5e1;
  background: rgba(148, 163, 184, 0.13);
  border: 1px solid rgba(148, 163, 184, 0.25);
}

.buy-text {
  color: #34d399;
  background: rgba(16, 185, 129, 0.14);
  border: 1px solid rgba(52, 211, 153, 0.35);
}

.sell-text {
  color: #f87171;
  background: rgba(248, 113, 113, 0.16);
  border: 1px solid rgba(248, 113, 113, 0.35);
}

.type-badge {
  color: #e5e7eb;
  background: rgba(30, 41, 59, 0.84);
  border: 1px solid rgba(71, 85, 105, 0.72);
}

.mono-text {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
}

.table-footer {
  margin-top: 16px;
  color: #94a3b8;
  font-size: 13px;
}

.detail-panel {
  min-height: 620px;
  padding: 18px;
}

.empty-detail {
  min-height: 560px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: #94a3b8;
  text-align: center;
}

.empty-detail h2 {
  margin: 0 0 8px;
  color: white;
  font-size: 20px;
}

.empty-detail p {
  max-width: 260px;
  margin: 0;
  line-height: 1.5;
}

.order-detail {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.detail-header {
  padding-bottom: 12px;
  border-bottom: 1px solid rgba(51, 65, 85, 0.85);
}

.detail-header p {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 6px 0 0;
  color: #a9b8ce;
  font-size: 13px;
}

.detail-card {
  padding: 14px 16px;
  background: rgba(15, 23, 42, 0.58);
  border: 1px solid rgba(51, 65, 85, 0.9);
  border-radius: 10px;
}

.detail-card h3 {
  margin: 0 0 14px;
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
  color: #a9b8ce;
  font-size: 13px;
}

.detail-row strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
  text-align: right;
  word-break: break-all;
}

@media (max-width: 1500px) {
  .orders-layout {
    grid-template-columns: 236px minmax(620px, 1fr);
  }

  .detail-panel {
    grid-column: 1 / -1;
  }
}

@media (max-width: 1050px) {
  .orders-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .orders-panel,
  .detail-panel {
    min-height: unset;
  }

  .top-tabs {
    grid-template-columns: repeat(3, max-content);
  }
}
</style>

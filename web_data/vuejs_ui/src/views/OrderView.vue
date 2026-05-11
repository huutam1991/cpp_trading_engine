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

type OrderSource = {
  type: string
  strategy_id: string
}

type Order = {
  commission_asset: string
  volumn_in_quote_currency: number
  instrument: Instrument
  error: OrderError
  source: OrderSource
  created_at: number
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

type SortKey = 'instrument' | 'status' | 'order_id' | 'side' | 'type' | 'price' | 'quantity' | 'fee'
type SortDirection = 'asc' | 'desc'

const loading = ref(false)
const errorMessage = ref('')
const orders = ref<Order[]>([])
const selectedOrder = ref<Order | null>(null)
const activeTab = ref('all')
const sortKey = ref<SortKey | null>(null)
const sortDirection = ref<SortDirection>('asc')

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


const sortedOrders = computed(() => {
  if (!sortKey.value) {
    return filteredOrders.value
  }

  return [...filteredOrders.value].sort((left, right) => {
    const leftValue = getSortValue(left, sortKey.value!)
    const rightValue = getSortValue(right, sortKey.value!)

    const result = compareSortValues(leftValue, rightValue)
    return sortDirection.value === 'asc' ? result : -result
  })
})

const isDetailOpen = computed(() => selectedOrder.value !== null)

function getSortValue(order: Order, key: SortKey): string | number {
  switch (key) {
    case 'instrument':
      return order.instrument.symbol
    case 'status':
      return order.status
    case 'order_id':
      return order.order_id
    case 'side':
      return order.side
    case 'type':
      return order.type
    case 'price':
      return order.price
    case 'quantity':
      return order.quantity
    case 'fee':
      return order.fee
  }
}

function compareSortValues(left: string | number, right: string | number) {
  const leftNumber = Number(left)
  const rightNumber = Number(right)

  if (Number.isFinite(leftNumber) && Number.isFinite(rightNumber)) {
    return leftNumber - rightNumber
  }

  return String(left).localeCompare(String(right), undefined, { numeric: true, sensitivity: 'base' })
}

function sortOrders(key: SortKey) {
  if (sortKey.value === key) {
    sortDirection.value = sortDirection.value === 'asc' ? 'desc' : 'asc'
    return
  }

  sortKey.value = key
  sortDirection.value = 'asc'
}

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

const formatCreateTime = (orderId: string | number | bigint): string =>
{
  const milliseconds = Number(BigInt(orderId) / 1000000n);
  const date = new Date(milliseconds);

  const parts = new Intl.DateTimeFormat("en-GB", {
    timeZone: "Asia/Singapore",
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: false,
  }).formatToParts(date);

  const get = (type: string): string => parts.find((p) => p.type === type)?.value ?? "00";
  const millis = String(date.getUTCMilliseconds()).padStart(3, "0");

  return `${get("day")}-${get("month")}-${get("year")} ${get("hour")}:${get("minute")}:${get("second")}.${millis}`;
};

onMounted(() => {
  fetchOrders()
})
</script>

<template>
  <main class="orders-page">
    <section class="orders-layout" :class="{ 'detail-open': isDetailOpen }">
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
          <!-- <span class="filter-icon">{{ tab.icon }}</span> -->

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

        <!-- <div class="top-tabs">
          <button
            v-for="tab in tabs"
            :key="tab.value"
            class="top-tab"
            :class="[`tone-${tab.tone}`, { active: activeTab === tab.value }]"
            @click="selectTab(tab.value)"
          >
            <span>{{ tab.label }}</span>
            <strong>{{ getTabCount(tab) }}</strong>
          </button>
        </div> -->

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
                <th class="sortable-header" @click="sortOrders('instrument')">Instrument</th>
                <th class="sortable-header" @click="sortOrders('status')">Status</th>
                <th class="sortable-header" @click="sortOrders('order_id')">Order ID</th>
                <th class="sortable-header" @click="sortOrders('side')">Side</th>
                <th class="sortable-header" @click="sortOrders('type')">Type</th>
                <th class="sortable-header" @click="sortOrders('price')">Price</th>
                <th class="sortable-header" @click="sortOrders('quantity')">Quantity</th>
                <th class="sortable-header" @click="sortOrders('fee')">Fee</th>
                <th class="sortable-header" @click="sortOrders('order_id')" v-if="!isDetailOpen">Created At</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="order in sortedOrders"
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
                <td>{{ formatNumber(order.fee) }}</td>

                <td v-if="!isDetailOpen" class="mono-text">
                  {{ formatCreateTime(order.created_at) }}
                </td>
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

      <aside v-if="selectedOrder" class="detail-panel">
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

            <div class="detail-row">
              <span>Created At</span>
              <strong class="mono-text">{{ formatCreateTime(selectedOrder.created_at) }}</strong>
            </div>
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

          <section class="detail-card">
            <h3>Source</h3>

            <div class="detail-row"><span>Type</span><strong>{{ selectedOrder.source.type != 'NOT_AVAILABLE' ? selectedOrder.source.type : '–' }}</strong></div>
            <div class="detail-row"><span>Strategy ID</span><strong>{{ selectedOrder.source.strategy_id != 'NO_STRATEGY' ? selectedOrder.source.strategy_id : '–' }}</strong></div>
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
  grid-template-columns: 220px minmax(860px, 1fr);
  gap: 12px;
}

.orders-layout.detail-open {
  grid-template-columns: 220px minmax(860px, 1fr) 430px;
}

.filter-panel,
.orders-panel,
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

.orders-panel {
  min-height: 720px;
  padding: 22px;
}

.detail-panel {
  min-height: 720px;
  padding: 22px;
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
  margin-bottom: 18px;
}

.filter-header h2,
.orders-header h1,
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

.filter-icon {
  width: 24px;
  height: 24px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  border-radius: 999px;
  font-size: 15px;
  font-weight: 900;
  background: #111827;
  border: 1px solid #374151;
}

.tone-all .filter-icon,
.filter-card.tone-all .filter-content strong {
  color: #e5e7eb;
}

.tone-open .filter-icon,
.filter-card.tone-open .filter-content strong {
  color: #60a5fa;
}

.tone-partial .filter-icon,
.filter-card.tone-partial .filter-content strong {
  color: #fbbf24;
}

.tone-filled .filter-icon,
.filter-card.tone-filled .filter-content strong {
  color: #34d399;
}

.tone-cancel .filter-icon,
.filter-card.tone-cancel .filter-content strong {
  color: #f87171;
}

.tone-reject .filter-icon,
.filter-card.tone-reject .filter-content strong {
  color: #c084fc;
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

.orders-header {
  margin-bottom: 18px;
}

.orders-header p {
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

.top-tabs {
  display: grid;
  grid-template-columns: repeat(6, max-content);
  align-items: center;
  gap: 28px;
  padding: 12px 20px;
  margin-bottom: 18px;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 10px;
}

.top-tab {
  position: relative;
  display: inline-flex;
  align-items: center;
  gap: 9px;
  padding: 8px 0;
  color: #d1d5db;
  background: transparent;
  border: 0;
  cursor: pointer;
  font-size: 12px;
  font-weight: 800;
  transition: none;
}

.top-tab strong {
  min-width: 28px;
  height: 28px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 999px;
  font-size: 13px;
  background: #1f2937;
  border: 1px solid #374151;
}

.top-tab.active {
  color: #60a5fa;
}

.top-tab.active::after {
  content: '';
  position: absolute;
  left: 0;
  right: 0;
  bottom: -13px;
  height: 2px;
  border-radius: 999px;
  background: #3b82f6;
}

.top-tab.tone-all strong { color: #e5e7eb; }
.top-tab.tone-open strong { color: #60a5fa; }
.top-tab.tone-partial strong { color: #fbbf24; }
.top-tab.tone-filled strong { color: #34d399; }
.top-tab.tone-cancel strong { color: #f87171; }
.top-tab.tone-reject strong { color: #c084fc; }

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

.orders-table-card {
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

.order-row {
  cursor: pointer;
  transition: none;
}

.order-row:hover {
  background: #273449;
}

.order-row.selected {
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
.side-badge,
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

.status-badge.tone-open {
  color: #60a5fa;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
}

.status-badge.tone-filled {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.status-badge.tone-partial {
  color: #fbbf24;
  background: #3a3320;
  border: 1px solid #8a6d1f;
}

.status-badge.tone-cancel {
  color: #f87171;
  background: #3b2b2b;
  border: 1px solid #7f1d1d;
}

.status-badge.tone-reject {
  color: #c084fc;
  background: #31263d;
  border: 1px solid #6b21a8;
}

.status-badge.tone-default {
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #374151;
}

.buy-text {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.sell-text {
  color: #f87171;
  background: #3b2b2b;
  border: 1px solid #7f1d1d;
}

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

.order-detail {
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

.detail-row span.buy-text {
  color: #34d399;
}

.detail-row span.sell-text {
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
  .orders-layout {
    grid-template-columns: 260px minmax(620px, 1fr);
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

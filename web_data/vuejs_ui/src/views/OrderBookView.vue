<script setup lang="ts">
import { computed, onMounted, onUnmounted, ref } from 'vue'
import { API_BASE_URL, WS_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

console.log(API_BASE_URL)
console.log(WS_BASE_URL)

const auth = useAuthStore()

type RawOrderBookLevel = {
  price: number
  quantity: number
}

type OrderBookMessage = {
  asks: RawOrderBookLevel[]
  bids: RawOrderBookLevel[]
  instrument: string
  route: string
}

type OrderBookLevel = {
  price: number
  quantity: number
  total: number
}

type InstrumentSubscribe = {
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
  data: InstrumentSubscribe[]
}

const connected = ref(false)
const loadingInstruments = ref(false)

const subscribedInstruments = ref<InstrumentSubscribe[]>([])

const activeInstrument = ref('')
const streamInstrument = ref('')

const activeInstrumentInfo = computed(() => {
  return subscribedInstruments.value.find(
    (item) => item.symbol === activeInstrument.value,
  )
})

const asks = ref<OrderBookLevel[]>([])
const bids = ref<OrderBookLevel[]>([])

let ws: WebSocket | null = null

function buildLevels(levels: RawOrderBookLevel[]): OrderBookLevel[] {
  let runningTotal = 0

  return levels.map((level) => {
    runningTotal += level.quantity

    return {
      price: level.price,
      quantity: level.quantity,
      total: runningTotal,
    }
  })
}

const bestAsk = computed(() => {
  const lastAsk = asks.value.at(-1)
  return lastAsk?.price ?? 0
})

const bestBid = computed(() => {
  const firstBid = bids.value.at(0)
  return firstBid?.price ?? 0
})

const spread = computed(() => {
  if (bestAsk.value === 0 || bestBid.value === 0) {
    return 0
  }

  return bestAsk.value - bestBid.value
})

const midPrice = computed(() => {
  if (bestAsk.value === 0 || bestBid.value === 0) {
    return 0
  }

  return (bestAsk.value + bestBid.value) / 2
})

const askTotal = computed(() => {
  return asks.value.at(-1)?.total ?? 0
})

const bidTotal = computed(() => {
  return bids.value.at(-1)?.total ?? 0
})

async function fetchSubscribedInstruments() {
  loadingInstruments.value = true

  try {
    const response = await fetch(
      `${API_BASE_URL}/instrument_subscribe`,
      {
        method: 'GET',
        credentials: 'include',
      },
    )

    const result: InstrumentSubscribeResponse =
      await response.json()

    if (!response.ok || result.error) {
      console.error(
        'Failed to fetch subscribed instruments:',
        result.msg,
      )

      return
    }

    subscribedInstruments.value = result.data

    const firstInstrument = result.data[0]
    if (firstInstrument) {
      activeInstrument.value = firstInstrument.symbol
      streamInstrument.value = firstInstrument.symbol
    }
  } catch (error) {
    console.error(
      'Fetch subscribed instruments error:',
      error,
    )
  } finally {
    loadingInstruments.value = false
  }
}

function selectInstrument(symbol: string) {
  activeInstrument.value = symbol
  streamInstrument.value = symbol

  asks.value = []
  bids.value = []

  if (ws?.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({
      op: 'subscribe',
      route: 'orderbook',
      instrument: symbol,
    }))
  }
}

function ensureInstrumentExists(symbol: string) {
  const exists = subscribedInstruments.value.some(
    (instrument) => instrument.symbol === symbol,
  )

  if (exists) {
    return
  }

  subscribedInstruments.value.push({
    symbol,
    exchange_symbol: '',
    exchange_id: '',
    instrument_type: '',
    lot_size: 0,
    tick_size: 0,
    price_precision: 0,
  })
}

function connectWebSocket() {
  ws = new WebSocket(`${WS_BASE_URL}/orderbook`)

  ws.onopen = () => {
    connected.value = true

    if (activeInstrument.value.length > 0) {
      ws?.send(JSON.stringify({
        op: 'subscribe',
        route: 'orderbook',
        instrument: activeInstrument.value,
      }))
    }
  }

  ws.onmessage = (event) => {
    try {
      const message = JSON.parse(event.data) as OrderBookMessage

      if (message.route !== 'orderbook') {
        return
      }

      ensureInstrumentExists(message.instrument)

      if (
        activeInstrument.value.length > 0 &&
        message.instrument !== activeInstrument.value
      ) {
        return
      }

      if (activeInstrument.value.length === 0) {
        activeInstrument.value = message.instrument
      }

      streamInstrument.value = message.instrument

      asks.value = buildLevels([...message.asks].reverse())
      bids.value = buildLevels(message.bids)
    } catch (error) {
      console.error('[WS] invalid message:', error)
    }
  }

  ws.onclose = (event) => {
    connected.value = false

    if (
      event.code === 1008 &&
      event.reason.includes('Invalid token')
    ) {
      auth.logout()
      return
    }

    console.warn(
      '[WS] closed:',
      event.code,
      event.reason,
    )
  }

  ws.onerror = (event) => {
    connected.value = false
    console.error('[WS] error:', event)
  }
}

onMounted(async () => {
  await fetchSubscribedInstruments()
  connectWebSocket()
})

onUnmounted(() => {
  ws?.close()
})
</script>

<template>
  <main class="orderbook-page">
    <div class="page-header">
      <div>
        <h1>OrderBook</h1>
        <p>Realtime market depth for subscribed instruments.</p>
      </div>

      <span
        class="status-pill"
        :class="connected ? 'connected' : 'disconnected'"
      >
        {{ connected ? 'Connected' : 'Disconnected' }}
      </span>
    </div>

    <section class="summary-grid">
      <div class="metric-card instrument-metric">
        <span>Instrument</span>
        <strong>{{ activeInstrument || '-' }}</strong>
        <small>
          {{ activeInstrumentInfo?.exchange_id || 'UNKNOWN' }}
          ·
          {{ activeInstrumentInfo?.instrument_type || 'UNKNOWN' }}
        </small>
      </div>

      <div class="metric-card">
        <span>Best Ask</span>
        <strong class="ask-text">
          {{ bestAsk.toFixed(2) }}
        </strong>
      </div>

      <div class="metric-card">
        <span>Best Bid</span>
        <strong class="bid-text">
          {{ bestBid.toFixed(2) }}
        </strong>
      </div>

      <div class="metric-card">
        <span>Spread</span>
        <strong>{{ spread.toFixed(2) }}</strong>
      </div>

      <div class="metric-card">
        <span>Mid Price</span>
        <strong>{{ midPrice.toFixed(2) }}</strong>
      </div>

      <div class="metric-card">
        <span>Depth Total</span>
        <strong>
          {{ bidTotal.toFixed(3) }} / {{ askTotal.toFixed(3) }}
        </strong>
      </div>
    </section>

    <section class="orderbook-workspace">
      <aside class="instrument-sidebar">
        <div class="sidebar-header">
          <h2>Subscribed</h2>
          <span>{{ subscribedInstruments.length }}</span>
        </div>

        <div
          v-if="loadingInstruments"
          class="sidebar-message"
        >
          Loading instruments...
        </div>

        <div
          v-else-if="subscribedInstruments.length === 0"
          class="sidebar-message"
        >
          No subscribed instruments.
        </div>

        <button
          v-for="instrument in subscribedInstruments"
          v-else
          :key="instrument.symbol"
          class="instrument-button"
          :class="{ active: instrument.symbol === activeInstrument }"
          @click="selectInstrument(instrument.symbol)"
        >
          <span class="instrument-dot" />

          <div class="instrument-info">
            <strong>{{ instrument.symbol }}</strong>

            <small>
              {{ instrument.exchange_id || 'UNKNOWN' }}
              ·
              {{ instrument.instrument_type || 'UNKNOWN' }}
            </small>
          </div>
        </button>
      </aside>

      <section class="book-panel">
        <div class="panel-header">
          <div>
            <h2>Market Depth</h2>
            <p>{{ streamInstrument || '-' }}</p>
          </div>

          <div class="mid-card">
            <span>Mid</span>
            <strong>{{ midPrice.toFixed(2) }}</strong>
          </div>
        </div>

        <div class="book-grid">
          <div class="side-panel">
            <div class="side-title ask-text">
              Asks
            </div>

            <table>
              <thead>
                <tr>
                  <th>Price</th>
                  <th>Quantity</th>
                  <th>Total</th>
                </tr>
              </thead>

              <tbody>
                <tr
                  v-for="ask in asks"
                  :key="ask.price"
                  class="ask-row"
                >
                  <td>{{ ask.price.toFixed(2) }}</td>
                  <td>{{ ask.quantity }}</td>
                  <td>{{ ask.total.toFixed(3) }}</td>
                </tr>
              </tbody>
            </table>
          </div>

          <div class="side-panel">
            <div class="side-title bid-text">
              Bids
            </div>

            <table>
              <thead>
                <tr>
                  <th>Price</th>
                  <th>Quantity</th>
                  <th>Total</th>
                </tr>
              </thead>

              <tbody>
                <tr
                  v-for="bid in bids"
                  :key="bid.price"
                  class="bid-row"
                >
                  <td>{{ bid.price.toFixed(2) }}</td>
                  <td>{{ bid.quantity }}</td>
                  <td>{{ bid.total.toFixed(3) }}</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </section>

      <section class="detail-panel">
        <div class="empty-state">
          <h2>Details</h2>
          <p>
            Click an order, price level, strategy action or event to inspect details here.
          </p>
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.orderbook-page {
  color: white;
}

.page-header {
  display: flex;
  align-items: center;
  justify-content: space-between;

  margin-bottom: 10px;
}

.page-header h1 {
  margin: 0;
  font-size: 24px;
}

.page-header p {
  display: none;
}

.status-pill {
  padding: 4px 10px;
  border-radius: 999px;

  font-size: 12px;
  font-weight: 700;
}

.status-pill.connected {
  color: #34d399;
  background: rgba(52, 211, 153, 0.12);
  border: 1px solid rgba(52, 211, 153, 0.25);
}

.status-pill.disconnected {
  color: #f87171;
  background: rgba(248, 113, 113, 0.12);
  border: 1px solid rgba(248, 113, 113, 0.25);
}

.summary-grid {
  display: grid;
  grid-template-columns: repeat(6, minmax(120px, 1fr));
  gap: 8px;

  margin-bottom: 10px;
}

.metric-card {
  padding: 10px 12px;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 9px;
}

.metric-card span {
  display: block;

  margin-bottom: 4px;

  color: #9ca3af;
  font-size: 12px;
}

.metric-card strong {
  font-size: 15px;
}

.instrument-metric {
  display: flex;
  flex-direction: column;
}

.instrument-metric strong {
  margin-bottom: 3px;
}

.instrument-metric small {
  color: #9ca3af;
  font-size: 12px;
}

.orderbook-workspace {
  display: grid;
  grid-template-columns: 260px 440px 1fr;
  gap: 12px;
}

.instrument-sidebar {
  padding: 12px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 12px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.sidebar-header {
  display: flex;
  align-items: center;
  justify-content: space-between;

  margin-bottom: 10px;
}

.sidebar-header h2 {
  margin: 0;
  font-size: 16px;
}

.sidebar-header span {
  padding: 4px 9px;

  color: #93c5fd;
  background: rgba(37, 99, 235, 0.18);

  border: 1px solid rgba(59, 130, 246, 0.45);
  border-radius: 999px;

  font-size: 12px;
  font-weight: 700;
}

.sidebar-message {
  padding: 12px;

  color: #9ca3af;
  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;

  font-size: 13px;
}

.instrument-button {
  width: 100%;

  display: flex;
  align-items: center;
  gap: 8px;

  padding: 9px 10px;
  margin-bottom: 7px;

  color: #d1d5db;
  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;

  cursor: pointer;

  text-align: left;
}

.instrument-button:hover {
  background: #273449;
}

.instrument-button.active {
  color: white;
  background: rgba(37, 99, 235, 0.22);
  border-color: rgba(59, 130, 246, 0.8);
}

.instrument-dot {
  width: 7px;
  height: 7px;
  flex-shrink: 0;

  border-radius: 999px;
  background: #34d399;
}

.instrument-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.instrument-info strong {
  font-size: 13px;
}

.instrument-info small {
  color: #9ca3af;
  font-size: 11px;
}

.book-panel {
  padding: 12px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 12px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;

  margin-bottom: 8px;
}

.panel-header h2 {
  margin: 0;
  font-size: 18px;
}

.panel-header p {
  display: none;
}

.mid-card {
  min-width: 100px;
  padding: 7px 10px;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;

  text-align: right;
}

.mid-card span {
  display: block;
  color: #9ca3af;
  font-size: 12px;
}

.mid-card strong {
  font-size: 16px;
}

.book-grid {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.side-panel {
  overflow: hidden;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;
}

.side-title {
  padding: 8px 10px;

  font-size: 15px;
  font-weight: 700;

  border-bottom: 1px solid #374151;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th {
  padding: 7px 8px;

  color: #9ca3af;
  background: #111827;

  border-bottom: 1px solid #374151;

  text-align: right;
  font-size: 12px;
}

td {
  padding: 6px 8px;

  border-bottom: 1px solid rgba(55, 65, 81, 0.7);

  text-align: right;
  font-size: 13px;
}

th:first-child,
td:first-child {
  text-align: left;
}

.ask-row td:first-child,
.ask-text {
  color: #f87171;
}

.bid-row td:first-child,
.bid-text {
  color: #34d399;
}

.ask-row:hover {
  background: rgba(248, 113, 113, 0.08);
}

.bid-row:hover {
  background: rgba(52, 211, 153, 0.08);
}

.detail-panel {
  min-height: 520px;

  padding: 12px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 12px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.empty-state {
  height: 100%;
  min-height: 460px;

  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;

  color: #9ca3af;
  text-align: center;
}

.empty-state h2 {
  margin: 0 0 8px;
  color: white;
}

.empty-state p {
  max-width: 420px;
  margin: 0;
}

@media (max-width: 1400px) {
  .summary-grid {
    grid-template-columns: repeat(3, 1fr);
  }

  .orderbook-workspace {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 800px) {
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 8px;
  }

  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
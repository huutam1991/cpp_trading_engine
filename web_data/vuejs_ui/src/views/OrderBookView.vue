<script setup lang="ts">
import { computed, onUnmounted, ref } from 'vue'
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

const connected = ref(false)

const subscribedInstruments = ref<string[]>([
  'BTC-USDC-PERPETUAL',
  'ETH-USDT-PERPETUAL',
])

const activeInstrument = ref('BTC-USDC-PERPETUAL')
const streamInstrument = ref('BTC-USDC-PERPETUAL')

const asks = ref<OrderBookLevel[]>([])
const bids = ref<OrderBookLevel[]>([])

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

const ws = new WebSocket(`${WS_BASE_URL}/orderbook`)

function selectInstrument(symbol: string) {
  activeInstrument.value = symbol
  asks.value = []
  bids.value = []

  if (ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({
      op: 'subscribe',
      route: 'orderbook',
      instrument: symbol,
    }))
  }
}

ws.onopen = () => {
  connected.value = true
  console.log('[WS] connected')

  ws.send(JSON.stringify({
    op: 'subscribe',
    route: 'orderbook',
    instrument: activeInstrument.value,
  }))
}

ws.onmessage = (event) => {
  try {
    const message = JSON.parse(event.data) as OrderBookMessage

    if (message.route !== 'orderbook') {
      return
    }

    if (!subscribedInstruments.value.includes(message.instrument)) {
      subscribedInstruments.value.push(message.instrument)
    }

    if (message.instrument !== activeInstrument.value) {
      return
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

  if (event.code === 1008 && event.reason.includes('Invalid token')) {
    auth.logout()
    return
  }

  console.warn('[WS] closed:', event.code, event.reason)
}

ws.onerror = (event) => {
  connected.value = false
  console.error('[WS] error:', event)
}

onUnmounted(() => {
  ws.close()
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
      <div class="metric-card">
        <span>Instrument</span>
        <strong>{{ activeInstrument }}</strong>
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

        <button
          v-for="symbol in subscribedInstruments"
          :key="symbol"
          class="instrument-button"
          :class="{ active: symbol === activeInstrument }"
          @click="selectInstrument(symbol)"
        >
          <span class="instrument-dot" />
          <span>{{ symbol }}</span>
        </button>
      </aside>

      <section class="book-panel">
        <div class="panel-header">
          <div>
            <h2>Market Depth</h2>
            <p>{{ streamInstrument }}</p>
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
    </section>
  </main>
</template>

<style scoped>
.orderbook-page {
  color: white;
}

.page-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;

  margin-bottom: 20px;
}

.page-header h1 {
  margin: 0 0 8px;
  font-size: 32px;
}

.page-header p {
  margin: 0;
  color: #9ca3af;
}

.status-pill {
  padding: 7px 13px;
  border-radius: 999px;

  font-size: 13px;
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
  grid-template-columns: repeat(6, minmax(140px, 1fr));
  gap: 12px;

  margin-bottom: 18px;
}

.metric-card {
  padding: 15px;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 12px;
}

.metric-card span {
  display: block;
  margin-bottom: 8px;

  color: #9ca3af;
  font-size: 13px;
}

.metric-card strong {
  font-size: 18px;
}

.orderbook-workspace {
  display: grid;
  grid-template-columns: 300px 1fr;
  gap: 18px;
}

.instrument-sidebar {
  padding: 16px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 14px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.sidebar-header {
  display: flex;
  align-items: center;
  justify-content: space-between;

  margin-bottom: 14px;
}

.sidebar-header h2 {
  margin: 0;
  font-size: 20px;
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

.instrument-button {
  width: 100%;

  display: flex;
  align-items: center;
  gap: 9px;

  padding: 12px;
  margin-bottom: 9px;

  color: #d1d5db;
  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;

  cursor: pointer;

  text-align: left;
  font-weight: 700;
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
  width: 8px;
  height: 8px;

  border-radius: 999px;
  background: #34d399;
}

.book-panel {
  padding: 20px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 14px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;

  margin-bottom: 16px;
}

.panel-header h2 {
  margin: 0 0 6px;
  font-size: 24px;
}

.panel-header p {
  margin: 0;
  color: #9ca3af;
}

.mid-card {
  min-width: 140px;
  padding: 10px 14px;

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
  font-size: 20px;
}

.book-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 14px;
}

.side-panel {
  overflow: hidden;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 12px;
}

.side-title {
  padding: 12px 14px;

  font-size: 18px;
  font-weight: 700;

  border-bottom: 1px solid #374151;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th {
  padding: 10px 12px;

  color: #9ca3af;
  background: #111827;

  border-bottom: 1px solid #374151;

  text-align: right;
  font-size: 13px;
}

td {
  padding: 10px 12px;

  border-bottom: 1px solid rgba(55, 65, 81, 0.7);

  text-align: right;
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

@media (max-width: 1400px) {
  .summary-grid {
    grid-template-columns: repeat(3, 1fr);
  }

  .orderbook-workspace {
    grid-template-columns: 1fr;
  }

  .book-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 800px) {
  .page-header {
    flex-direction: column;
    gap: 12px;
  }

  .summary-grid {
    grid-template-columns: 1fr;
  }
}
</style>
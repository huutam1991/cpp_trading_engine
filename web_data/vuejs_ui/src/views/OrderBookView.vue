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

const instrument = ref('BTC-USDC-PERP')
const connected = ref(false)

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

const ws = new WebSocket(`${WS_BASE_URL}/orderbook`)

ws.onopen = () => {
  connected.value = true
  console.log('[WS] connected')
}

ws.onmessage = (event) => {
  try {
    const message = JSON.parse(event.data) as OrderBookMessage

    if (message.route !== 'orderbook') {
      return
    }

    instrument.value = message.instrument

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
    <h1>Order Book</h1>

    <section class="summary">
      <div>
        <span>Symbol</span>
        <strong>{{ instrument }}</strong>
      </div>

      <div>
        <span>Spread</span>
        <strong>{{ spread.toFixed(2) }}</strong>
      </div>

      <div>
        <span>Status</span>
        <strong class="connected">Connected</strong>
      </div>
    </section>

    <section class="book">
      <table>
        <thead>
          <tr>
            <th>Ask Price</th>
            <th>Quantity</th>
            <th>Total</th>
          </tr>
        </thead>

        <tbody>
          <tr v-for="ask in asks" :key="ask.price" class="ask">
            <td>{{ ask.price.toFixed(2) }}</td>
            <td>{{ ask.quantity }}</td>
            <td>{{ ask.total.toFixed(3) }}</td>
          </tr>
        </tbody>
      </table>

      <div class="mid-price">Mid Price: {{ midPrice.toFixed(2) }}</div>

      <table>
        <thead>
          <tr>
            <th>Bid Price</th>
            <th>Quantity</th>
            <th>Total</th>
          </tr>
        </thead>

        <tbody>
          <tr v-for="bid in bids" :key="bid.price" class="bid">
            <td>{{ bid.price.toFixed(2) }}</td>
            <td>{{ bid.quantity }}</td>
            <td>{{ bid.total.toFixed(3) }}</td>
          </tr>
        </tbody>
      </table>
    </section>
  </main>
</template>

<style scoped>
.orderbook-page {
  padding: 24px;
  font-family: Arial, sans-serif;
  background: #111827;
  min-height: 100vh;
  color: white;
}

.summary {
  display: flex;
  gap: 16px;
  margin-bottom: 24px;
}

.summary div {
  border: 1px solid #ddd;
  border-radius: 8px;
  padding: 12px 16px;
  min-width: 160px;
}

.summary span {
  display: block;
  color: #777;
  font-size: 12px;
  margin-bottom: 4px;
}

.connected {
  color: green;
}

.book {
  max-width: 700px;
}

table {
  width: 100%;
  border-collapse: collapse;
  background: #1f2937;
  border-radius: 8px;
  overflow: hidden;
}

th,
td {
  padding: 8px 12px;
  text-align: right;
  border-bottom: 1px solid #374151;
}

th:first-child,
td:first-child {
  text-align: left;
}

.ask td:first-child {
  color: #c0392b;
}

.bid td:first-child {
  color: #16a085;
}

.mid-price {
  padding: 16px;
  text-align: center;
  font-weight: bold;
  border-top: 1px solid #ddd;
  border-bottom: 1px solid #374151;
}
</style>
<script setup lang="ts">
type GatewayStatus = 'connected' | 'disconnected' | 'reconnecting'

type Gateway = {
  id: string
  name: string
  exchange: string
  environment: string
  endpoint: string
  status: GatewayStatus
  uptime: string
  latencyMs: number | null
  instruments: number
  accounts: number
  messages1m: number
  lastUpdate: string
  events: {
    level: 'info' | 'warn' | 'error'
    time: string
    message: string
  }[]
}

const gateways: Gateway[] = [
  {
    id: 'binance_futures',
    name: 'Binance Futures',
    exchange: 'Binance Futures',
    environment: 'Production',
    endpoint: 'wss://fstream.binance.com',
    status: 'connected',
    uptime: '2d 14h 36m',
    latencyMs: 18.4,
    instruments: 320,
    accounts: 2,
    messages1m: 12540,
    lastUpdate: '10:15:30',
    events: [
      { level: 'info', time: '10:15:12', message: 'Heartbeat received' },
      { level: 'info', time: '10:14:42', message: 'OrderBook snapshot updated: BTCUSDT' },
      { level: 'warn', time: '10:14:10', message: 'High latency detected: 245ms' },
    ],
  },
  {
    id: 'coinbase',
    name: 'Coinbase',
    exchange: 'Coinbase',
    environment: 'Production',
    endpoint: 'wss://ws-feed.exchange.coinbase.com',
    status: 'disconnected',
    uptime: '-',
    latencyMs: null,
    instruments: 120,
    accounts: 1,
    messages1m: 0,
    lastUpdate: '-',
    events: [
      { level: 'error', time: '10:10:05', message: 'Connection closed: No route to host' },
    ],
  },
]

function statusText(status: GatewayStatus) {
  if (status === 'connected') return 'Connected'
  if (status === 'reconnecting') return 'Reconnecting'
  return 'Disconnected'
}
</script>

<template>
  <section class="gateway-page">
    <div class="page-header">
      <h1>Gateway</h1>
      <p>Manage and monitor exchange gateways and connections.</p>
    </div>

    <div class="gateway-list">
      <article
        v-for="gateway in gateways"
        :key="gateway.id"
        class="gateway-card"
      >
        <div class="gateway-top">
          <div>
            <div class="gateway-title-row">
              <h2>{{ gateway.name }}</h2>

              <span
                class="status-pill"
                :class="gateway.status"
              >
                {{ statusText(gateway.status) }}
              </span>
            </div>

            <p class="gateway-subtitle">
              {{ gateway.id }} · {{ gateway.endpoint }}
            </p>
          </div>

          <div class="actions">
            <button class="button secondary">View Details</button>

            <button
              v-if="gateway.status === 'connected'"
              class="button danger"
            >
              Disconnect
            </button>

            <button
              v-else
              class="button primary"
            >
              Connect
            </button>
          </div>
        </div>

        <div class="metrics-grid">
          <div class="metric-card">
            <span>Status</span>
            <strong :class="gateway.status">
              {{ statusText(gateway.status) }}
            </strong>
          </div>

          <div class="metric-card">
            <span>Uptime</span>
            <strong>{{ gateway.uptime }}</strong>
          </div>

          <div class="metric-card">
            <span>Latency</span>
            <strong>
              {{ gateway.latencyMs === null ? '-' : `${gateway.latencyMs} ms` }}
            </strong>
          </div>

          <div class="metric-card">
            <span>Instruments</span>
            <strong>{{ gateway.instruments }}</strong>
          </div>

          <div class="metric-card">
            <span>Accounts</span>
            <strong>{{ gateway.accounts }}</strong>
          </div>

          <div class="metric-card">
            <span>Messages / 1m</span>
            <strong>{{ gateway.messages1m.toLocaleString() }}</strong>
          </div>

          <div class="metric-card">
            <span>Last Update</span>
            <strong>{{ gateway.lastUpdate }}</strong>
          </div>
        </div>

        <div class="details-grid">
          <section class="panel">
            <h3>Connection Info</h3>

            <div class="info-row">
              <span>Exchange</span>
              <strong>{{ gateway.exchange }}</strong>
            </div>

            <div class="info-row">
              <span>Environment</span>
              <strong>{{ gateway.environment }}</strong>
            </div>

            <div class="info-row">
              <span>Endpoint</span>
              <strong>{{ gateway.endpoint }}</strong>
            </div>

            <div class="info-row">
              <span>Protocol</span>
              <strong>WebSocket</strong>
            </div>

            <div class="info-row">
              <span>Compression</span>
              <strong>Enabled</strong>
            </div>
          </section>

          <section class="panel">
            <div class="panel-header">
              <h3>Recent Events</h3>
              <button class="link-button">View All Logs</button>
            </div>

            <div class="event-list">
              <div
                v-for="event in gateway.events"
                :key="`${gateway.id}-${event.time}-${event.message}`"
                class="event-row"
              >
                <span
                  class="event-dot"
                  :class="event.level"
                />

                <span class="event-time">
                  {{ event.time }}
                </span>

                <span
                  class="event-level"
                  :class="event.level"
                >
                  {{ event.level.toUpperCase() }}
                </span>

                <span class="event-message">
                  {{ event.message }}
                </span>
              </div>
            </div>
          </section>
        </div>
      </article>
    </div>
  </section>
</template>

<style scoped>
.gateway-page {
  color: white;
}

.page-header {
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

.gateway-list {
  display: flex;
  flex-direction: column;
  gap: 18px;
}

.gateway-card {
  width: 100%;
  box-sizing: border-box;

  padding: 20px;

  background: #111827;

  border: 1px solid #374151;
  border-radius: 14px;

  box-shadow: 0 0 28px rgba(0, 0, 0, 0.25);
}

.gateway-top {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 20px;
}

.gateway-title-row {
  display: flex;
  align-items: center;
  gap: 12px;
}

.gateway-title-row h2 {
  margin: 0;
  font-size: 24px;
}

.gateway-subtitle {
  margin: 8px 0 0;
  color: #9ca3af;
}

.status-pill {
  padding: 5px 12px;
  border-radius: 999px;
  font-size: 13px;
  font-weight: bold;
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

.status-pill.reconnecting {
  color: #fbbf24;
  background: rgba(251, 191, 36, 0.12);
  border: 1px solid rgba(251, 191, 36, 0.25);
}

.actions {
  display: flex;
  align-items: center;
  gap: 10px;
}

.button {
  padding: 9px 14px;
  border-radius: 8px;
  cursor: pointer;
  color: white;
  border: 1px solid #374151;
  background: #1f2937;
}

.button.primary {
  color: #93c5fd;
  border-color: rgba(59, 130, 246, 0.45);
  background: rgba(37, 99, 235, 0.18);
}

.button.secondary:hover {
  background: #273449;
}

.button.danger {
  color: #f87171;
  border-color: rgba(248, 113, 113, 0.35);
  background: rgba(127, 29, 29, 0.18);
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(7, minmax(120px, 1fr));
  gap: 10px;
  margin-bottom: 14px;
}

.metric-card {
  padding: 14px;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;
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

.connected {
  color: #34d399;
}

.disconnected {
  color: #f87171;
}

.reconnecting {
  color: #fbbf24;
}

.details-grid {
  display: grid;
  grid-template-columns: 360px 1fr;
  gap: 12px;
}

.panel {
  padding: 16px;

  background: #1f2937;

  border: 1px solid #374151;
  border-radius: 10px;
}

.panel h3 {
  margin: 0 0 14px;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.link-button {
  color: #60a5fa;
  background: transparent;
  border: none;
  cursor: pointer;
}

.info-row {
  display: grid;
  grid-template-columns: 130px 1fr;
  gap: 12px;

  padding: 7px 0;

  border-bottom: 1px solid rgba(55, 65, 81, 0.6);
}

.info-row span {
  color: #9ca3af;
}

.info-row strong {
  font-weight: normal;
}

.event-list {
  display: flex;
  flex-direction: column;
}

.event-row {
  display: grid;
  grid-template-columns: 12px 70px 70px 1fr;
  align-items: center;
  gap: 10px;

  padding: 8px 0;

  border-bottom: 1px solid rgba(55, 65, 81, 0.6);
}

.event-dot {
  width: 7px;
  height: 7px;
  border-radius: 999px;
}

.event-dot.info {
  background: #34d399;
}

.event-dot.warn {
  background: #fbbf24;
}

.event-dot.error {
  background: #f87171;
}

.event-time {
  color: #d1d5db;
}

.event-level.info {
  color: #34d399;
}

.event-level.warn {
  color: #fbbf24;
}

.event-level.error {
  color: #f87171;
}

.event-message {
  color: #e5e7eb;
}

@media (max-width: 1200px) {
  .metrics-grid {
    grid-template-columns: repeat(3, 1fr);
  }

  .details-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 700px) {
  .gateway-top {
    flex-direction: column;
  }

  .metrics-grid {
    grid-template-columns: 1fr;
  }

  .event-row {
    grid-template-columns: 12px 64px 64px 1fr;
  }
}
</style>
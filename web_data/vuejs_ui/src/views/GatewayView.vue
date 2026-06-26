<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

type GatewayStatus = 'connected' | 'disconnected' | 'reconnecting'

type GatewayListItem = {
  messages_per_minute?: number
  up_time?: string
  latency?: string
  instruments?: number
  exchange_id?: string
  endpoints?: Record<string, string>
  accounts?: number
  environment?: string
  status?: string
}

type GatewayListResponse = {
  error: boolean
  status_code: number
  msg: string
  data: GatewayListItem[]
}

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
  protocol: string
  compression: string
  events: {
    level: 'info' | 'warn' | 'error'
    time: string
    message: string
  }[]
}

const loading = ref(false)
const errorMessage = ref('')
const gateways = ref<Gateway[]>([])

function normalizeGatewayStatus(status?: string): GatewayStatus {
  const normalized = (status ?? '').toLowerCase()

  if (normalized === 'connected') return 'connected'
  if (normalized === 'reconnecting') return 'reconnecting'
  return 'disconnected'
}

function parseLatencyMs(latency?: string) {
  if (!latency) {
    return null
  }

  const value = Number.parseFloat(latency.replace('ms', '').trim())
  return Number.isFinite(value) ? value : null
}

function formatGatewayName(exchangeId?: string) {
  if (!exchangeId) {
    return 'Unknown Gateway'
  }

  return exchangeId
    .toLowerCase()
    .split('_')
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join(' ')
}

function getPrimaryEndpoint(endpoints?: Record<string, string>) {
  if (!endpoints || Object.keys(endpoints).length === 0) {
    return 'wss://stream.placeholder.local'
  }

  const [type, url] = Object.entries(endpoints)[0]!
  return `${type}: ${url}`
}

function getProtocol(endpoints?: Record<string, string>) {
  const urls = Object.values(endpoints ?? {})

  if (urls.some((url) => url.startsWith('wss://') || url.startsWith('ws://'))) {
    return 'WebSocket'
  }

  if (urls.some((url) => url.startsWith('http://') || url.startsWith('https://'))) {
    return 'REST'
  }

  return 'WebSocket'
}

function getCurrentTime() {
  return new Intl.DateTimeFormat('en-GB', {
    timeZone: 'Asia/Singapore',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false,
  }).format(new Date())
}

function buildEvents(item: GatewayListItem, status: GatewayStatus) {
  const now = getCurrentTime()
  const exchange = item.exchange_id ?? 'UNKNOWN'

  if (status === 'connected') {
    return [
      { level: 'info' as const, time: now, message: `${exchange} gateway connected` },
      { level: 'info' as const, time: now, message: `Market data running: ${(item.messages_per_minute ?? 0).toLocaleString()} msg/min` },
      { level: 'info' as const, time: now, message: `Latency updated: ${item.latency ?? '0ms'}` },
    ]
  }

  return [
    { level: 'error' as const, time: now, message: `${exchange} gateway disconnected` },
    { level: 'warn' as const, time: now, message: 'Waiting for reconnect signal' },
  ]
}

function mapGateway(item: GatewayListItem): Gateway {
  const status = normalizeGatewayStatus(item.status)

  return {
    id: item.exchange_id ?? 'UNKNOWN',
    name: formatGatewayName(item.exchange_id),
    exchange: item.exchange_id ?? 'UNKNOWN',
    environment: item.environment ?? 'Production',
    endpoint: getPrimaryEndpoint(item.endpoints),
    status,
    uptime: item.up_time && item.up_time !== '0' ? item.up_time : '-',
    latencyMs: parseLatencyMs(item.latency),
    instruments: item.instruments ?? 0,
    accounts: item.accounts ?? 0,
    messages1m: item.messages_per_minute ?? 0,
    lastUpdate: getCurrentTime(),
    protocol: getProtocol(item.endpoints),
    compression: 'Enabled',
    events: buildEvents(item, status),
  }
}

async function fetchGateways() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/gateway_list`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: GatewayListResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch gateway list.'
      return
    }

    gateways.value = (result.data ?? []).map(mapGateway)
  } catch (error) {
    console.error('Fetch gateway list error:', error)
    errorMessage.value = 'Fetch gateway list error.'
  } finally {
    loading.value = false
  }
}

function statusText(status: GatewayStatus) {
  if (status === 'connected') return 'Connected'
  if (status === 'reconnecting') return 'Reconnecting'
  return 'Disconnected'
}

onMounted(() => {
  fetchGateways()
})
</script>

<template>
  <section class="gateway-page">
    <div class="page-header">
      <h1>Gateway</h1>
      <p>Manage and monitor exchange gateways and connections.</p>
    </div>

    <div class="gateway-toolbar">
      <button
        class="button secondary"
        :disabled="loading"
        @click="fetchGateways"
      >
        ↻ Refresh
      </button>
    </div>

    <div
      v-if="loading"
      class="panel-message"
    >
      Loading gateways...
    </div>

    <div
      v-else-if="errorMessage"
      class="panel-message error-message"
    >
      {{ errorMessage }}
    </div>

    <div
      v-else-if="gateways.length === 0"
      class="panel-message"
    >
      No gateways found.
    </div>

    <div
      v-else
      class="gateway-list"
    >
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
              <strong>{{ gateway.protocol }}</strong>
            </div>

            <div class="info-row">
              <span>Compression</span>
              <strong>{{ gateway.compression }}</strong>
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

.gateway-toolbar {
  display: flex;
  justify-content: flex-end;
  margin-bottom: 14px;
}

.panel-message {
  padding: 22px;
  color: #d1d5db;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 14px;
}

.error-message {
  color: #f87171;
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

.instrument-section {
  margin-top: 18px;
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
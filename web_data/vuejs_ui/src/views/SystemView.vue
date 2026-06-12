<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

type CrashCallPathFrame = {
  frame_index: number | string
  function: string
  file: string
  line: string
}

type CrashLog = {
  app: string
  env: string
  signal: string
  crash_function: string
  crash_file: string
  crash_line: string
  suspect_function?: string
  suspect_file?: string
  suspect_line?: string
  call_path?: CrashCallPathFrame[]
  core_file?: string
  core_file_size_bytes?: number | string
  backtrace_size?: number | string
  frame_count?: number | string
  created_at: string
  created_at_ns: number | string
}

type CrashLogResponse = {
  error: boolean
  status_code: number
  msg: string
  data: CrashLog[]
}

type RequestLog = {
  method: string
  start_time: string
  duration: string
  endpoint: string
  host: string
  client_ip: string
  user_agent: string
  response_status: string
}

type RequestLogResponse = {
  error: boolean
  status_code: number
  msg: string
  data: RequestLog[]
}

type ObjectPoolInfoResponse = {
  error: boolean
  status_code: number
  msg: string
  data: Record<string, number>
}

type UpTimeResponse = {
  error: boolean
  status_code: number
  msg: string
  data: string
}

type SystemTab = {
  label: string
  value: 'crash_log' | 'request_log' | 'object_pool'
  description: string
}

const tabs: SystemTab[] = [
  { label: 'Crash Log', value: 'crash_log', description: 'Runtime crash reports' },
  { label: 'Request', value: 'request_log', description: 'HTTP request logs' },
  { label: 'Object Pool', value: 'object_pool', description: 'Pool metrics' },
]

const activeTab = ref<SystemTab['value']>('crash_log')
const loading = ref(false)
const errorMessage = ref('')
const crashLogs = ref<CrashLog[]>([])
const requestLogs = ref<RequestLog[]>([])
const objectPoolInfo = ref<Record<string, number>>({})
const upTime = ref('--:--:--')
const expandedCrashId = ref<string | null>(null)

let upTimeTimer: ReturnType<typeof setInterval> | null = null

const activeTabInfo = computed(() => {
  return tabs.find((tab) => tab.value === activeTab.value) ?? tabs[0]!
})

const sortedCrashLogs = computed(() => {
  return [...crashLogs.value].sort((left, right) => {
    return Number(right.created_at_ns) - Number(left.created_at_ns)
  })
})

const sortedRequestLogs = computed(() => {
  return [...requestLogs.value].sort((left, right) => {
    return parseDateTime(right.start_time) - parseDateTime(left.start_time)
  })
})

const objectPoolEntries = computed(() => {
  return Object.entries(objectPoolInfo.value).map(([name, size]) => ({
    name,
    size,
  }))
})

function getTabCount(tab: SystemTab) {
  if (tab.value === 'crash_log') {
    return crashLogs.value.length
  }

  if (tab.value === 'request_log') {
    return requestLogs.value.length
  }

  return objectPoolEntries.value.length
}

function formatNumber(value: number) {
  return value.toLocaleString('en-US')
}

function parseDateTime(value: string) {
  const match = /^(\d{2})-(\d{2})-(\d{4}) (\d{2}):(\d{2}):(\d{2})$/.exec(value)

  if (!match) {
    return 0
  }

  const [, day, month, year, hour, minute, second] = match

  return new Date(
    Number(year),
    Number(month) - 1,
    Number(day),
    Number(hour),
    Number(minute),
    Number(second),
  ).getTime()
}

function getMethodTone(method: string) {
  return `method-${method.toLowerCase()}`
}

function getResponseStatusTone(responseStatus: string) {
  if (responseStatus === 'OK_200' || responseStatus === 'CREATED_201') {
    return 'response-success'
  }

  if (responseStatus === 'BAD_REQUEST_400' || responseStatus === 'UNAUTHORIZED_REQUEST_401') {
    return 'response-warning'
  }

  if (responseStatus === 'NOT_FOUND_404') {
    return 'response-not-found'
  }

  if (responseStatus === 'RISK_ERROR_410') {
    return 'response-risk'
  }

  if (responseStatus === 'INTERNAL_SERVER_ERROR_500') {
    return 'response-error'
  }

  return 'response-unknown'
}

function formatResponseStatus(responseStatus: string) {
  return responseStatus || '–'
}

function formatDuration(value: string) {
  return value.trim() || '–'
}

function formatFilePath(file?: string) {
  if (!file) {
    return '–'
  }

  const projectPrefix = '/home/huutam1991/projects/personal/cpp_trading_engine/'

  if (file.startsWith(projectPrefix)) {
    return file.slice(projectPrefix.length)
  }

  const parts = file.split('/')
  return parts.length > 2 ? parts.slice(-3).join('/') : file
}

function formatFunctionName(functionName?: string) {
  if (!functionName) {
    return '–'
  }

  return functionName.replace(/\s+/g, ' ').trim()
}

function getCrashId(log: CrashLog, index?: number) {
  return String(log.created_at_ns ?? `${log.created_at}-${index ?? 0}`)
}

function toggleCrashDetail(log: CrashLog, index: number) {
  const id = getCrashId(log, index)
  expandedCrashId.value = expandedCrashId.value === id ? null : id
}

function getUniqueCallPath(callPath?: CrashCallPathFrame[]) {
  if (!callPath) {
    return []
  }

  const seen = new Set<string>()
  const uniqueFrames: CrashCallPathFrame[] = []

  for (const frame of callPath) {
    const key = `${frame.function}|${frame.file}|${frame.line}`

    if (seen.has(key)) {
      continue
    }

    seen.add(key)
    uniqueFrames.push(frame)
  }

  return uniqueFrames
}

function formatBytes(value?: number | string) {
  const bytes = Number(value)

  if (!Number.isFinite(bytes) || bytes <= 0) {
    return '–'
  }

  const gb = bytes / 1024 / 1024 / 1024

  if (gb >= 1) {
    return `${gb.toFixed(1)}G`
  }

  const mb = bytes / 1024 / 1024

  if (mb >= 1) {
    return `${mb.toFixed(1)}M`
  }

  return `${Math.round(bytes / 1024)}K`
}

function parseUpTime(value: string): number | null {
  const match = /^(\d+):(\d+):(\d+)$/.exec(value)

  if (!match) {
    return null
  }

  const hours = Number(match[1])
  const minutes = Number(match[2])
  const seconds = Number(match[3])

  if (Number.isNaN(hours) || Number.isNaN(minutes) || Number.isNaN(seconds)) {
    return null
  }

  return hours * 3600 + minutes * 60 + seconds
}

function formatUpTime(totalSeconds: number) {
  const hours = Math.floor(totalSeconds / 3600)
  const minutes = Math.floor((totalSeconds % 3600) / 60)
  const seconds = totalSeconds % 60

  return [
    String(hours).padStart(2, '0'),
    String(minutes).padStart(2, '0'),
    String(seconds).padStart(2, '0'),
  ].join(':')
}

function stopUpTimeTimer() {
  if (upTimeTimer) {
    clearInterval(upTimeTimer)
    upTimeTimer = null
  }
}

function startUpTimeTimer() {
  stopUpTimeTimer()

  upTimeTimer = setInterval(() => {
    const currentSeconds = parseUpTime(upTime.value)

    if (currentSeconds === null) {
      return
    }

    upTime.value = formatUpTime(currentSeconds + 1)
  }, 1000)
}

async function selectTab(tab: SystemTab['value']) {
  activeTab.value = tab

  if (tab === 'crash_log') {
    await fetchCrashLogs()
  } else if (tab === 'request_log') {
    await fetchRequestLogs()
  } else if (tab === 'object_pool') {
    await fetchObjectPoolInfo()
  }
}

async function fetchUpTime() {
  try {
    const response = await fetch(`${API_BASE_URL}/up_time`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: UpTimeResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      authStore.logout()
      stopUpTimeTimer()
      return
    }

    if (!response.ok || result.error) {
      upTime.value = '--:--:--'
      stopUpTimeTimer()
      return
    }

    upTime.value = result.data || '--:--:--'

    if (parseUpTime(upTime.value) !== null) {
      startUpTimeTimer()
    } else {
      stopUpTimeTimer()
    }
  } catch (error) {
    console.error('Fetch up time error:', error)
    upTime.value = '--:--:--'
    stopUpTimeTimer()
  }
}

async function fetchCrashLogs() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/crash_log`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: CrashLogResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      authStore.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch crash logs.'
      return
    }

    crashLogs.value = result.data ?? []
  } catch (error) {
    console.error('Fetch crash logs error:', error)
    errorMessage.value = 'Fetch crash logs error.'
  } finally {
    loading.value = false
  }
}

async function fetchRequestLogs() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/request_log`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: RequestLogResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      authStore.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch request logs.'
      return
    }

    requestLogs.value = result.data ?? []
  } catch (error) {
    console.error('Fetch request logs error:', error)
    errorMessage.value = 'Fetch request logs error.'
  } finally {
    loading.value = false
  }
}

async function fetchObjectPoolInfo() {
  loading.value = true
  errorMessage.value = ''

  try {
    const response = await fetch(`${API_BASE_URL}/object_pool_info`, {
      method: 'GET',
      credentials: 'include',
    })

    const result: ObjectPoolInfoResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      authStore.logout()
      return
    }

    if (!response.ok || result.error) {
      errorMessage.value = result.msg || 'Failed to fetch object pool info.'
      return
    }

    objectPoolInfo.value = result.data ?? {}
  } catch (error) {
    console.error('Fetch object pool info error:', error)
    errorMessage.value = 'Fetch object pool info error.'
  } finally {
    loading.value = false
  }
}

async function refreshSystem() {
  await fetchUpTime()

  if (activeTab.value === 'crash_log') {
    await fetchCrashLogs()
  } else if (activeTab.value === 'request_log') {
    await fetchRequestLogs()
  } else {
    await fetchObjectPoolInfo()
  }
}

onMounted(async () => {
  await Promise.all([
    fetchCrashLogs(),
    fetchRequestLogs(),
    fetchUpTime(),
  ])
})

onBeforeUnmount(() => {
  stopUpTimeTimer()
})
</script>

<template>
  <main class="system-page">
    <section class="system-layout">
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>System</h2>
          <span class="filter-total">{{ getTabCount(activeTabInfo) }}</span>
        </div>

        <button
          v-for="tab in tabs"
          :key="tab.value"
          class="filter-card"
          :class="{ active: activeTab === tab.value }"
          @click="selectTab(tab.value)"
        >
          <span class="filter-content">
            <strong>{{ tab.label }}</strong>
            <small>{{ tab.description }}</small>
          </span>
        </button>
      </aside>

      <section class="system-panel">
        <div class="system-header">
          <div>
            <h1>{{ activeTabInfo.label }}</h1>
            <p>{{ activeTabInfo.description }}</p>
          </div>

          <div class="system-actions">
            <div class="up-time-box">
              <span class="up-time-label">Up Time</span>
              <span class="up-time-value">{{ upTime }}</span>
            </div>

            <button
              class="refresh-button"
              :disabled="loading"
              @click="refreshSystem"
            >
              ↻
            </button>
          </div>
        </div>

        <div v-if="loading" class="panel-message">
          Loading system data...
        </div>

        <div v-else-if="errorMessage" class="panel-message error-message">
          {{ errorMessage }}
        </div>

        <div v-else-if="activeTab === 'crash_log'" class="table-card">
          <table v-if="sortedCrashLogs.length > 0" class="crash-table">
            <colgroup>
              <col class="col-created" />
              <col class="col-env" />
              <col class="col-signal" />
              <col class="col-crash-function" />
              <col class="col-crash-line" />
              <col class="col-crash-file" />
            </colgroup>

            <thead>
              <tr>
                <th>Created At</th>
                <th>Env</th>
                <th>Signal</th>
                <th>Crash Function</th>
                <th>Line</th>
                <th>Crash File</th>
              </tr>
            </thead>

            <tbody>
              <template
                v-for="(log, index) in sortedCrashLogs"
                :key="getCrashId(log, index)"
              >
                <tr
                  class="table-row clickable-row"
                  :class="{ expanded: expandedCrashId === getCrashId(log, index) }"
                  @click="toggleCrashDetail(log, index)"
                >
                  <td class="mono-text" data-label="Created At">{{ log.created_at }}</td>
                  <td data-label="Env"><span class="type-badge">{{ log.env }}</span></td>
                  <td data-label="Signal"><span class="status-badge tone-reject">{{ log.signal }}</span></td>
                  <td class="mono-text function-cell" data-label="Crash Function">
                    {{ formatFunctionName(log.crash_function) }}
                  </td>
                  <td class="mono-text" data-label="Line">{{ log.crash_line || '–' }}</td>
                  <td class="mono-text function-cell" data-label="Crash File">
                    {{ formatFilePath(log.crash_file) }}
                  </td>
                </tr>

                <tr
                  v-if="expandedCrashId === getCrashId(log, index)"
                  class="crash-detail-row"
                >
                  <td colspan="6">
                    <div class="crash-detail-card">
                      <div class="crash-detail-grid">
                        <div class="crash-detail-block">
                          <span class="detail-label">Suspect Function</span>
                          <strong class="mono-text">{{ formatFunctionName(log.suspect_function) }}</strong>
                        </div>

                        <div class="crash-detail-block">
                          <span class="detail-label">Suspect File</span>
                          <strong class="mono-text">
                            {{ formatFilePath(log.suspect_file) }}:{{ log.suspect_line || '–' }}
                          </strong>
                        </div>

                        <div class="crash-detail-block">
                          <span class="detail-label">Core File</span>
                          <strong class="mono-text">{{ log.core_file || '–' }}</strong>
                        </div>

                        <div class="crash-detail-block">
                          <span class="detail-label">Core Size</span>
                          <strong class="mono-text">{{ formatBytes(log.core_file_size_bytes) }}</strong>
                        </div>
                      </div>

                      <div class="call-path-section">
                        <div class="call-path-title">Call Path</div>

                        <ol v-if="getUniqueCallPath(log.call_path).length > 0" class="call-path-list">
                          <li
                            v-for="frame in getUniqueCallPath(log.call_path)"
                            :key="`${frame.frame_index}-${frame.file}-${frame.line}-${frame.function}`"
                            class="call-path-item"
                          >
                            <div class="call-path-index mono-text">#{{ frame.frame_index }}</div>
                            <div class="call-path-content">
                              <div class="call-path-function mono-text">
                                {{ formatFunctionName(frame.function) }}
                              </div>
                              <div class="call-path-file mono-text">
                                {{ formatFilePath(frame.file) }}:{{ frame.line || '–' }}
                              </div>
                            </div>
                          </li>
                        </ol>

                        <div v-else class="empty-call-path">
                          No call path available.
                        </div>
                      </div>
                    </div>
                  </td>
                </tr>
              </template>
            </tbody>
          </table>

          <div v-else class="empty-table">
            No crash logs found.
          </div>
        </div>

        <div v-else-if="activeTab === 'request_log'" class="table-card">
          <table v-if="sortedRequestLogs.length > 0" class="request-table">
            <colgroup>
              <col class="col-request-time" />
              <col class="col-request-method" />
              <col class="col-request-endpoint" />
              <col class="col-request-status" />
              <col class="col-request-duration" />
              <col class="col-request-client-ip" />
              <col class="col-request-host" />
              <col class="col-request-user-agent" />
            </colgroup>

            <thead>
              <tr>
                <th>Start Time</th>
                <th>Method</th>
                <th>Endpoint</th>
                <th>Status</th>
                <th>Duration</th>
                <th>Client IP</th>
                <th>Host</th>
                <th>User Agent</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="(log, index) in sortedRequestLogs"
                :key="`${log.start_time}-${log.method}-${log.endpoint}-${log.response_status}-${index}`"
                class="table-row"
              >
                <td class="mono-text" data-label="Start Time">{{ log.start_time }}</td>
                <td data-label="Method">
                  <span class="method-badge" :class="getMethodTone(log.method)">
                    {{ log.method }}
                  </span>
                </td>
                <td class="mono-text function-cell" data-label="Endpoint">{{ log.endpoint }}</td>
                <td data-label="Status">
                  <span class="response-status-badge" :class="getResponseStatusTone(log.response_status)">
                    {{ formatResponseStatus(log.response_status) }}
                  </span>
                </td>
                <td class="mono-text" data-label="Duration">{{ formatDuration(log.duration) }}</td>
                <td class="mono-text" data-label="Client IP">{{ log.client_ip || '–' }}</td>
                <td class="mono-text" data-label="Host">{{ log.host || '–' }}</td>
                <td class="mono-text user-agent-cell" data-label="User Agent" :title="log.user_agent">
                  {{ log.user_agent || '–' }}
                </td>
              </tr>
            </tbody>
          </table>

          <div v-else class="empty-table">
            No request logs found.
          </div>
        </div>

        <div v-else-if="activeTab === 'object_pool'" class="object-pool-grid">
          <article
            v-for="item in objectPoolEntries"
            :key="item.name"
            class="object-pool-card"
          >
            <div class="object-pool-name">
              {{ item.name }}
            </div>

            <div class="object-pool-value">
              {{ formatNumber(item.size) }}
            </div>
          </article>

          <div v-if="objectPoolEntries.length === 0" class="empty-table">
            No object pool data found.
          </div>
        </div>

        <div class="table-footer">
          Showing {{ getTabCount(activeTabInfo) }} records
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.system-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.system-layout {
  display: grid;
  grid-template-columns: 220px minmax(860px, 1fr);
  gap: 12px;
}

.filter-panel,
.system-panel {
  background: #111827;
  border: 1px solid #374151;
  border-radius: 12px;
}

.filter-panel {
  min-height: 720px;
  padding: 14px 12px;
}

.system-panel {
  min-height: 720px;
  padding: 22px;
}

.filter-header,
.system-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.filter-header {
  margin-bottom: 18px;
}

.filter-header h2,
.system-header h1 {
  margin: 0;
  color: #ffffff;
  font-size: 18px;
  font-weight: 800;
  letter-spacing: -0.02em;
}

.system-header {
  margin-bottom: 18px;
}

.system-header p {
  margin: 5px 0 0;
  color: #9ca3af;
  font-size: 13px;
}

.system-actions {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.up-time-box {
  height: 42px;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 0 12px;
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
}

.up-time-label {
  color: #9ca3af;
  font-size: 12px;
  font-weight: 800;
}

.up-time-value {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 900;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
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
  padding: 10px 12px;
  margin-bottom: 12px;
  color: #e5e7eb;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 12px;
  text-align: left;
  cursor: pointer;
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
  color: #e5e7eb;
  font-size: 14px;
  font-weight: 800;
}

.filter-content small {
  color: #9ca3af;
  font-size: 12px;
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
  border-color: #7f1d1d;
}

.table-card {
  min-height: 420px;
  overflow-x: hidden;
  overflow-y: auto;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
}

table {
  width: 100%;
  border-collapse: collapse;
  table-layout: fixed;
}

.col-created { width: 13%; }
.col-env { width: 7%; }
.col-signal { width: 8%; }
.col-crash-function { width: 36%; }
.col-crash-line { width: 6%; }
.col-crash-file { width: 30%; }

.col-request-time { width: 14%; }
.col-request-method { width: 7%; }
.col-request-endpoint { width: 20%; }
.col-request-status { width: 13%; }
.col-request-duration { width: 10%; }
.col-request-client-ip { width: 12%; }
.col-request-host { width: 12%; }
.col-request-user-agent { width: 12%; }

th,
td {
  padding: 10px 12px;
  border-bottom: 1px solid #374151;
  vertical-align: top;
  white-space: normal;
  word-break: break-word;
  overflow-wrap: anywhere;
}

th {
  color: #9ca3af;
  background: #1f2937;
  font-size: 13px;
  font-weight: 800;
  text-align: left;
}

td {
  color: #f8fafc;
  font-size: 13px;
  text-align: left;
}

.crash-table td:nth-child(2),
.crash-table td:nth-child(3),
.crash-table td:nth-child(5) {
  text-align: center;
}

.request-table td:nth-child(1),
.request-table td:nth-child(3),
.request-table td:nth-child(5),
.request-table td:nth-child(6),
.request-table td:nth-child(7),
.request-table td:nth-child(8) {
  text-align: left;
}

.request-table td:nth-child(2),
.request-table td:nth-child(4) {
  text-align: center;
}

.table-row:hover {
  background: #273449;
}

.clickable-row {
  cursor: pointer;
}

.clickable-row.expanded {
  background: #273449;
}

.function-cell {
  line-height: 1.5;
  white-space: normal;
  word-break: break-word;
  overflow-wrap: anywhere;
}

.user-agent-cell {
  max-width: 260px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.status-badge,
.type-badge {
  min-width: 46px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 3px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.status-badge.tone-reject {
  color: #c084fc;
  background: #31263d;
  border: 1px solid #6b21a8;
}

.type-badge {
  color: #e5e7eb;
  background: #111827;
  border: 1px solid #374151;
}

.crash-detail-row td {
  padding: 0;
  background: #172033;
  border-bottom: 1px solid #374151;
}

.crash-detail-card {
  padding: 16px;
  background: #172033;
}

.crash-detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(240px, 1fr));
  gap: 12px;
  margin-bottom: 16px;
}

.crash-detail-block {
  padding: 12px;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
}

.detail-label {
  display: block;
  margin-bottom: 6px;
  color: #9ca3af;
  font-size: 11px;
  font-weight: 900;
  text-transform: uppercase;
  letter-spacing: 0.04em;
}

.call-path-section {
  padding: 12px;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
}

.call-path-title {
  margin-bottom: 10px;
  color: #f8fafc;
  font-size: 13px;
  font-weight: 900;
}

.call-path-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin: 0;
  padding: 0;
  list-style: none;
}

.call-path-item {
  display: grid;
  grid-template-columns: 48px 1fr;
  gap: 10px;
  padding: 10px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 8px;
}

.call-path-index {
  color: #93c5fd;
  font-weight: 900;
}

.call-path-function {
  color: #f8fafc;
  line-height: 1.45;
}

.call-path-file {
  margin-top: 4px;
  color: #9ca3af;
  font-size: 12px;
  line-height: 1.45;
}

.empty-call-path {
  color: #9ca3af;
  font-size: 12px;
}

.method-badge,
.response-status-badge {
  min-width: 58px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 3px 8px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 900;
  text-transform: uppercase;
}

.response-status-badge {
  min-width: 108px;
  white-space: nowrap;
}

.method-get {
  color: #93c5fd;
  background: #172554;
  border: 1px solid #2563eb;
}

.method-post {
  color: #86efac;
  background: #14532d;
  border: 1px solid #16a34a;
}

.method-put,
.method-patch {
  color: #fde68a;
  background: #451a03;
  border: 1px solid #d97706;
}

.method-delete {
  color: #fca5a5;
  background: #450a0a;
  border: 1px solid #dc2626;
}

.method-options,
.method-head {
  color: #c4b5fd;
  background: #2e1065;
  border: 1px solid #7c3aed;
}

.response-success {
  color: #86efac;
  background: #14532d;
  border: 1px solid #16a34a;
}

.response-warning {
  color: #fde68a;
  background: #451a03;
  border: 1px solid #d97706;
}

.response-not-found {
  color: #facc15;
  background: rgba(120, 53, 15, 0.35);
  border: 1px solid rgba(180, 83, 9, 0.45);
}

.response-risk {
  color: #c4b5fd;
  background: #2e1065;
  border: 1px solid #7c3aed;
}

.response-error {
  color: #fecaca;
  background: #7f1d1d;
  border: 1px solid #ef4444;
}

.response-unknown {
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #4b5563;
}

.object-pool-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(260px, 1fr));
  gap: 14px;
}

.object-pool-card {
  min-height: 120px;
  padding: 18px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 10px;
}

.object-pool-name {
  color: #9ca3af;
  font-size: 13px;
  font-weight: 800;
  line-height: 1.35;
}

.object-pool-value {
  margin-top: 18px;
  color: #f8fafc;
  font-size: 28px;
  font-weight: 900;
  letter-spacing: -0.03em;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
}

.mono-text {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
  line-height: 1.5;
}

.table-footer {
  margin-top: 18px;
  color: #9ca3af;
  font-size: 13px;
}

@media (max-width: 1050px) {
  .system-layout,
  .object-pool-grid {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .system-panel {
    min-height: unset;
  }

  .crash-detail-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 760px) {
  .system-layout {
    grid-template-columns: 1fr;
  }

  .system-panel,
  .filter-panel {
    padding: 16px;
    min-height: unset;
  }

  .system-header {
    align-items: flex-start;
    gap: 12px;
  }

  .system-actions {
    flex-shrink: 0;
  }

  .up-time-box {
    padding: 0 10px;
  }

  .table-card {
    overflow: visible;
    border: none;
    background: transparent;
  }

  table,
  thead,
  tbody,
  tr,
  th,
  td {
    display: block;
    width: 100%;
  }

  thead {
    display: none;
  }

  .table-row {
    margin-bottom: 14px;
    padding: 14px;
    background: #1f2937;
    border: 1px solid #374151;
    border-radius: 12px;
  }

  td {
    display: grid;
    grid-template-columns: 110px 1fr;
    gap: 10px;
    padding: 8px 0;
    border-bottom: 1px solid #374151;
    text-align: left !important;
  }

  td:last-child {
    border-bottom: none;
  }

  td::before {
    color: #9ca3af;
    font-weight: 800;
  }

  .crash-table td::before,
  .request-table td::before {
    content: attr(data-label);
  }

  .crash-detail-row {
    margin: -8px 0 14px;
  }

  .crash-detail-row td {
    display: block;
    padding: 0;
  }

  .crash-detail-row td::before {
    display: none;
  }

  .function-cell,
  .mono-text {
    word-break: break-word;
    overflow-wrap: anywhere;
  }

  .user-agent-cell {
    max-width: none;
    white-space: normal;
  }

  .object-pool-grid {
    grid-template-columns: 1fr;
  }
}
</style>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

type CrashLog = {
  app: string
  env: string
  exit_code: number
  signal: string
  crash_function: string
  crash_line: string
  caller: string
  caller_line: string
  core_file_size: string
  host: string
  created_at: string
  created_at_ns: number | string
}

type CrashLogResponse = {
  error: boolean
  status_code: number
  msg: string
  data: CrashLog[]
}

type SystemTab = {
  label: string
  value: 'crash_log' | 'object_pool'
  description: string
}

const authStore = useAuthStore()

const tabs: SystemTab[] = [
  {
    label: 'Crash Log',
    value: 'crash_log',
    description: 'Runtime crash reports',
  },
  {
    label: 'Object Pool',
    value: 'object_pool',
    description: 'Pool metrics',
  },
]

const activeTab = ref<SystemTab['value']>('crash_log')
const loading = ref(false)
const errorMessage = ref('')
const crashLogs = ref<CrashLog[]>([])

const activeTabInfo = computed(() => {
  return tabs.find((tab) => tab.value === activeTab.value) ?? tabs[0]!
})

const sortedCrashLogs = computed(() => {
  return [...crashLogs.value].sort((left, right) => {
    return Number(right.created_at_ns) - Number(left.created_at_ns)
  })
})

function getTabCount(tab: SystemTab) {
  if (tab.value === 'crash_log') {
    return crashLogs.value.length
  }

  return 0
}

function selectTab(tab: SystemTab['value']) {
  activeTab.value = tab
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

function refreshSystem() {
  if (activeTab.value === 'crash_log') {
    fetchCrashLogs()
  }
}

onMounted(() => {
  fetchCrashLogs()
})
</script>

<template>
  <main class="system-page">
    <section class="system-layout">
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>System</h2>

          <span class="filter-total">
            {{ getTabCount(activeTabInfo) }}
          </span>
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

          <button
            class="refresh-button"
            :disabled="loading"
            @click="refreshSystem"
          >
            ↻
          </button>
        </div>

        <div v-if="loading" class="panel-message">
          Loading system data...
        </div>

        <div
          v-else-if="errorMessage"
          class="panel-message error-message"
        >
          {{ errorMessage }}
        </div>

        <div
          v-else-if="activeTab === 'crash_log'"
          class="table-card"
        >
          <table v-if="sortedCrashLogs.length > 0">
            <colgroup>
              <col class="col-created" />
              <col class="col-env" />
              <col class="col-signal" />
              <col class="col-exit" />
              <col class="col-function" />
              <col class="col-line" />
              <col class="col-caller" />
              <col class="col-caller-line" />
              <col class="col-core" />
              <col class="col-host" />
            </colgroup>

            <thead>
              <tr>
                <th>Created At</th>
                <th>Env</th>
                <th>Signal</th>
                <th>Exit</th>
                <th>Crash Function</th>
                <th>Line</th>
                <th>Caller</th>
                <th>Caller Line</th>
                <th>Core</th>
                <th>Host</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="log in sortedCrashLogs"
                :key="String(log.created_at_ns)"
                class="table-row"
              >
                <td class="mono-text">
                  {{ log.created_at }}
                </td>

                <td>
                  <span class="type-badge">
                    {{ log.env }}
                  </span>
                </td>

                <td>
                  <span class="status-badge tone-reject">
                    {{ log.signal }}
                  </span>
                </td>

                <td>
                  {{ log.exit_code }}
                </td>

                <td class="mono-text function-cell">
                  {{ log.crash_function }}
                </td>

                <td>
                  {{ log.crash_line || '–' }}
                </td>

                <td class="mono-text function-cell">
                  {{ log.caller }}
                </td>

                <td>
                  {{ log.caller_line || '–' }}
                </td>

                <td>
                  {{ log.core_file_size }}
                </td>

                <td class="mono-text">
                  {{ log.host }}
                </td>
              </tr>
            </tbody>
          </table>

          <div
            v-else
            class="empty-table"
          >
            No crash logs found.
          </div>
        </div>

        <div
          v-else
          class="empty-table"
        >
          Object pool metrics are not available yet.
        </div>

        <div class="table-footer">
          Showing
          {{
            activeTab === 'crash_log'
              ? sortedCrashLogs.length
              : 0
          }}
          records
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.system-page {
  min-height: 100%;
  color: #f8fafc;
  font-family:
    Inter,
    ui-sans-serif,
    system-ui,
    -apple-system,
    BlinkMacSystemFont,
    'Segoe UI',
    sans-serif;
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

.col-created {
  width: 10%;
}

.col-env {
  width: 6%;
}

.col-signal {
  width: 7%;
}

.col-exit {
  width: 4%;
}

.col-function {
  width: 30%;
}

.col-line {
  width: 4%;
}

.col-caller {
  width: 28%;
}

.col-caller-line {
  width: 6%;
}

.col-core {
  width: 5%;
}

.col-host {
  width: 8%;
}

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

td:nth-child(2),
td:nth-child(3),
td:nth-child(4),
td:nth-child(6),
td:nth-child(8),
td:nth-child(9) {
  text-align: center;
}

.table-row:hover {
  background: #273449;
}

.function-cell {
  line-height: 1.5;

  white-space: normal;
  word-break: break-word;
  overflow-wrap: anywhere;
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

.mono-text {
  font-family:
    ui-monospace,
    SFMono-Regular,
    Menlo,
    Monaco,
    Consolas,
    'Liberation Mono',
    'Courier New',
    monospace;

  line-height: 1.5;
}

.table-footer {
  margin-top: 18px;

  color: #9ca3af;
  font-size: 13px;
}

@media (max-width: 1050px) {
  .system-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .system-panel {
    min-height: unset;
  }
}
</style>
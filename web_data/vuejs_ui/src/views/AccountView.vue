<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { API_BASE_URL } from '@/config/env'
import { useAuthStore } from '@/stores/auth'

const auth = useAuthStore()

type AccountFieldConfig = {
  exchange_id: string
  field_names: string[]
}

type AccountValue = string | boolean

type Account = {
  exchange_id: string
  key: string
  [fieldName: string]: AccountValue
}

type AccountFieldNameListResponse = {
  error: boolean
  status_code: number
  msg: string
  data: AccountFieldConfig[]
}

type AccountListResponse = {
  error: boolean
  status_code: number
  msg: string
  data: Account[]
}

type AddAccountResponse = {
  error: boolean
  status_code: number
  msg: string
  data: null | string
}

type SetActiveAccountResponse = {
  error: boolean
  status_code: number
  msg: string
  data: string
}

type SortKey = 'exchange_id' | 'key'
type SortDirection = 'asc' | 'desc'
type NoticeTone = 'success' | 'error' | 'info'

const loading = ref(false)
const submitting = ref(false)
const updatingActiveKey = ref<string | null>(null)
const detailNoticeMessage = ref('')
const detailNoticeTone = ref<NoticeTone>('info')
const errorMessage = ref('')
const noticeMessage = ref('')
const noticeTone = ref<NoticeTone>('info')
const fieldConfigs = ref<AccountFieldConfig[]>([])
const accounts = ref<Account[]>([])
const selectedExchangeId = ref('')
const selectedFilterExchangeId = ref('ALL')
const selectedAccount = ref<Account | null>(null)
const viewMode = ref<'list' | 'create'>('list')
const formValues = ref<Record<string, string | boolean>>({})
const sortKey = ref<SortKey | null>(null)
const sortDirection = ref<SortDirection>('asc')

type ExchangeFilter = {
  value: string
  label: string
  count: number
}

const exchangeFilters = computed<ExchangeFilter[]>(() => {
  const filters: ExchangeFilter[] = [
    {
      value: 'ALL',
      label: 'All',
      count: accounts.value.length,
    },
  ]

  fieldConfigs.value.forEach((config) => {
    filters.push({
      value: config.exchange_id,
      label: config.exchange_id,
      count: accounts.value.filter(
        (account) => account.exchange_id === config.exchange_id,
      ).length,
    })
  })

  accounts.value.forEach((account) => {
    if (!filters.some((filter) => filter.value === account.exchange_id)) {
      filters.push({
        value: account.exchange_id,
        label: account.exchange_id,
        count: accounts.value.filter(
          (currentAccount) => currentAccount.exchange_id === account.exchange_id,
        ).length,
      })
    }
  })

  return filters
})

const filteredAccounts = computed(() => {
  if (selectedFilterExchangeId.value === 'ALL') {
    return accounts.value
  }

  return accounts.value.filter(
    (account) => account.exchange_id === selectedFilterExchangeId.value,
  )
})

const activeFilterInfo = computed(() => {
  return exchangeFilters.value.find(
    (filter) => filter.value === selectedFilterExchangeId.value,
  ) ?? exchangeFilters.value[0]
})

const selectedFieldConfig = computed(() => {
  return fieldConfigs.value.find(
    (config) => config.exchange_id === selectedExchangeId.value,
  )
})

const selectedFieldNames = computed(() => {
  return selectedFieldConfig.value?.field_names ?? []
})

const visibleInputFieldNames = computed(() => {
  return selectedFieldNames.value.filter((fieldName) => fieldName !== 'exchange_id')
})

const sortedAccounts = computed(() => {
  if (!sortKey.value) {
    return filteredAccounts.value
  }

  return [...filteredAccounts.value].sort((left, right) => {
    const result = String(left[sortKey.value!] ?? '').localeCompare(
      String(right[sortKey.value!] ?? ''),
      undefined,
      {
        numeric: true,
        sensitivity: 'base',
      },
    )

    return sortDirection.value === 'asc' ? result : -result
  })
})

const isDetailOpen = computed(() => selectedAccount.value !== null)

const selectedExchangeAccountCount = computed(() => {
  if (!selectedExchangeId.value) {
    return accounts.value.length
  }

  return accounts.value.filter(
    (account) => account.exchange_id === selectedExchangeId.value,
  ).length
})

const canSubmit = computed(() => {
  if (!selectedExchangeId.value || submitting.value) {
    return false
  }

  return visibleInputFieldNames.value.every((fieldName) => {
    if (isBooleanField(fieldName)) {
      return typeof formValues.value[fieldName] === 'boolean'
    }

    return String(formValues.value[fieldName] ?? '').trim().length > 0
  })
})

function sortAccounts(key: SortKey) {
  if (sortKey.value === key) {
    sortDirection.value = sortDirection.value === 'asc' ? 'desc' : 'asc'
    return
  }

  sortKey.value = key
  sortDirection.value = 'asc'
}

function selectExchange(exchangeId: string) {
  selectedExchangeId.value = exchangeId
  noticeMessage.value = ''
  formValues.value = {
    exchange_id: exchangeId,
  }

  visibleInputFieldNames.value.forEach((fieldName) => {
    formValues.value[fieldName] = initialFieldValue(fieldName)
  })
}

function selectFilter(exchangeId: string) {
  selectedFilterExchangeId.value = exchangeId

  if (
    selectedAccount.value &&
    exchangeId !== 'ALL' &&
    selectedAccount.value.exchange_id !== exchangeId
  ) {
    selectedAccount.value = null
  }
}

function openCreateAccount() {
  selectedAccount.value = null
  noticeMessage.value = ''
  viewMode.value = 'create'

  if (!selectedExchangeId.value && fieldConfigs.value.length > 0) {
    selectExchange(fieldConfigs.value[0]!.exchange_id)
  }
}

function backToAccountList() {
  viewMode.value = 'list'
  noticeMessage.value = ''
}

function selectAccount(account: Account) {
  selectedAccount.value = account
  detailNoticeMessage.value = ''
}

function closeDetail() {
  selectedAccount.value = null
  detailNoticeMessage.value = ''
}

function maskSecret(value: AccountValue | null | undefined) {
  const text = String(value ?? '')

  if (!text) {
    return '–'
  }

  if (text.length <= 10) {
    return '•'.repeat(text.length)
  }

  return `${text.slice(0, 6)}••••••${text.slice(-4)}`
}

function formatLabel(fieldName: string) {
  return fieldName
    .split('_')
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join(' ')
}

function isSecretField(fieldName: string) {
  return fieldName.toLowerCase().includes('secret') ||
    fieldName.toLowerCase().includes('passphrase') ||
    fieldName.toLowerCase().includes('api_key')
}

function isBooleanField(fieldName: string) {
  return fieldName.toLowerCase() === 'is_active'
}

function initialFieldValue(fieldName: string) {
  return isBooleanField(fieldName) ? false : ''
}

function accountIsActive(account: Account) {
  return account.is_active === true
}

function statusLabel(account: Account) {
  return accountIsActive(account) ? 'Active' : 'Inactive'
}

function selectedAccountStatusText(account: Account) {
  return accountIsActive(account) ? 'This account is currently active.' : 'This account is currently inactive.'
}

function setDetailNotice(message: string, tone: NoticeTone) {
  detailNoticeMessage.value = message
  detailNoticeTone.value = tone
}

function setNotice(message: string, tone: NoticeTone) {
  noticeMessage.value = message
  noticeTone.value = tone
}

async function fetchFieldConfigs() {
  const response = await fetch(`${API_BASE_URL}/account_field_name_list`, {
    method: 'GET',
    credentials: 'include',
  })

  const result: AccountFieldNameListResponse = await response.json()

  if (response.status === 401 || response.status === 403) {
    auth.logout()
    return
  }

  if (!response.ok || result.error) {
    throw new Error(result.msg || 'Failed to fetch account field name list.')
  }

  fieldConfigs.value = result.data ?? []

  if (!selectedExchangeId.value && fieldConfigs.value.length > 0) {
    selectExchange(fieldConfigs.value[0]!.exchange_id)
  }
}

async function fetchAccounts() {
  const response = await fetch(`${API_BASE_URL}/account_list`, {
    method: 'GET',
    credentials: 'include',
  })

  const result: AccountListResponse = await response.json()

  if (response.status === 401 || response.status === 403) {
    auth.logout()
    return
  }

  if (!response.ok || result.error) {
    throw new Error(result.msg || 'Failed to fetch account list.')
  }

  accounts.value = result.data ?? []

  if (
    selectedAccount.value &&
    !accounts.value.some(
      (account) => account.key === selectedAccount.value?.key &&
        account.exchange_id === selectedAccount.value?.exchange_id,
    )
  ) {
    selectedAccount.value = null
  }
}

async function startAccountView() {
  loading.value = true
  errorMessage.value = ''

  try {
    await Promise.all([
      fetchFieldConfigs(),
      fetchAccounts(),
    ])
  } catch (error) {
    console.error('Start account view error:', error)
    errorMessage.value = error instanceof Error ? error.message : 'Start account view error.'
  } finally {
    loading.value = false
  }
}

async function refreshAccounts() {
  await startAccountView()
}

async function addAccount() {
  if (!canSubmit.value) {
    setNotice('Please fill all required account fields.', 'error')
    return
  }

  submitting.value = true
  setNotice('', 'info')

  try {
    const payload: Record<string, string | boolean> = {
      ...formValues.value,
      exchange_id: selectedExchangeId.value,
    }

    const response = await fetch(`${API_BASE_URL}/add_account`, {
      method: 'POST',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(payload),
    })

    const result: AddAccountResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      setNotice(result.msg || 'Add account failed.', 'error')
      return
    }

    setNotice(result.msg || 'Account registered successfully.', 'success')
    clearForm(false)
    await fetchAccounts()
  } catch (error) {
    console.error('Add account error:', error)
    setNotice('Add account error.', 'error')
  } finally {
    submitting.value = false
  }
}


async function setActiveAccount(account: Account, isActive: boolean) {
  if (updatingActiveKey.value) {
    return
  }

  const previousValue = account.is_active === true
  updatingActiveKey.value = account.key
  setDetailNotice('', 'info')

  try {
    const response = await fetch(`${API_BASE_URL}/set_active_account`, {
      method: 'POST',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        key: account.key,
        is_active: isActive,
      }),
    })

    const result: SetActiveAccountResponse = await response.json()

    if (response.status === 401 || response.status === 403) {
      auth.logout()
      return
    }

    if (!response.ok || result.error) {
      setDetailNotice(result.msg || 'Update account status failed.', 'error')
      account.is_active = previousValue
      return
    }

    accounts.value = accounts.value.map((currentAccount) => {
      if (
        currentAccount.key === account.key &&
        currentAccount.exchange_id === account.exchange_id
      ) {
        return {
          ...currentAccount,
          is_active: isActive,
        }
      }

      return currentAccount
    })

    if (
      selectedAccount.value?.key === account.key &&
      selectedAccount.value.exchange_id === account.exchange_id
    ) {
      selectedAccount.value = {
        ...selectedAccount.value,
        is_active: isActive,
      }
    }

    setDetailNotice(result.msg || 'Account status updated successfully.', 'success')
  } catch (error) {
    console.error('Set active account error:', error)
    account.is_active = previousValue
    setDetailNotice('Set active account error.', 'error')
  } finally {
    updatingActiveKey.value = null
  }
}

function handleActiveToggle(account: Account, event: Event) {
  const input = event.target as HTMLInputElement
  setActiveAccount(account, input.checked)
}

function clearForm(clearNotice = true) {
  const exchangeId = selectedExchangeId.value
  formValues.value = {
    exchange_id: exchangeId,
  }

  visibleInputFieldNames.value.forEach((fieldName) => {
    formValues.value[fieldName] = initialFieldValue(fieldName)
  })

  if (clearNotice) {
    noticeMessage.value = ''
  }
}

function accountFields(account: Account) {
  return Object.keys(account).filter((fieldName) => fieldName !== 'exchange_id')
}

onMounted(() => {
  startAccountView()
})
</script>

<template>
  <main class="accounts-page">
    <section
      v-if="viewMode === 'list'"
      class="accounts-layout"
      :class="{ 'detail-open': isDetailOpen }"
    >
      <aside class="filter-panel">
        <div class="filter-header">
          <h2>Filters</h2>
          <span class="filter-total">{{ filteredAccounts.length }}</span>
        </div>

        <button
          v-for="filter in exchangeFilters"
          :key="filter.value"
          class="filter-card"
          :class="{ active: selectedFilterExchangeId === filter.value, 'tone-all': filter.value === 'ALL', 'tone-exchange': filter.value !== 'ALL' }"
          @click="selectFilter(filter.value)"
        >
          <span class="filter-content">
            <strong>{{ filter.label }}</strong>
            <small>{{ filter.count }} accounts</small>
          </span>
        </button>
      </aside>

      <section class="accounts-panel">
        <div class="accounts-header">
          <div>
            <h1>Accounts</h1>
            <p>{{ activeFilterInfo?.label ?? 'All' }} accounts</p>
          </div>

          <div class="header-actions">
            <button
              class="create-account-button"
              type="button"
              @click="openCreateAccount"
            >
              + New Account
            </button>

            <button
              class="refresh-button"
              :disabled="loading"
              @click="refreshAccounts"
            >
              ↻
            </button>
          </div>
        </div>

        <div
          v-if="loading"
          class="panel-message"
        >
          Loading accounts...
        </div>

        <div
          v-else-if="errorMessage"
          class="panel-message error-message"
        >
          {{ errorMessage }}
        </div>

        <div
          v-else
          class="accounts-table-card"
        >
          <table v-if="filteredAccounts.length > 0">
            <thead>
              <tr>
                <th class="sortable-header" @click="sortAccounts('exchange_id')">Exchange</th>
                <th class="sortable-header" @click="sortAccounts('key')">Account Key</th>
                <th>API Key</th>
                <th v-if="!isDetailOpen">API Secret</th>
                <th v-if="!isDetailOpen">Is Active</th>
              </tr>
            </thead>

            <tbody>
              <tr
                v-for="account in sortedAccounts"
                :key="`${account.exchange_id}-${account.key}`"
                class="account-row"
                :class="{
                  selected: selectedAccount?.key === account.key &&
                    selectedAccount?.exchange_id === account.exchange_id,
                }"
                @click="selectAccount(account)"
              >
                <td>
                  <span class="exchange-badge">{{ account.exchange_id }}</span>
                </td>

                <td>
                  <div class="account-key-cell">
                    <strong>{{ account.key }}</strong>
                    <small>{{ account.exchange_id }}</small>
                  </div>
                </td>

                <td class="mono-text">{{ maskSecret(account.api_key ?? '') }}</td>
                <td v-if="!isDetailOpen" class="mono-text">{{ maskSecret(account.api_secret ?? '') }}</td>
                <td v-if="!isDetailOpen">
                  <span
                    class="status-badge"
                    :class="accountIsActive(account) ? 'active' : 'inactive'"
                  >
                    <span class="status-dot"></span>
                    {{ statusLabel(account) }}
                  </span>
                </td>
              </tr>
            </tbody>
          </table>

          <div
            v-else
            class="empty-table"
          >
            No accounts found.
          </div>
        </div>

        <div class="table-footer">
          Showing {{ filteredAccounts.length > 0 ? 1 : 0 }} to {{ filteredAccounts.length }} of
          {{ filteredAccounts.length }} accounts
        </div>
      </section>

      <aside v-if="selectedAccount" class="detail-panel">
        <div class="account-detail">
          <div class="detail-header">
            <div>
              <h2>Account Details</h2>
              <p>
                <span class="mono-text">{{ selectedAccount.key }}</span>
                <span class="exchange-badge">{{ selectedAccount.exchange_id }}</span>
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
            <h3>Account Info</h3>

            <div class="detail-row">
              <span>Exchange</span>
              <strong class="exchange-text">{{ selectedAccount.exchange_id }}</strong>
            </div>

            <div class="detail-row">
              <span>Account Key</span>
              <strong class="mono-text">{{ selectedAccount.key }}</strong>
            </div>
          </section>

          <section class="detail-card">
            <h3>Credentials</h3>

            <div
              v-for="fieldName in accountFields(selectedAccount)"
              :key="fieldName"
              class="detail-row"
            >
              <span>{{ formatLabel(fieldName) }}</span>
              <strong v-if="isBooleanField(fieldName)">
                <span
                  class="status-badge"
                  :class="selectedAccount[fieldName] === true ? 'active' : 'inactive'"
                >
                  <span class="status-dot"></span>
                  {{ selectedAccount[fieldName] === true ? 'Active' : 'Inactive' }}
                </span>
              </strong>
              <strong v-else class="mono-text">
                {{ isSecretField(fieldName) ? maskSecret(selectedAccount[fieldName] ?? '') : (selectedAccount[fieldName] ?? '–') }}
              </strong>
            </div>
          </section>

          <section class="detail-card status-management-card">
            <h3>Account Status</h3>

            <div class="status-management-content">
              <div>
                <span class="status-caption">Current Status</span>
                <strong>{{ selectedAccountStatusText(selectedAccount) }}</strong>
              </div>

              <label
                class="boolean-field status-toggle-field"
                :class="{ disabled: updatingActiveKey === selectedAccount.key }"
              >
                <input
                  class="boolean-native-input"
                  type="checkbox"
                  :checked="accountIsActive(selectedAccount)"
                  :disabled="updatingActiveKey === selectedAccount.key"
                  @change="handleActiveToggle(selectedAccount, $event)"
                >
                <span
                  class="boolean-switch"
                  :class="{ checked: accountIsActive(selectedAccount) }"
                  aria-hidden="true"
                >
                  <span class="boolean-switch-thumb"></span>
                </span>
                <span class="boolean-status">
                  {{ updatingActiveKey === selectedAccount.key ? 'Updating...' : statusLabel(selectedAccount) }}
                </span>
              </label>
            </div>

            <div
              v-if="detailNoticeMessage"
              class="notice-card detail-notice"
              :class="`tone-${detailNoticeTone}`"
            >
              {{ detailNoticeMessage }}
            </div>
          </section>
        </div>
      </aside>
    </section>

    <section
      v-else
      class="create-layout"
    >
      <section class="create-panel">
        <div class="accounts-header">
          <div>
            <h1>Create Account</h1>
            <p>Select exchange and enter required credentials</p>
          </div>

          <button
            class="back-button"
            type="button"
            @click="backToAccountList"
          >
            ← Accounts
          </button>
        </div>

        <div class="create-content-card">
          <div class="create-form-grid">
            <div class="form-group">
              <label class="field-label">Exchange</label>
              <select
                v-model="selectedExchangeId"
                class="form-control"
                @change="selectExchange(selectedExchangeId)"
              >
                <option
                  v-for="config in fieldConfigs"
                  :key="config.exchange_id"
                  :value="config.exchange_id"
                >
                  {{ config.exchange_id }}
                </option>
              </select>
            </div>

            <div class="exchange-summary create-summary">
              <strong>{{ selectedExchangeId || 'No Exchange' }}</strong>
              <small>{{ selectedExchangeAccountCount }} accounts</small>
            </div>
          </div>

          <form class="account-form create-account-form" @submit.prevent="addAccount">
            <div
              v-for="fieldName in visibleInputFieldNames"
              :key="fieldName"
              class="form-group"
            >
              <label class="field-label">{{ formatLabel(fieldName) }}</label>
              <label
                v-if="isBooleanField(fieldName)"
                class="boolean-field"
              >
                <input
                  v-model="formValues[fieldName]"
                  class="boolean-native-input"
                  type="checkbox"
                >
                <span
                  class="boolean-switch"
                  :class="{ checked: formValues[fieldName] === true }"
                  aria-hidden="true"
                >
                  <span class="boolean-switch-thumb"></span>
                </span>
                <span class="boolean-status">{{ formValues[fieldName] === true ? 'Active' : 'Inactive' }}</span>
              </label>

              <input
                v-else
                v-model="formValues[fieldName]"
                class="form-control mono-text"
                :type="isSecretField(fieldName) ? 'password' : 'text'"
                :placeholder="fieldName"
                autocomplete="off"
              >
            </div>

            <div
              v-if="noticeMessage"
              class="notice-card"
              :class="`tone-${noticeTone}`"
            >
              {{ noticeMessage }}
            </div>

            <div class="form-actions">
              <button
                class="submit-button"
                :disabled="!canSubmit"
                type="submit"
              >
                {{ submitting ? 'Creating...' : 'Create Account' }}
              </button>

              <button
                class="clear-button"
                type="button"
                @click="clearForm()"
              >
                Clear
              </button>
            </div>
          </form>
        </div>
      </section>
    </section>
  </main>
</template>

<style scoped>
.accounts-page {
  min-height: 100%;
  color: #f8fafc;
  font-family: Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
}

.accounts-layout {
  display: grid;
  grid-template-columns: 300px minmax(860px, 1fr);
  gap: 12px;
}

.accounts-layout.detail-open {
  grid-template-columns: 300px minmax(760px, 1fr) 430px;
}

.filter-panel,
.accounts-panel,
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

.accounts-panel {
  min-height: 720px;
  padding: 22px;
}

.detail-panel {
  min-height: 720px;
  padding: 22px;
}

.filter-header,
.accounts-header,
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
.accounts-header h1,
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

.filter-card.tone-all .filter-content strong {
  color: #60a5fa;
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

.create-layout {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: calc(100vh - 140px);
}

.create-panel {
  width: min(900px, 100%);
  min-height: 720px;
  padding: 22px;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 12px;
}

.create-content-card {
  max-width: 720px;
  padding: 18px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 12px;
}

.create-form-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 14px;
}

.create-summary {
  margin: 0;
}

.create-account-form {
  max-width: 520px;
}

.form-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}

.create-card {
  padding: 14px;
  background: #1f2937;
  border: 1px solid #374151;
  border-radius: 12px;
}

.exchange-summary {
  min-height: 72px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 4px;
  padding: 10px 12px;
  margin: 12px 0;
  color: #e5e7eb;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
  border-radius: 12px;
}

.exchange-summary strong {
  color: #60a5fa;
  font-size: 14px;
  font-weight: 900;
}

.exchange-summary small {
  color: #9ca3af;
  font-size: 12px;
  font-weight: 700;
}

.account-form {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 6px;

  width: 100%;
}

.field-label {
  color: #9ca3af;
  font-size: 12px;
  font-weight: 800;
}

.form-control {
  width: 100%;
  min-height: 40px;
  padding: 8px 10px;
  color: #f8fafc;
  background: #111827;
  border: 1px solid #374151;
  border-radius: 9px;
  outline: none;
  font-size: 13px;
  font-weight: 700;

}

.form-control:focus {
  border-color: #3b82f6;
  background: #111827;
}

.boolean-field {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  width: fit-content;
  min-height: 30px;
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
  cursor: pointer;
  user-select: none;
}

.boolean-native-input {
  position: absolute;
  width: 1px;
  height: 1px;
  opacity: 0;
  pointer-events: none;
}

.boolean-switch {
  width: 44px;
  height: 24px;
  display: inline-flex;
  align-items: center;
  padding: 2px;
  background: #111827;
  border: 1px solid #4b5563;
  border-radius: 999px;
  box-sizing: border-box;
}

.boolean-switch.checked {
  background: #1e3a5f;
  border-color: #3b82f6;
}

.boolean-switch-thumb {
  width: 18px;
  height: 18px;
  display: block;
  background: #9ca3af;
  border-radius: 999px;
}

.boolean-switch.checked .boolean-switch-thumb {
  transform: translateX(20px);
  background: #60a5fa;
}

.boolean-status {
  color: #cbd5e1;
}


.status-management-card {
  margin-top: 2px;
}

.status-management-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.status-management-content > div {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.status-caption {
  color: #9ca3af;
  font-size: 12px;
  font-weight: 800;
}

.status-management-content strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
}

.status-toggle-field {
  flex-shrink: 0;
}

.status-toggle-field.disabled {
  cursor: not-allowed;
  opacity: 0.65;
}

.detail-notice {
  margin-top: 12px;
}

.notice-card {
  padding: 10px 12px;
  border-radius: 9px;
  font-size: 12px;
  font-weight: 800;
  line-height: 1.45;
}

.notice-card.tone-success {
  color: #34d399;
  background: #1f3a35;
  border: 1px solid #2f6f5f;
}

.notice-card.tone-error {
  color: #fca5a5;
  background: #3b2b2b;
  border: 1px solid #7f1d1d;
}

.notice-card.tone-info {
  color: #bfdbfe;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
}

.submit-button,
.clear-button {
  width: 100%;
  min-height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 900;
  cursor: pointer;
  transition: none;
}

.submit-button {
  color: #ffffff;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
}

.submit-button:hover:not(:disabled) {
  background: #244b7d;
}

.submit-button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.clear-button {
  color: #cbd5e1;
  background: #111827;
  border: 1px solid #374151;
}

.clear-button:hover {
  color: #ffffff;
  background: #273449;
  border-color: #4b5563;
}

.accounts-header {
  margin-bottom: 18px;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 10px;
}

.create-account-button,
.back-button {
  min-height: 42px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 0 14px;
  color: #ffffff;
  background: #1e3a5f;
  border: 1px solid #3b82f6;
  border-radius: 9px;
  font-size: 13px;
  font-weight: 900;
  cursor: pointer;
  transition: none;
}

.create-account-button:hover,
.back-button:hover {
  background: #244b7d;
}

.accounts-header p {
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

.accounts-table-card {
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
td:first-child,
th:nth-child(2),
td:nth-child(2) {
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

.account-row {
  cursor: pointer;
  transition: none;
}

.account-row:hover {
  background: #273449;
}

.account-row.selected {
  background: #1e3a5f;
}

.account-key-cell {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.account-key-cell strong {
  color: #ffffff;
  font-size: 13px;
  font-weight: 900;
}

.account-key-cell small {
  color: #9ca3af;
  font-size: 11px;
  font-weight: 700;
}

.exchange-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 70px;
  padding: 3px 8px;
  color: #facc15;
  background: #3a3320;
  border: 1px solid #8a6d1f;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 800;
}

.exchange-text {
  color: #facc15;
}

.status-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 7px;
  min-width: 82px;
  padding: 6px 10px;
  border-radius: 8px;
  font-size: 12px;
  font-weight: 800;
  line-height: 1;
}

.status-badge.active {
  color: #4ade80;
  background: #123c29;
  border: 1px solid #237a45;
}

.status-badge.inactive {
  color: #f87171;
  background: #3a1f24;
  border: 1px solid #7f1d1d;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 999px;
  background: currentColor;
  box-shadow: 0 0 8px currentColor;
}

.mono-text {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace;
}

.table-footer {
  margin-top: 18px;
  color: #9ca3af;
  font-size: 13px;
}

.account-detail {
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

.detail-row strong {
  color: #f8fafc;
  font-size: 13px;
  font-weight: 800;
  text-align: right;
  word-break: break-all;
}

@media (max-width: 1500px) {
  .accounts-layout {
    grid-template-columns: 300px minmax(620px, 1fr);
  }

  .detail-panel {
    grid-column: 1 / -1;
  }
}

@media (max-width: 1050px) {
  .accounts-layout {
    grid-template-columns: 1fr;
  }

  .filter-panel,
  .accounts-panel,
  .detail-panel {
    min-height: unset;
  }
}

/* Create account alignment polish */
.create-panel .create-form-top,
.create-panel .exchange-input-row,
.create-panel .exchange-row,
.create-panel .create-exchange-row {
  display: block;
  width: 100%;
}

.create-panel .exchange-select-wrap,
.create-panel .exchange-field,
.create-panel select,
.create-panel input {
  width: 100%;
  box-sizing: border-box;
}

.create-panel .exchange-summary-card,
.create-panel .selected-exchange-card,
.create-panel .exchange-info-card {
  width: 100%;
  margin-top: 14px;
  box-sizing: border-box;
}

.create-panel .create-form-grid,
.create-panel .create-fields-grid,
.create-panel .credential-grid {
  grid-template-columns: 1fr;
}



/* Create account correct centered block patch */
.create-panel {
  box-sizing: border-box;
}

.create-panel .create-form-card,
.create-panel .create-card,
.create-panel .form-card,
.create-panel .credential-card,
.create-panel .account-create-card {
  width: 100%;
  max-width: none;
  box-sizing: border-box;
}

.create-panel .create-form,
.create-panel form {
  width: 100%;
  max-width: none;
  box-sizing: border-box;
}

.create-panel .create-form-top,
.create-panel .exchange-input-row,
.create-panel .exchange-row,
.create-panel .create-exchange-row {
  display: block;
  width: 100%;
  max-width: none;
  box-sizing: border-box;
}

.create-panel .form-group,
.create-panel .exchange-select-wrap,
.create-panel .exchange-field,
.create-panel .exchange-summary-card,
.create-panel .selected-exchange-card,
.create-panel .exchange-info-card {
  width: 100%;
  max-width: none;
  box-sizing: border-box;
}

.create-panel input:not(.boolean-native-input),
.create-panel select {
  width: 100%;
  max-width: none;
  box-sizing: border-box;
}

.create-panel .boolean-native-input {
  width: 1px;
}

.create-panel .create-form-grid,
.create-panel .create-fields-grid,
.create-panel .credential-grid {
  width: 100%;
  max-width: none;
  display: grid;
  grid-template-columns: 1fr;
  box-sizing: border-box;
}

.create-panel .form-actions,
.create-panel .create-actions,
.create-panel .button-row {
  width: 100%;
  max-width: none;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 14px;
  box-sizing: border-box;
}

.create-panel .form-actions button,
.create-panel .create-actions button,
.create-panel .button-row button {
  width: 100%;
}

</style>

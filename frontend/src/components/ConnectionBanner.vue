<script setup lang="ts">
import { ref, watchEffect } from 'vue'
import { useConnectionStore } from '@/stores/connection'
import { usePreferencesStore } from '@/stores/preferences'

const conn  = useConnectionStore()
const prefs = usePreferencesStore()

const host = ref(prefs.host)
const port = ref(prefs.port)

function connect() {
  prefs.host = host.value.trim()
  prefs.port = Number(port.value)
  conn.connect(prefs.host, prefs.port)
}

function disconnect() {
  conn.disconnect()
}

const DOT_CLASS: Record<string, string> = {
  connected:    'dot dot--ok',
  connecting:   'dot dot--connecting',
  disconnected: 'dot dot--error',
}

const STATUS_LABEL: Record<string, string> = {
  connected:    'Connected',
  connecting:   'Connecting…',
  disconnected: 'Disconnected',
}
</script>

<template>
  <header class="banner surface">
    <div class="banner__status">
      <span :class="DOT_CLASS[conn.status]" />
      <span class="label">{{ STATUS_LABEL[conn.status] }}</span>
      <span v-if="conn.lastError" class="banner__error">{{ conn.lastError }}</span>
    </div>

    <form class="banner__form" @submit.prevent="connect">
      <label class="label" for="ws-host">Host</label>
      <input
        id="ws-host"
        v-model="host"
        class="input"
        style="width: 140px"
        placeholder="127.0.0.1"
        :disabled="conn.status !== 'disconnected'"
      />
      <label class="label" for="ws-port">Port</label>
      <input
        id="ws-port"
        v-model.number="port"
        class="input"
        style="width: 70px"
        type="number"
        min="1024"
        max="65535"
        :disabled="conn.status !== 'disconnected'"
      />
      <label class="banner__checkbox-label" for="auto-reconnect">
        <input
          id="auto-reconnect"
          v-model="prefs.autoReconnect"
          type="checkbox"
          class="banner__checkbox"
        />
        Auto Reconnect
      </label>
      <button
        v-if="conn.status === 'disconnected'"
        type="submit"
        class="btn btn--accent"
      >
        Connect
      </button>
      <button
        v-else
        type="button"
        class="btn btn--danger"
        @click="disconnect"
      >
        Disconnect
      </button>
    </form>
  </header>
</template>

<style scoped>
.banner {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  border-radius: 0;
  border-left: none;
  border-right: none;
  border-top: none;
  padding: 8px 16px;
}

.banner__status {
  display: flex;
  align-items: center;
  gap: 8px;
}

.banner__error {
  color: var(--c-danger);
  font-size: 12px;
}

.banner__form {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.banner__checkbox-label {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--c-text-muted);
  cursor: pointer;
  user-select: none;
}

.banner__checkbox {
  cursor: pointer;
  accent-color: var(--c-accent);
}
</style>

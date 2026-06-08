<script setup lang="ts">
import { onMounted, watch } from 'vue'
import { usePreferencesStore } from '@/stores/preferences'
import ConnectionBanner from '@/components/ConnectionBanner.vue'
import PlayerStatus    from '@/components/PlayerStatus.vue'
import InventoryList   from '@/components/InventoryList.vue'
import MapView         from '@/components/MapView.vue'

const prefs = usePreferencesStore()

function applyTheme(dark: boolean) {
  document.documentElement.setAttribute('data-theme', dark ? 'dark' : 'light')
}

onMounted(() => { applyTheme(prefs.darkMode) })
watch(() => prefs.darkMode, applyTheme)
</script>

<template>
  <div class="layout">
    <ConnectionBanner />

    <div class="toolbar">
      <span class="app-title">&#9878; Pip-Boy Remote</span>
      <label class="label toggle-label">
        <input type="checkbox" v-model="prefs.darkMode" />
        Dark mode
      </label>
    </div>

    <main class="content">
      <div class="col col--status">
        <PlayerStatus />
      </div>
      <div class="col col--inventory">
        <InventoryList />
      </div>
      <div class="col col--map">
        <MapView />
      </div>
    </main>
  </div>
</template>

<style scoped>
.layout {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 100vh;
}

.toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 16px;
  border-bottom: 1px solid var(--c-border);
  background: var(--c-surface);
}

.app-title {
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 0.03em;
  color: var(--c-accent);
  font-family: var(--font-mono);
}

.toggle-label {
  display: flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  user-select: none;
}

.content {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  padding: 12px;
  flex: 1;
  align-items: flex-start;
}

.col {
  flex: 1;
  min-width: 280px;
}

.col--status    { flex: 0 0 300px; }
.col--inventory { flex: 1 1 340px; }
.col--map       { flex: 0 0 544px; }   /* 512px canvas + 14px padding × 2 + 2px border */
</style>

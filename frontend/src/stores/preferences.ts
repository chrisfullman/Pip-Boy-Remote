import { defineStore } from 'pinia'
import { ref, watch } from 'vue'

const STORAGE_KEY = 'pip-boy-prefs'

interface StoredPrefs {
  host: string
  port: number
  darkMode: boolean
  autoReconnect: boolean
}

const DEFAULTS: StoredPrefs = {
  host: '127.0.0.1',
  port: 11104,
  darkMode: true,
  autoReconnect: false,
}

// Use the hostname the browser used to reach the frontend so that the WebSocket
// default points at the same machine — whether that is "localhost" on a desktop
// or a LAN IP when accessed from a tablet/phone.
function resolveDefaultHost(): string {
  const h = typeof window !== 'undefined' ? window.location.hostname : ''
  return h.length > 0 ? h : '127.0.0.1'
}

function loadPrefs(): StoredPrefs {
  const defaults: StoredPrefs = { ...DEFAULTS, host: resolveDefaultHost() }
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    if (raw) {
      return { ...defaults, ...(JSON.parse(raw) as Partial<StoredPrefs>) }
    }
  } catch {
    // Corrupt storage — fall through to defaults.
  }
  return { ...defaults }
}

export const usePreferencesStore = defineStore('preferences', () => {
  const saved = loadPrefs()

  const host          = ref(saved.host)
  const port          = ref(saved.port)
  const darkMode      = ref(saved.darkMode)
  const autoReconnect = ref(saved.autoReconnect)

  function persist() {
    try {
      localStorage.setItem(
        STORAGE_KEY,
        JSON.stringify({
          host:          host.value,
          port:          port.value,
          darkMode:      darkMode.value,
          autoReconnect: autoReconnect.value,
        }),
      )
    } catch {
      // localStorage may be unavailable (private browsing, test environments, SSR).
    }
  }

  watch([host, port, darkMode, autoReconnect], persist)

  return { host, port, darkMode, autoReconnect }
})

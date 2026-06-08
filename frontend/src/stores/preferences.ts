import { defineStore } from 'pinia'
import { ref, watch } from 'vue'

const STORAGE_KEY = 'pip-boy-prefs'

interface StoredPrefs {
  host: string
  port: number
  darkMode: boolean
}

const DEFAULTS: StoredPrefs = {
  host: '127.0.0.1',
  port: 11104,
  darkMode: true,
}

function loadPrefs(): StoredPrefs {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    if (raw) {
      return { ...DEFAULTS, ...(JSON.parse(raw) as Partial<StoredPrefs>) }
    }
  } catch {
    // Corrupt storage — fall through to defaults.
  }
  return { ...DEFAULTS }
}

export const usePreferencesStore = defineStore('preferences', () => {
  const saved = loadPrefs()

  const host     = ref(saved.host)
  const port     = ref(saved.port)
  const darkMode = ref(saved.darkMode)

  function persist() {
    try {
      localStorage.setItem(
        STORAGE_KEY,
        JSON.stringify({ host: host.value, port: port.value, darkMode: darkMode.value }),
      )
    } catch {
      // localStorage may be unavailable (private browsing, test environments, SSR).
    }
  }

  watch([host, port, darkMode], persist)

  return { host, port, darkMode }
})

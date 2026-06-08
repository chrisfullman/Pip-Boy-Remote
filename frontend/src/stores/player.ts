import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { PlayerState } from '@/types'

export const usePlayerStore = defineStore('player', () => {
  const state = ref<PlayerState | null>(null)

  function applyUpdate(update: PlayerState): void {
    state.value = update
  }

  return { state, applyUpdate }
})

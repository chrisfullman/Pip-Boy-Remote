import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { MapMarker } from '@/types'

export const useMapStore = defineStore('map', () => {
  const markers          = ref<MapMarker[]>([])
  const activeWaypointID = ref<number | null>(null)
  /** Worldspace editor ID sent by the backend, e.g. 'Commonwealth'. */
  const worldspace       = ref<string | null>(null)

  function applyMarkersUpdate(
    incoming:      MapMarker[],
    ws?:           string,
    waypointID?:   number,
  ): void {
    markers.value          = incoming
    worldspace.value       = ws ?? null
    activeWaypointID.value = waypointID ?? null
  }

  return { markers, activeWaypointID, worldspace, applyMarkersUpdate }
})

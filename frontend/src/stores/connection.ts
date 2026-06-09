import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { ConnectionStatus, IncomingMessage, OutgoingAction } from '@/types'
import {
  validateHeartbeat,
  validateStateUpdate,
  validateInventoryUpdate,
  validateActionResponse,
  validateMapMarkersUpdate,
} from '@/schemas'
import { usePlayerStore } from './player'
import { useInventoryStore } from './inventory'
import { useMapStore } from './map'
import { usePreferencesStore } from './preferences'

const RECONNECT_DELAY_MS = 3000

export const useConnectionStore = defineStore('connection', () => {
  const status        = ref<ConnectionStatus>('disconnected')
  const lastHeartbeat = ref<Date | null>(null)
  const lastError     = ref<string | null>(null)

  const isConnected = computed(() => status.value === 'connected')

  let ws:              WebSocket | null = null
  let reconnectTimer:  ReturnType<typeof setTimeout> | null = null
  let targetHost = ''
  let targetPort = 0
  // Set to true when the user explicitly clicks Disconnect, preventing the
  // onclose handler from scheduling a reconnect even if the pref is on.
  let manualDisconnect = false

  function connect(host: string, port: number): void {
    targetHost = host
    targetPort = port
    _openSocket(host, port)
  }

  function disconnect(): void {
    manualDisconnect = true
    _clearReconnect()
    ws?.close()
    ws = null
    status.value = 'disconnected'
  }

  function sendAction(action: OutgoingAction): void {
    if (ws?.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(action))
    }
  }

  function _openSocket(host: string, port: number): void {
    if (ws) { ws.onclose = null; ws.close(); ws = null }

    status.value    = 'connecting'
    lastError.value = null

    const socket = new WebSocket(`ws://${host}:${port}`)

    socket.onopen = () => {
      status.value = 'connected'
      lastError.value = null
    }

    socket.onmessage = (event: MessageEvent<string>) => {
      _handleMessage(event.data)
    }

    socket.onerror = () => {
      lastError.value = `Cannot reach ${host}:${port}`
    }

    socket.onclose = () => {
      status.value = 'disconnected'
      ws = null
      const wasManual = manualDisconnect
      manualDisconnect = false
      if (!wasManual && usePreferencesStore().autoReconnect) {
        _scheduleReconnect()
      }
    }

    ws = socket
  }

  function _scheduleReconnect(): void {
    _clearReconnect()
    reconnectTimer = setTimeout(() => {
      // Re-check the pref at fire time: the user may have unchecked it while
      // the timer was pending.
      if (usePreferencesStore().autoReconnect) { _openSocket(targetHost, targetPort) }
    }, RECONNECT_DELAY_MS)
  }

  function _clearReconnect(): void {
    if (reconnectTimer !== null) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
  }

  function _handleMessage(raw: string): void {
    let msg: unknown
    try {
      msg = JSON.parse(raw)
    } catch {
      console.warn('[WS] Malformed JSON:', raw)
      return
    }

    if (typeof msg !== 'object' || msg === null || !('type' in msg)) {
      console.warn('[WS] Missing "type" field:', msg)
      return
    }

    const incoming = msg as IncomingMessage

    switch (incoming.type) {
      case 'heartbeat':
        if (!validateHeartbeat(incoming)) {
          console.warn('[WS] Invalid heartbeat:', validateHeartbeat.errors)
          return
        }
        lastHeartbeat.value = new Date()
        break

      case 'state_update':
        if (!validateStateUpdate(incoming)) {
          console.warn('[WS] Invalid state_update:', validateStateUpdate.errors)
          return
        }
        usePlayerStore().applyUpdate(incoming.player)
        break

      case 'inventory_update':
        if (!validateInventoryUpdate(incoming)) {
          console.warn('[WS] Invalid inventory_update:', validateInventoryUpdate.errors)
          return
        }
        useInventoryStore().applyUpdate(incoming.items)
        break

      case 'action_response':
        if (!validateActionResponse(incoming)) {
          console.warn('[WS] Invalid action_response:', validateActionResponse.errors)
          return
        }
        if (!incoming.success) {
          console.warn(`[WS] Action "${incoming.action}" failed:`, incoming.error ?? '(no detail)')
        }
        break

      case 'map_markers_update':
        if (!validateMapMarkersUpdate(incoming)) {
          console.warn('[WS] Invalid map_markers_update:', validateMapMarkersUpdate.errors)
          return
        }
        useMapStore().applyMarkersUpdate(incoming.markers, incoming.worldspace, incoming.activeWaypointID)
        break

      default:
        console.warn('[WS] Unknown message type:', (incoming as Record<string, unknown>).type)
    }
  }

  return { status, lastHeartbeat, lastError, isConnected, connect, disconnect, sendAction }
})

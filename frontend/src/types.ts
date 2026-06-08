// TypeScript types mirroring the JSON Schema contracts in schema/*.schema.json.
// Keep in sync with the backend's GameState.h and JsonMessages.cpp.

export interface LimbConditions {
  head: number
  torso: number
  leftArm: number
  rightArm: number
  leftLeg: number
  rightLeg: number
  powerArmor: {
    head: number
    torso: number
    leftArm: number
    rightArm: number
    leftLeg: number
    rightLeg: number
  }
}

export interface PlayerState {
  health: number
  maxHealth: number
  actionPoints: number
  maxActionPoints: number
  currentWeight: number
  maxCarryWeight: number
  /** [x, y, z] world coordinates in Fallout 4 units. */
  coords: [number, number, number]
  /** Optional — backend emits 0 until GMST-based XP formula is verified. */
  level?: number
  experience?: number
  nextLevelXP?: number
  condition: LimbConditions
}

// 0=Weapon, 1=Armor, 2=Aid, 3=Ammo, 4=Misc, 255=Other
export const ItemCategory = {
  Weapon: 0,
  Armor: 1,
  Aid: 2,
  Ammo: 3,
  Misc: 4,
  Other: 255,
} as const
export type ItemCategoryValue = (typeof ItemCategory)[keyof typeof ItemCategory]

export const CATEGORY_LABEL: Record<ItemCategoryValue, string> = {
  0: 'Weapons',
  1: 'Armor',
  2: 'Aid',
  3: 'Ammo',
  4: 'Misc',
  255: 'Other',
}

export interface InventoryItem {
  formID: number
  name: string
  category: ItemCategoryValue
  count: number
  isEquipped: boolean
  weight: number
  value: number
}

export type ConnectionStatus = 'disconnected' | 'connecting' | 'connected'

// ── Incoming WebSocket messages ───────────────────────────────────────────────

export interface HeartbeatMessage {
  type: 'heartbeat'
  timestamp: string
  connected: boolean
}

export interface StateUpdateMessage {
  type: 'state_update'
  timestamp: string
  player: PlayerState
}

export interface InventoryUpdateMessage {
  type: 'inventory_update'
  timestamp: string
  items: InventoryItem[]
}

export interface ActionResponseMessage {
  type: 'action_response'
  timestamp: string
  action: string
  success: boolean
  error?: string
}

// ── Map marker types ──────────────────────────────────────────────────────────

export interface MapMarker {
  formID: number
  name: string
  /** [x, y] world-space coordinates. */
  coords: [number, number]
  isDiscovered: boolean
  isFastTravelable: boolean
  /** Icon type prefix, e.g. 'Vault' → maps/markers/VaultMarker.svg. */
  markerType?: string
}

export interface MapMarkersUpdateMessage {
  type: 'map_markers_update'
  timestamp: string
  /** Fallout 4 worldspace editor ID, e.g. 'Commonwealth'. */
  worldspace?: string
  markers: MapMarker[]
  activeWaypointID?: number
}

export type IncomingMessage =
  | HeartbeatMessage
  | StateUpdateMessage
  | InventoryUpdateMessage
  | ActionResponseMessage
  | MapMarkersUpdateMessage

// ── Outgoing action messages ──────────────────────────────────────────────────

export type OutgoingAction =
  | { action: 'set_waypoint'; markerID: number }
  | { action: 'fast_travel'; markerID: number }
  | { action: 'equip'; formID: number }
  | { action: 'unequip'; formID: number }
  | { action: 'consume'; formID: number }

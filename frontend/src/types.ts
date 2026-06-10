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
  /** Cumulative XP at the start of the current level; used for within-level progress bar. */
  levelStartXP?: number
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
  // Parsed from the game's raw name string; not shown in UI but preserved for future use.
  iconTag?: string       // Content of a leading [Tag] in the name, e.g. "ToolHammer"
  scrapMaterials?: string[] // Content of a trailing {{{Mat, Mat}}} in the name, e.g. ["Wood","Steel"]
}

// ── Quest types ───────────────────────────────────────────────────────────────

export interface QuestObjective {
  index: number
  text: string
  isCompleted: boolean
}

export interface QuestEntry {
  formID: number
  name: string
  currentStage: number
  /** True when the quest is shown in the HUD / tracked in the Pip-Boy. */
  isTracked: boolean
  /** Objectives currently displayed to the player. */
  objectives: QuestObjective[]
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

export interface QuestUpdateMessage {
  type: 'quest_update'
  timestamp: string
  quests: QuestEntry[]
  /** FormID of the currently tracked quest; absent when none is tracked. */
  activeQuestFormID?: number
}

export type IncomingMessage =
  | HeartbeatMessage
  | StateUpdateMessage
  | InventoryUpdateMessage
  | ActionResponseMessage
  | MapMarkersUpdateMessage
  | QuestUpdateMessage

// ── Outgoing action messages ──────────────────────────────────────────────────

export type OutgoingAction =
  | { action: 'set_waypoint'; markerID: number }
  | { action: 'fast_travel'; markerID: number }
  | { action: 'equip'; formID: number }
  | { action: 'unequip'; formID: number }
  | { action: 'consume'; formID: number }
  | { action: 'set_active_quest'; formID: number }

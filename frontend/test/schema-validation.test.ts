import { describe, it, expect } from 'vitest'
import {
  validateHeartbeat,
  validateStateUpdate,
  validateInventoryUpdate,
  validateActionResponse,
  validateMapMarkersUpdate,
} from '../src/schemas'

// ── Heartbeat ─────────────────────────────────────────────────────────────────

describe('heartbeat schema', () => {
  it('accepts a valid heartbeat', () => {
    const msg = { type: 'heartbeat', timestamp: '2026-06-07T12:00:00.000Z', connected: true }
    expect(validateHeartbeat(msg)).toBe(true)
  })

  it('rejects when "connected" is missing', () => {
    const msg = { type: 'heartbeat', timestamp: '2026-06-07T12:00:00.000Z' }
    expect(validateHeartbeat(msg)).toBe(false)
  })

  it('rejects when "timestamp" is not a date-time', () => {
    const msg = { type: 'heartbeat', timestamp: 'not-a-date', connected: false }
    expect(validateHeartbeat(msg)).toBe(false)
  })

  it('rejects the wrong type discriminant', () => {
    const msg = { type: 'state_update', timestamp: '2026-06-07T12:00:00.000Z', connected: true }
    expect(validateHeartbeat(msg)).toBe(false)
  })
})

// ── StateUpdate ───────────────────────────────────────────────────────────────

const VALID_STATE_UPDATE = {
  type: 'state_update',
  timestamp: '2026-06-07T12:00:00.000Z',
  player: {
    health: 95,
    maxHealth: 100,
    actionPoints: 60,
    maxActionPoints: 80,
    currentWeight: 150,
    maxCarryWeight: 210,
    coords: [1234.5, -678.9, 42.0],
    level: 15,
    experience: 3200,
    nextLevelXP: 0,
    condition: {
      head: 100, torso: 80, leftArm: 60, rightArm: 90, leftLeg: 100, rightLeg: 70,
      powerArmor: { head: 0, torso: 0, leftArm: 0, rightArm: 0, leftLeg: 0, rightLeg: 0 },
    },
  },
}

describe('state_update schema', () => {
  it('accepts a fully-populated state update', () => {
    expect(validateStateUpdate(VALID_STATE_UPDATE)).toBe(true)
  })

  it('accepts without optional level/experience/nextLevelXP', () => {
    const { player: { level, experience, nextLevelXP, ...playerRest }, ...rest } = VALID_STATE_UPDATE
    const msg = { ...rest, player: playerRest }
    expect(validateStateUpdate(msg)).toBe(true)
  })

  it('rejects coords with wrong length', () => {
    const msg = {
      ...VALID_STATE_UPDATE,
      player: { ...VALID_STATE_UPDATE.player, coords: [1, 2] },
    }
    expect(validateStateUpdate(msg)).toBe(false)
  })

  it('rejects when "player" is missing', () => {
    const { player: _p, ...msg } = VALID_STATE_UPDATE
    expect(validateStateUpdate(msg)).toBe(false)
  })
})

// ── InventoryUpdate ───────────────────────────────────────────────────────────

const VALID_INVENTORY_UPDATE = {
  type: 'inventory_update',
  timestamp: '2026-06-07T12:00:00.000Z',
  items: [
    { formID: 0x1A2B3C, name: 'Pipe Pistol', category: 0, count: 1, isEquipped: true,  weight: 2.3, value: 15  },
    { formID: 0xDEAD,   name: 'Stimpak',     category: 2, count: 5, isEquipped: false, weight: 0.1, value: 53  },
  ],
}

describe('inventory_update schema', () => {
  it('accepts a valid inventory snapshot', () => {
    expect(validateInventoryUpdate(VALID_INVENTORY_UPDATE)).toBe(true)
  })

  it('accepts an empty items array', () => {
    const msg = { ...VALID_INVENTORY_UPDATE, items: [] }
    expect(validateInventoryUpdate(msg)).toBe(true)
  })

  it('rejects an unknown category value', () => {
    const msg = {
      ...VALID_INVENTORY_UPDATE,
      items: [{ ...VALID_INVENTORY_UPDATE.items[0], category: 99 }],
    }
    expect(validateInventoryUpdate(msg)).toBe(false)
  })

  it('rejects items with count < 1', () => {
    const msg = {
      ...VALID_INVENTORY_UPDATE,
      items: [{ ...VALID_INVENTORY_UPDATE.items[0], count: 0 }],
    }
    expect(validateInventoryUpdate(msg)).toBe(false)
  })
})

// ── ActionResponse ────────────────────────────────────────────────────────────

describe('action_response schema', () => {
  it('accepts a successful action response', () => {
    const msg = {
      type: 'action_response',
      timestamp: '2026-06-07T12:00:00.000Z',
      action: 'equip',
      success: true,
    }
    expect(validateActionResponse(msg)).toBe(true)
  })

  it('accepts a failed response with an error string', () => {
    const msg = {
      type: 'action_response',
      timestamp: '2026-06-07T12:00:00.000Z',
      action: 'fast_travel',
      success: false,
      error: 'Fast travel is not allowed here',
    }
    expect(validateActionResponse(msg)).toBe(true)
  })

  it('rejects when "success" is missing', () => {
    const msg = {
      type: 'action_response',
      timestamp: '2026-06-07T12:00:00.000Z',
      action: 'equip',
    }
    expect(validateActionResponse(msg)).toBe(false)
  })

  it('rejects unknown extra properties', () => {
    const msg = {
      type: 'action_response',
      timestamp: '2026-06-07T12:00:00.000Z',
      action: 'equip',
      success: true,
      unknown: 'field',
    }
    expect(validateActionResponse(msg)).toBe(false)
  })
})

// ── MapMarkersUpdate ──────────────────────────────────────────────────────────

const VALID_MARKER = {
  formID: 0x1A26D,
  name: 'Diamond City',
  coords: [10000, 5000] as [number, number],
  isDiscovered: true,
  isFastTravelable: true,
}

const VALID_MAP_MARKERS_UPDATE = {
  type: 'map_markers_update',
  timestamp: '2026-06-07T12:00:00.000Z',
  worldspace: 'Commonwealth',
  markers: [
    VALID_MARKER,
    {
      formID: 0x23421,
      name: 'Vault 111',
      coords: [-24000, 118000] as [number, number],
      isDiscovered: false,
      isFastTravelable: false,
      markerType: 'Vault',
    },
  ],
}

describe('map_markers_update schema', () => {
  it('accepts a fully-populated update', () => {
    expect(validateMapMarkersUpdate(VALID_MAP_MARKERS_UPDATE)).toBe(true)
  })

  it('accepts an empty markers array', () => {
    const msg = { ...VALID_MAP_MARKERS_UPDATE, markers: [] }
    expect(validateMapMarkersUpdate(msg)).toBe(true)
  })

  it('accepts with activeWaypointID', () => {
    const msg = { ...VALID_MAP_MARKERS_UPDATE, activeWaypointID: 0x1A26D }
    expect(validateMapMarkersUpdate(msg)).toBe(true)
  })

  it('accepts without optional worldspace', () => {
    const { worldspace: _ws, ...msg } = VALID_MAP_MARKERS_UPDATE
    expect(validateMapMarkersUpdate(msg)).toBe(true)
  })

  it('rejects when "markers" is missing', () => {
    const { markers: _m, ...msg } = VALID_MAP_MARKERS_UPDATE
    expect(validateMapMarkersUpdate(msg)).toBe(false)
  })

  it('rejects a marker with only one coord', () => {
    const msg = {
      ...VALID_MAP_MARKERS_UPDATE,
      markers: [{ ...VALID_MARKER, coords: [10000] }],
    }
    expect(validateMapMarkersUpdate(msg)).toBe(false)
  })

  it('rejects a marker with three coords', () => {
    const msg = {
      ...VALID_MAP_MARKERS_UPDATE,
      markers: [{ ...VALID_MARKER, coords: [10000, 5000, 42] }],
    }
    expect(validateMapMarkersUpdate(msg)).toBe(false)
  })

  it('rejects extra properties on the top-level message', () => {
    const msg = { ...VALID_MAP_MARKERS_UPDATE, unknown: 'field' }
    expect(validateMapMarkersUpdate(msg)).toBe(false)
  })

  it('rejects extra properties on a marker object', () => {
    const msg = {
      ...VALID_MAP_MARKERS_UPDATE,
      markers: [{ ...VALID_MARKER, unknown: 'field' }],
    }
    expect(validateMapMarkersUpdate(msg)).toBe(false)
  })
})

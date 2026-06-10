<script setup lang="ts">
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from 'vue'
import { MglMap, MglMarker, useMap } from '@indoorequal/vue-maplibre-gl'
import { usePlayerStore } from '@/stores/player'
import { useMapStore }    from '@/stores/map'
import { useConnectionStore } from '@/stores/connection'
import type { MapMarker } from '@/types'

const player   = usePlayerStore()
const mapStore = useMapStore()
const conn     = useConnectionStore()

// ── Worldspace definitions ─────────────────────────────────────────────────────
// Bounds define the game-unit coordinate extents that each map JPEG covers.
// Derived from the Mappalachia rendering formula (see git history for derivation).
type WorldspaceKey = 'Commonwealth' | 'DLC03FarHarbor' | 'NukaWorld'

const WORLDSPACES: Record<WorldspaceKey, {
  label: string
  file:  string
  minX: number; maxX: number
  minY: number; maxY: number
}> = {
  Commonwealth: {
    label: 'Commonwealth',
    file:  '/maps/Commonwealth.jpg',
    minX: -144_000, maxX: 124_000,
    minY: -157_000, maxY: 111_000,
  },
  DLC03FarHarbor: {
    label: 'Far Harbor',
    file:  '/maps/DLC03FarHarbor.jpg',
    minX: -67_000, maxX:  83_000,
    minY: -75_000, maxY:  74_000,
  },
  NukaWorld: {
    label: 'Nuka-World',
    file:  '/maps/NukaWorld.jpg',
    minX: -68_000, maxX:  54_000,
    minY: -54_000, maxY:  68_000,
  },
}

// ── MapLibre initial-view constants ────────────────────────────────────────────
// Using module-level constants (stable object references) is critical here.
// vue-maplibre-gl watches :center and :zoom props by reference. If you pass an
// inline literal (e.g. :bounds="[[-1,-1],[1,1]]") Vue creates a new array on
// every render and the watcher fires on every WebSocket update, calling
// fitBounds() in a tight loop and making drag/zoom impossible.
const MAP_INITIAL_CENTER: [number, number] = [0, 0]
const MAP_INITIAL_ZOOM = 7

// ── Active worldspace ──────────────────────────────────────────────────────────
const activeWorldspace = ref<WorldspaceKey>('Commonwealth')

watch(() => mapStore.worldspace, (serverWs) => {
  if (serverWs && serverWs in WORLDSPACES) {
    activeWorldspace.value = serverWs as WorldspaceKey
  }
})

// ── Coordinate transform ────────────────────────────────────────────────────────
// Maps FO4 world-space (x, y) to pseudo-geographic [lng, lat] in [-1°, 1°].
// Using a ±1° equatorial band keeps Web Mercator distortion under 0.02%, which
// is imperceptible on a game map.  The game-map JPEG corners map to exactly
// [-1,-1] (SW) through [1,1] (NE) so all coordinate arithmetic stays consistent.
function fo4ToLngLat(x: number, y: number): [number, number] {
  const ws = WORLDSPACES[activeWorldspace.value]
  const lng = ((x - ws.minX) / (ws.maxX - ws.minX)) * 2 - 1
  const lat = ((y - ws.minY) / (ws.maxY - ws.minY)) * 2 - 1
  return [lng, lat]
}

// ── MapLibre GL style ───────────────────────────────────────────────────────────
// Minimal style: dark background + the active worldspace JPEG as a raster image
// source.  No remote tile servers or API keys required.
const mapStyle = computed(() => ({
  version: 8 as const,
  sources: {
    'game-map': {
      type: 'image' as const,
      url: WORLDSPACES[activeWorldspace.value].file,
      // Four corners ordered NW → NE → SE → SW, as required by MapLibre.
      coordinates: [
        [-1,  1] as [number, number],
        [ 1,  1] as [number, number],
        [ 1, -1] as [number, number],
        [-1, -1] as [number, number],
      ] as [[number, number], [number, number], [number, number], [number, number]],
    },
  },
  layers: [
    {
      id:   'bg',
      type: 'background' as const,
      paint: { 'background-color': '#161b22' },
    },
    {
      id:     'game-map-raster',
      type:   'raster' as const,
      source: 'game-map',
    },
  ],
}))

// ── Map access ─────────────────────────────────────────────────────────────────
// useMap() returns a reactive handle to the MapLibre instance.  map is null
// until after MglMap fires its load event.
const mapCtx = useMap()

// ── Follow mode ─────────────────────────────────────────────────────────────────
// When enabled the map continuously pans to keep the player centred.
// Only a user-initiated zoom disables follow mode.
const followPlayer = ref(true)

// Guards the one-time initial fly-in so it only fires once per session.
let initialZoomDone = false

// Fly to the player at zoom 11 on the first opportunity when both the map and
// the player state are available.
function tryInitialZoom(): void {
  if (initialZoomDone || !mapCtx.isLoaded || !player.state) return
  initialZoomDone = true
  const [lng, lat] = fo4ToLngLat(player.state.coords[0], player.state.coords[1])
  mapCtx.map?.flyTo({ center: [lng, lat], zoom: 11 })
}

// Re-enable follow at the current zoom level (used by the Follow Location button).
function enableFollow(): void {
  followPlayer.value = true
  if (!player.state || !mapCtx.map) return
  const [lng, lat] = fo4ToLngLat(player.state.coords[0], player.state.coords[1])
  mapCtx.map.easeTo({ center: [lng, lat], duration: 500 })
}

// Pan smoothly to the player whenever their coordinates change, but only while
// follow mode is active.  The initial position update is handled by tryInitialZoom.
watch(
  () => player.state?.coords,
  (coords) => {
    if (!coords || !followPlayer.value || !initialZoomDone || !mapCtx.isLoaded || !mapCtx.map) return
    const [lng, lat] = fo4ToLngLat(coords[0], coords[1])
    mapCtx.map.easeTo({ center: [lng, lat], duration: 300 })
  },
  { deep: true },
)

// ── Map event setup ─────────────────────────────────────────────────────────────
watch(() => mapCtx.isLoaded, (loaded) => {
  if (!loaded) return

  // Clicking the map canvas (not a marker overlay) clears the selection.
  mapCtx.map?.on('click', () => { selectedMarker.value = null })

  // A user-initiated zoom (originalEvent is defined) disables follow mode.
  // Programmatic zoom (flyTo / easeTo / fitBounds) has no originalEvent.
  mapCtx.map?.on('zoomstart', (e) => {
    if ('originalEvent' in e && e.originalEvent) {
      followPlayer.value = false
    }
  })

  tryInitialZoom()
})

// Also attempt the initial zoom when the first player-state payload arrives
// (handles the race where the map loads before the WebSocket data).
watch(() => player.state, (state) => {
  if (state) tryInitialZoom()
}, { once: true })

// ── Selected marker ─────────────────────────────────────────────────────────────
const selectedMarker = ref<MapMarker | null>(null)

// ── Control actions ─────────────────────────────────────────────────────────────
function fitToWorld(): void {
  followPlayer.value = false
  selectedMarker.value = null
  mapCtx.map?.fitBounds([[-1, -1], [1, 1]], { padding: 0, animate: false })
}

// Flies to the player at zoom 11 and re-enables follow mode.
function centerOnPlayer(): void {
  followPlayer.value = true
  if (!player.state || !mapCtx.map) return
  const [lng, lat] = fo4ToLngLat(player.state.coords[0], player.state.coords[1])
  mapCtx.map.flyTo({ center: [lng, lat], zoom: 11 })
}

function sendSetWaypoint(): void {
  if (selectedMarker.value) {
    conn.sendAction({ action: 'set_waypoint', markerID: selectedMarker.value.formID })
  }
}

function sendFastTravel(): void {
  if (selectedMarker.value?.isFastTravelable) {
    conn.sendAction({ action: 'fast_travel', markerID: selectedMarker.value.formID })
  }
}

// Re-fit the viewport whenever the active worldspace changes.
watch(activeWorldspace, () => {
  selectedMarker.value = null
  nextTick(() => {
    mapCtx.map?.once('styledata', () => {
      mapCtx.map?.fitBounds([[-1, -1], [1, 1]], { padding: 0, animate: false })
    })
  })
})

// ── Marker color helper ─────────────────────────────────────────────────────────
function markerDotColor(marker: MapMarker): string {
  if (!marker.isDiscovered) return 'var(--c-muted)'
  return marker.isFastTravelable ? 'var(--c-ok)' : 'var(--c-accent)'
}

// ── New marker detection ────────────────────────────────────────────────────────
// Tracks formIDs seen in the initial batch for each worldspace so that only
// genuinely new undiscovered markers get the appear animation.  Using plain
// (non-reactive) Sets because we only need them for comparison bookkeeping,
// not for driving template rendering.
const seenMarkerIDs          = new Set<number>()
const initializedWorldspaces = new Set<string>()

// Reactive array of formIDs currently playing the appear animation.
// A new array reference is written on each change so Vue detects the update.
const newMarkerIDs = ref<number[]>([])

const NEW_MARKER_ANIM_MS = 900

watch(() => mapStore.markers, (markers) => {
  if (!markers.length) return

  // First batch for this worldspace — seed all IDs as "seen" without animating,
  // so the map doesn't flash white when the initial data arrives.
  if (!initializedWorldspaces.has(activeWorldspace.value)) {
    initializedWorldspaces.add(activeWorldspace.value)
    for (const marker of markers) {
      seenMarkerIDs.add(marker.formID)
    }
    return
  }

  // Collect formIDs that are genuinely new and undiscovered.
  const additions: number[] = []
  for (const marker of markers) {
    if (!seenMarkerIDs.has(marker.formID) && !marker.isDiscovered) {
      seenMarkerIDs.add(marker.formID)
      additions.push(marker.formID)
    }
  }

  if (additions.length === 0) return

  newMarkerIDs.value = [...newMarkerIDs.value, ...additions]

  // Remove the highlight class once the CSS animation finishes.
  setTimeout(() => {
    newMarkerIDs.value = newMarkerIDs.value.filter(id => !additions.includes(id))
  }, NEW_MARKER_ANIM_MS)
})

// ── Fullscreen mode ─────────────────────────────────────────────────────────────
// Expands the map to fill the full browser viewport without using the
// native Fullscreen API (which replaces the entire display).
const fullscreen = ref(false)

function enterFullscreen(): void {
  fullscreen.value = true
}

function exitFullscreen(): void {
  fullscreen.value = false
}

function onKeyDown(e: KeyboardEvent): void {
  if (e.key === 'Escape' && fullscreen.value) {
    exitFullscreen()
  }
}

onMounted(() => { window.addEventListener('keydown', onKeyDown) })
onUnmounted(() => { window.removeEventListener('keydown', onKeyDown) })
</script>

<template>
  <section class="surface map-view" :class="{ 'map-view--fullscreen': fullscreen }">

    <!-- Header: label + worldspace selector + controls -->
    <div class="map-header">
      <span class="label">Map</span>
      <div class="map-controls">
        <select
          v-model="activeWorldspace"
          class="input map-select"
          title="Select worldspace"
        >
          <option v-for="(def, key) in WORLDSPACES" :key="key" :value="key">
            {{ def.label }}
          </option>
        </select>
        <button
          class="btn"
          :disabled="!player.state"
          title="Centre map on player"
          @click="centerOnPlayer"
        >
          ⊕ Player
        </button>
        <button
          class="btn"
          title="Fit entire worldspace"
          @click="fitToWorld"
        >
          ⊞ World
        </button>
        <button
          class="btn"
          title="Expand map to full viewport"
          @click="enterFullscreen"
        >
          ⛶ Fullscreen
        </button>
      </div>
    </div>

    <!-- Map container + cardinal direction overlays -->
    <div class="map-wrapper">
      <!-- map-inner gives MapLibre an explicit pixel height via absolute
           positioning (top: 0 / bottom: 0).  This is more reliable than
           height: 100% on a CSS aspect-ratio parent, which Safari does not
           always treat as a definite height. -->
      <div class="map-inner">
      <mgl-map
        :map-style="mapStyle"
        :center="MAP_INITIAL_CENTER"
        :zoom="MAP_INITIAL_ZOOM"
        :attribution-control="false"
        class="mgl-map"
      >
        <!-- Player position marker -->
        <mgl-marker
          v-if="player.state"
          :coordinates="fo4ToLngLat(player.state.coords[0], player.state.coords[1])"
        >
          <template #marker>
            <div class="player-marker">
              <div class="player-marker__pulse" />
              <div class="player-marker__dot" />
            </div>
          </template>
        </mgl-marker>

        <!-- Location markers -->
        <mgl-marker
          v-for="marker in mapStore.markers"
          :key="marker.formID"
          :coordinates="fo4ToLngLat(marker.coords[0], marker.coords[1])"
        >
          <template #marker>
            <div
              class="loc-marker"
              :class="{
                'loc-marker--undiscovered': !marker.isDiscovered,
                'loc-marker--selected':     selectedMarker?.formID === marker.formID,
                'loc-marker--waypoint':     marker.formID === mapStore.activeWaypointID,
                'loc-marker--new':          newMarkerIDs.includes(marker.formID),
              }"
              :style="{ '--dot': markerDotColor(marker) }"
              :title="marker.name"
              @click.stop="selectedMarker = marker"
            >
              <img
                v-if="marker.markerType"
                :src="`/maps/markers/${marker.markerType}Marker.svg`"
                class="loc-marker__icon"
                aria-hidden="true"
              />
            </div>
          </template>
        </mgl-marker>
      </mgl-map>

      <!-- Follow Location button — appears only when follow mode is disabled -->
      <button
        v-if="!followPlayer && player.state"
        class="btn follow-btn"
        @click="enableFollow"
      >
        ⊙ Follow Location
      </button>

      <!-- Cardinal direction labels positioned over the map -->
      <span class="cardinal cardinal--n">N</span>
      <span class="cardinal cardinal--s">S</span>
      <span class="cardinal cardinal--w">W</span>
      <span class="cardinal cardinal--e">E</span>

      <!-- Exit fullscreen button — top-right corner, fullscreen only -->
      <button
        v-if="fullscreen"
        class="btn exit-fullscreen-btn"
        title="Exit fullscreen (Esc)"
        @click="exitFullscreen"
      >
        ✕ Exit Fullscreen
      </button>

      <!-- Floating legend — overlays map in fullscreen mode -->
      <div v-if="fullscreen" class="map-legend map-legend--float">
        <div class="legend-item">
          <span class="legend-dot" style="background: var(--c-accent); box-shadow: 0 0 4px var(--c-accent)" />
          <span class="label">You</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot" style="background: var(--c-ok)" />
          <span class="label">Fast Travel</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot" style="background: var(--c-accent)" />
          <span class="label">Discovered</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot" style="background: var(--c-muted)" />
          <span class="label">Undiscovered</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot legend-dot--waypoint" />
          <span class="label">Waypoint</span>
        </div>
      </div>

      <!-- Floating marker panel — overlays map in fullscreen mode -->
      <div v-if="fullscreen && selectedMarker" class="marker-panel marker-panel--float">
        <div class="marker-panel__name">{{ selectedMarker.name }}</div>
        <div class="marker-panel__meta">
          <span class="label">Form ID</span>
          <span class="value">0x{{ selectedMarker.formID.toString(16).toUpperCase() }}</span>
          <span v-if="!selectedMarker.isDiscovered" class="badge badge--muted">Undiscovered</span>
          <span v-if="selectedMarker.isFastTravelable" class="badge badge--ok">Fast Travel</span>
        </div>
        <div class="marker-panel__actions">
          <button
            class="btn"
            :disabled="!conn.isConnected"
            @click="sendSetWaypoint"
          >
            Set Waypoint
          </button>
          <button
            class="btn btn--accent"
            :disabled="!conn.isConnected || !selectedMarker.isFastTravelable"
            @click="sendFastTravel"
          >
            Fast Travel
          </button>
          <button class="btn" title="Dismiss" @click="selectedMarker = null">✕</button>
        </div>
      </div>
      </div><!-- /.map-inner -->
    </div><!-- /.map-wrapper -->

    <!-- Selected marker info panel — below map in normal mode only -->
    <div v-if="!fullscreen && selectedMarker" class="marker-panel">
      <div class="marker-panel__name">{{ selectedMarker.name }}</div>
      <div class="marker-panel__meta">
        <span class="label">Form ID</span>
        <span class="value">0x{{ selectedMarker.formID.toString(16).toUpperCase() }}</span>
        <span v-if="!selectedMarker.isDiscovered" class="badge badge--muted">Undiscovered</span>
        <span v-if="selectedMarker.isFastTravelable" class="badge badge--ok">Fast Travel</span>
      </div>
      <div class="marker-panel__actions">
        <button
          class="btn"
          :disabled="!conn.isConnected"
          @click="sendSetWaypoint"
        >
          Set Waypoint
        </button>
        <button
          class="btn btn--accent"
          :disabled="!conn.isConnected || !selectedMarker.isFastTravelable"
          @click="sendFastTravel"
        >
          Fast Travel
        </button>
        <button class="btn" title="Dismiss" @click="selectedMarker = null">✕</button>
      </div>
    </div>

    <!-- Legend — below map in normal mode; floats inside map in fullscreen -->
    <div v-if="!fullscreen" class="map-legend">
      <div class="legend-item">
        <span class="legend-dot" style="background: var(--c-accent); box-shadow: 0 0 4px var(--c-accent)" />
        <span class="label">You</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot" style="background: var(--c-ok)" />
        <span class="label">Fast Travel</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot" style="background: var(--c-accent)" />
        <span class="label">Discovered</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot" style="background: var(--c-muted)" />
        <span class="label">Undiscovered</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot legend-dot--waypoint" />
        <span class="label">Waypoint</span>
      </div>
    </div>

  </section>
</template>

<style scoped>
.map-view {
  display: flex;
  flex-direction: column;
  gap: 10px;
  padding: 12px 14px;
}

/* ── Header ─────────────────────────────────────────────────────── */

.map-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 8px;
}

.map-controls {
  display: flex;
  align-items: center;
  gap: 6px;
  flex-wrap: wrap;
}

.map-select {
  font-family: var(--font-sans);
  padding: 4px 6px;
  cursor: pointer;
}

/* ── Map wrapper ────────────────────────────────────────────────── */

.map-wrapper {
  position: relative;
  width: 100%;
  /* padding-bottom: 100% creates a 1:1 square without relying on CSS
     aspect-ratio, which Safari does not always treat as a definite height for
     percentage-height children. */
  padding-bottom: 100%;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  overflow: hidden;
}

/* Absolutely positioned to fill the padding area created above.
   top: 0 + bottom: 0 gives a definite height so that height: 100% on
   children (including MapLibre's container) resolves correctly. */
.map-inner {
  position: absolute;
  inset: 0;
  /* Prevent the browser's scroll/pinch-zoom from swallowing touch events
     before MapLibre's pointer handlers can process them. */
  touch-action: none;
  user-select: none;
}

.mgl-map {
  width: 100%;
  height: 100%;
}

/* Hide the MapLibre attribution and logo badges; they reference external tile
   services we are not using and would confuse players. */
:deep(.maplibregl-ctrl-attrib),
:deep(.maplibregl-ctrl-logo) {
  display: none !important;
}

/* ── Follow Location button ─────────────────────────────────────── */

.follow-btn {
  position: absolute;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 3;
  white-space: nowrap;
  /* Slight backdrop so it's legible over any map content. */
  backdrop-filter: blur(4px);
}

/* ── Fullscreen mode ─────────────────────────────────────────────── */

/* Covers the entire viewport without triggering the native browser
   fullscreen API, so the rest of the browser chrome remains accessible. */
.map-view--fullscreen {
  position: fixed;
  inset: 0;
  z-index: 100;
  border-radius: 0;
  overflow: hidden;
}

/* In fullscreen the map grows to fill whatever space remains after
   the header, so the 1:1 aspect-ratio padding trick is not needed. */
.map-view--fullscreen .map-wrapper {
  flex: 1;
  padding-bottom: 0;
  border: none;
  border-radius: 0;
}

/* ── Exit fullscreen button ──────────────────────────────────────── */

.exit-fullscreen-btn {
  position: absolute;
  top: 8px;
  right: 8px;
  z-index: 10;
  white-space: nowrap;
  backdrop-filter: blur(4px);
}

/* ── Floating legend (fullscreen only) ───────────────────────────── */

/* Shares base layout with .map-legend but positioned absolutely over
   the map in the bottom-left corner. */
.map-legend--float {
  position: absolute;
  bottom: 8px;
  left: 8px;
  z-index: 3;
  background: var(--c-surface2);
  padding: 6px 10px;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border);
  backdrop-filter: blur(4px);
}

/* ── Floating marker panel (fullscreen only) ─────────────────────── */

/* Mirrors the normal .marker-panel but anchored to the bottom-right
   of the map so it does not obscure the Follow Location button. */
.marker-panel--float {
  position: absolute;
  bottom: 8px;
  right: 8px;
  z-index: 4;
  width: calc(100% - 16px);
  max-width: 320px;
  backdrop-filter: blur(4px);
}

/* ── Cardinal direction labels ──────────────────────────────────── */

.cardinal {
  position: absolute;
  color: var(--c-muted);
  font-size: 11px;
  font-family: var(--font-mono);
  opacity: 0.7;
  pointer-events: none;
  /* Sit above MapLibre's canvas and marker layers. */
  z-index: 2;
}

.cardinal--n { top: 4px;    left: 50%;  transform: translateX(-50%); }
.cardinal--s { bottom: 4px; left: 50%;  transform: translateX(-50%); }
.cardinal--w { left: 4px;   top: 50%;   transform: translateY(-50%); }
.cardinal--e { right: 4px;  top: 50%;   transform: translateY(-50%); }

/* ── Player marker ──────────────────────────────────────────────── */

.player-marker {
  position: relative;
  width: 10px;
  height: 10px;
}

.player-marker__dot {
  position: absolute;
  inset: 0;
  border-radius: 50%;
  background: var(--c-accent);
}

/* Outer pulse ring — matches the canvas version's 12 px arc. */
.player-marker__pulse {
  position: absolute;
  inset: -7px;
  border-radius: 50%;
  border: 1px solid var(--c-accent);
  opacity: 0.35;
}

/* ── Location markers ───────────────────────────────────────────── */

/* Vue scoped attributes are retained when MapLibre moves the element into its
   marker overlay container, so scoped selectors work correctly here. */

.loc-marker {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: var(--dot);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  box-sizing: border-box;
}

.loc-marker--undiscovered {
  opacity: 0.55;
}

.loc-marker--selected {
  outline: 2px solid #ffffff;
  outline-offset: 2px;
}

/* Waypoint is a solid warn-coloured 5-point star.
   clip-path replaces the circle shape; the inner icon is hidden to keep the
   star silhouette clean. */
.loc-marker--waypoint {
  width: 22px;
  height: 22px;
  background: var(--c-warn);
  border-radius: 0;
  clip-path: polygon(50% 0%, 61% 35%, 98% 35%, 68% 57%, 79% 91%, 50% 70%, 21% 91%, 32% 57%, 2% 35%, 39% 35%);
  outline: none;
}

.loc-marker--waypoint .loc-marker__icon {
  display: none;
}

.loc-marker__icon {
  width: 14px;
  height: 14px;
  pointer-events: none;
  user-select: none;
}

/* ── New marker appear animation ────────────────────────────────── */

/* brightness(10) saturate(0) washes the element out to white regardless of
   the underlying --dot color.  Animating back to brightness(1) saturate(1)
   lets the element's natural color emerge without trying to interpolate
   between white and an arbitrary CSS variable value.
   The cubic-bezier spring curve gives a slight elastic overshoot on the
   scale so the marker "pops" into place rather than linearly shrinking. */
@keyframes loc-marker-appear {
  from {
    transform: scale(1.8);
    filter: brightness(10) saturate(0);
    opacity: 1;
  }
  to {
    transform: scale(1);
    filter: brightness(1) saturate(1);
  }
}

.loc-marker--new {
  animation: loc-marker-appear 0.8s cubic-bezier(0.34, 1.56, 0.64, 1) both;
}

/* ── Selected marker panel ──────────────────────────────────────── */

.marker-panel {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 12px;
  background: var(--c-surface2);
  border: 1px solid var(--c-accent);
  border-radius: var(--radius-sm);
}

.marker-panel__name {
  font-size: 13px;
  font-weight: 600;
  color: var(--c-text);
}

.marker-panel__meta {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.marker-panel__actions {
  display: flex;
  gap: 6px;
  flex-wrap: wrap;
}

/* ── Badges ─────────────────────────────────────────────────────── */

.badge {
  font-size: 10px;
  font-weight: 600;
  border-radius: 3px;
  padding: 1px 5px;
  letter-spacing: 0.04em;
  text-transform: uppercase;
  border: 1px solid;
}

.badge--ok    { color: var(--c-ok);    border-color: var(--c-ok); }
.badge--muted { color: var(--c-muted); border-color: var(--c-muted); }

/* ── Legend ─────────────────────────────────────────────────────── */

.map-legend {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 5px;
}

.legend-dot {
  display: inline-block;
  width: 9px;
  height: 9px;
  border-radius: 50%;
  flex-shrink: 0;
}

.legend-dot--waypoint {
  background: var(--c-warn);
  border: none;
  border-radius: 0;
  clip-path: polygon(50% 0%, 61% 35%, 98% 35%, 68% 57%, 79% 91%, 50% 70%, 21% 91%, 32% 57%, 2% 35%, 39% 35%);
  width: 11px;
  height: 11px;
}
</style>

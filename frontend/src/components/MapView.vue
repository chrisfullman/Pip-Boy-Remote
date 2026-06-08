<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch } from 'vue'
import { usePlayerStore } from '@/stores/player'
import { useMapStore }    from '@/stores/map'
import { useConnectionStore } from '@/stores/connection'
import type { MapMarker } from '@/types'

const player  = usePlayerStore()
const mapStore = useMapStore()
const conn    = useConnectionStore()

// ── Canvas ─────────────────────────────────────────────────────────────────────
const CANVAS_W = 512
const CANVAS_H = 512

const canvas = ref<HTMLCanvasElement | null>(null)

// ── Worldspace definitions ─────────────────────────────────────────────────────
// Coordinate bounds are approximate.  They cover the full extent of each
// game worldspace in Fallout 4 units and are mapped 1:1 to the downloaded
// map images (which are square 4096×4096 exports from Mappalachia).
// Calibrate these constants once real in-game positions are verified.
type WorldspaceKey = 'Commonwealth' | 'DLC03FarHarbor' | 'NukaWorld'

const WORLDSPACES: Record<WorldspaceKey, {
  label: string
  file: string
  minX: number; maxX: number
  minY: number; maxY: number
}> = {
  Commonwealth: {
    label: 'Commonwealth',
    file:  '/maps/Commonwealth.jpg',
    minX: -130_000, maxX: 130_000,
    minY: -100_000, maxY: 130_000,
  },
  DLC03FarHarbor: {
    label: 'Far Harbor',
    file:  '/maps/DLC03FarHarbor.jpg',
    minX: -60_000, maxX: 60_000,
    minY: -60_000, maxY: 60_000,
  },
  NukaWorld: {
    label: 'Nuka-World',
    file:  '/maps/NukaWorld.jpg',
    minX: -60_000, maxX: 60_000,
    minY: -60_000, maxY: 60_000,
  },
}

// ── Active worldspace ──────────────────────────────────────────────────────────
// Defaults to Commonwealth; auto-switches when the backend sends a worldspace
// field in map_markers_update.  The user can also override via the UI dropdown.
const activeWorldspace = ref<WorldspaceKey>('Commonwealth')

function ws() { return WORLDSPACES[activeWorldspace.value] }

// Keep the dropdown in sync when the backend tells us the worldspace.
watch(() => mapStore.worldspace, (serverWs) => {
  if (serverWs && serverWs in WORLDSPACES) {
    activeWorldspace.value = serverWs as WorldspaceKey
  }
})

// ── Map background images ──────────────────────────────────────────────────────
// One HTMLImageElement per worldspace; loaded lazily on first access.
const mapImages: Partial<Record<WorldspaceKey, HTMLImageElement>> = {}

function getMapImage(key: WorldspaceKey): HTMLImageElement {
  if (!mapImages[key]) {
    const img = new Image()
    img.onload  = requestDraw
    img.onerror = () => { /* falls back to vector grid silently */ }
    img.src = WORLDSPACES[key].file
    mapImages[key] = img
  }
  return mapImages[key]!
}

// Eagerly load the default worldspace so it's ready when the component mounts.
getMapImage('Commonwealth')

// ── Marker icon cache ──────────────────────────────────────────────────────────
// Loaded lazily from /maps/markers/<type>Marker.svg; white SVG drawn over a
// coloured circle background (matching the in-game map marker style).
const iconCache = new Map<string, HTMLImageElement | null>()

function getIcon(markerType: string): HTMLImageElement | null {
  if (iconCache.has(markerType)) return iconCache.get(markerType) ?? null

  // Optimistically insert null so concurrent requests don't double-fetch.
  iconCache.set(markerType, null)

  const img = new Image()
  img.onload = () => { iconCache.set(markerType, img); requestDraw() }
  img.src = `/maps/markers/${markerType}Marker.svg`
  return null
}

// ── Viewport ───────────────────────────────────────────────────────────────────
// viewX / viewY: world-space point shown at the canvas centre.
// zoom: canvas pixels per world unit.
const viewX = ref(0)
const viewY = ref(0)
const zoom  = ref(1)

function minZoom(): number {
  const w = ws()
  return Math.min(
    CANVAS_W / (w.maxX - w.minX),
    CANVAS_H / (w.maxY - w.minY),
  )
}

const MAX_ZOOM_FACTOR = 64

function clampZoom(z: number): number {
  return Math.min(minZoom() * MAX_ZOOM_FACTOR, Math.max(minZoom(), z))
}

// Initialise or re-initialise the viewport to fit the full worldspace.
function fitToWorld(): void {
  const w = ws()
  viewX.value = (w.minX + w.maxX) / 2
  viewY.value = (w.minY + w.maxY) / 2
  zoom.value  = minZoom()
  selectedMarker.value = null
  requestDraw()
}

// Call fitToWorld whenever the active worldspace changes.
watch(activeWorldspace, () => {
  getMapImage(activeWorldspace.value)
  fitToWorld()
}, { immediate: true })

// ── Selected marker ────────────────────────────────────────────────────────────
const selectedMarker = ref<MapMarker | null>(null)

// ── Coordinate transforms ──────────────────────────────────────────────────────
// FO4 Y increases northward; canvas Y increases downward → invert.
function worldToCanvas(wx: number, wy: number): [number, number] {
  const cx = (wx - viewX.value) * zoom.value + CANVAS_W / 2
  const cy = -(wy - viewY.value) * zoom.value + CANVAS_H / 2
  return [cx, cy]
}

function canvasToWorld(cx: number, cy: number): [number, number] {
  const wx = (cx - CANVAS_W / 2) / zoom.value + viewX.value
  const wy = -((cy - CANVAS_H / 2) / zoom.value) + viewY.value
  return [wx, wy]
}

// Converts a pointer event's client position to canvas-space coordinates,
// accounting for any CSS scaling of the canvas element.
function clientToCanvas(e: { clientX: number; clientY: number }): [number, number] {
  const rect   = canvas.value!.getBoundingClientRect()
  const scaleX = CANVAS_W / rect.width
  const scaleY = CANVAS_H / rect.height
  return [(e.clientX - rect.left) * scaleX, (e.clientY - rect.top) * scaleY]
}

// ── Render ─────────────────────────────────────────────────────────────────────
let rafPending = false

function requestDraw(): void {
  if (rafPending) return
  rafPending = true
  requestAnimationFrame(() => { rafPending = false; draw() })
}

interface ThemeColors {
  surface: string; border: string; accent: string
  muted: string; ok: string; warn: string; text: string
}

function readTheme(): ThemeColors {
  const cs = getComputedStyle(document.documentElement)
  const get = (v: string, fb: string) => cs.getPropertyValue(v).trim() || fb
  return {
    surface: get('--c-surface', '#161b22'),
    border:  get('--c-border',  '#30363d'),
    accent:  get('--c-accent',  '#39ff14'),
    muted:   get('--c-muted',   '#768390'),
    ok:      get('--c-ok',      '#3fb950'),
    warn:    get('--c-warn',    '#d29922'),
    text:    get('--c-text',    '#cdd9e5'),
  }
}

function draw(): void {
  const el = canvas.value
  if (!el) return
  const ctx = el.getContext('2d')
  if (!ctx) return

  const c = readTheme()

  ctx.clearRect(0, 0, CANVAS_W, CANVAS_H)
  ctx.fillStyle = c.surface
  ctx.fillRect(0, 0, CANVAS_W, CANVAS_H)

  const img = getMapImage(activeWorldspace.value)
  if (img.complete && img.naturalWidth > 0) {
    drawMapImage(ctx, img)
  } else {
    drawGrid(ctx, c)
  }

  drawMarkers(ctx, c)
  drawPlayer(ctx, c)
  drawCardinals(ctx, c)
}

function drawMapImage(ctx: CanvasRenderingContext2D, img: HTMLImageElement): void {
  const w = ws()
  const [x0, y0] = worldToCanvas(w.minX, w.maxY)  // top-left  (west, north)
  const [x1, y1] = worldToCanvas(w.maxX, w.minY)  // btm-right (east, south)
  ctx.drawImage(img, x0, y0, x1 - x0, y1 - y0)
}

function drawGrid(ctx: CanvasRenderingContext2D, c: ThemeColors): void {
  const CELLS   = 20
  const w       = ws()
  const spacing = (w.maxX - w.minX) / CELLS

  ctx.save()
  ctx.strokeStyle = c.border
  ctx.lineWidth   = 0.5

  const [startWX] = canvasToWorld(0, 0)
  const [endWX]   = canvasToWorld(CANVAS_W, 0)
  const [, endWY] = canvasToWorld(0, 0)
  const [, startWY] = canvasToWorld(0, CANVAS_H)

  for (
    let wx = Math.floor(startWX / spacing) * spacing;
    wx <= endWX + spacing;
    wx += spacing
  ) {
    const [cx] = worldToCanvas(wx, 0)
    ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, CANVAS_H); ctx.stroke()
  }
  for (
    let wy = Math.floor(startWY / spacing) * spacing;
    wy <= endWY + spacing;
    wy += spacing
  ) {
    const [, cy] = worldToCanvas(0, wy)
    ctx.beginPath(); ctx.moveTo(0, cy); ctx.lineTo(CANVAS_W, cy); ctx.stroke()
  }

  ctx.restore()

  ctx.save()
  ctx.fillStyle  = c.muted
  ctx.font       = '11px ui-monospace, monospace'
  ctx.textAlign  = 'center'
  ctx.fillText('Run scripts/download-assets.sh to load the map image', CANVAS_W / 2, CANVAS_H - 14)
  ctx.restore()
}

// Marker visual constants (canvas pixels).
const MARKER_CIRCLE_R  = 10   // radius of the coloured background disc
const MARKER_ICON_SIZE = 14   // SVG drawn within the disc
const MARKER_DOT_R     =  3   // fallback dot when zoomed out
const MARKER_HIT_R     = 14   // hit-test radius for clicks/taps

// Zoom thresholds (multiples of minZoom) below which markers simplify.
const ZOOM_ICONS   = 6   // show full circle + icon above this factor
const ZOOM_LABELS  = 10  // add location labels above this factor

function drawMarkers(ctx: CanvasRenderingContext2D, c: ThemeColors): void {
  const zFactor = zoom.value / minZoom()

  for (const marker of mapStore.markers) {
    const [cx, cy] = worldToCanvas(marker.coords[0], marker.coords[1])

    if (
      cx < -MARKER_HIT_R || cx > CANVAS_W + MARKER_HIT_R ||
      cy < -MARKER_HIT_R || cy > CANVAS_H + MARKER_HIT_R
    ) {
      continue
    }

    const isWaypoint = marker.formID === mapStore.activeWaypointID
    const isSelected = selectedMarker.value?.formID === marker.formID
    const dotColor   = marker.isDiscovered
      ? (marker.isFastTravelable ? c.ok : c.accent)
      : c.muted

    // ── Waypoint ring ──────────────────────────────────────────────────────
    if (isWaypoint) {
      ctx.save()
      ctx.strokeStyle = c.warn
      ctx.lineWidth   = 2
      ctx.setLineDash([4, 3])
      ctx.beginPath()
      ctx.arc(cx, cy, MARKER_CIRCLE_R + 5, 0, Math.PI * 2)
      ctx.stroke()
      ctx.restore()
    }

    // ── Selection ring ─────────────────────────────────────────────────────
    if (isSelected) {
      ctx.save()
      ctx.strokeStyle = '#ffffff'
      ctx.lineWidth   = 1.5
      ctx.beginPath()
      ctx.arc(cx, cy, MARKER_CIRCLE_R + 3, 0, Math.PI * 2)
      ctx.stroke()
      ctx.restore()
    }

    if (zFactor >= ZOOM_ICONS) {
      // Full marker: coloured disc + white SVG icon.
      ctx.save()
      ctx.globalAlpha = marker.isDiscovered ? 1 : 0.55

      // Coloured disc background.
      ctx.fillStyle = dotColor
      ctx.beginPath()
      ctx.arc(cx, cy, MARKER_CIRCLE_R, 0, Math.PI * 2)
      ctx.fill()

      // White SVG icon on top (matches in-game marker style).
      const icon = marker.markerType ? getIcon(marker.markerType) : null
      if (icon) {
        const half = MARKER_ICON_SIZE / 2
        ctx.drawImage(icon, cx - half, cy - half, MARKER_ICON_SIZE, MARKER_ICON_SIZE)
      }

      ctx.restore()
    } else {
      // Simplified dot for performance at low zoom.
      ctx.save()
      ctx.fillStyle   = dotColor
      ctx.globalAlpha = marker.isDiscovered ? 1 : 0.5
      ctx.beginPath()
      ctx.arc(cx, cy, MARKER_DOT_R, 0, Math.PI * 2)
      ctx.fill()
      ctx.restore()
    }

    // ── Label ──────────────────────────────────────────────────────────────
    if (isSelected || zFactor >= ZOOM_LABELS) {
      ctx.save()
      ctx.fillStyle  = isSelected ? '#ffffff' : dotColor
      ctx.font       = isSelected
        ? 'bold 10px ui-monospace, monospace'
        : '9px ui-monospace, monospace'
      ctx.textAlign  = 'left'
      const labelX   = Math.min(cx + MARKER_CIRCLE_R + 3, CANVAS_W - 90)
      ctx.fillText(marker.name, labelX, cy + 4)
      ctx.restore()
    }
  }
}

function drawPlayer(ctx: CanvasRenderingContext2D, c: ThemeColors): void {
  const state = player.state
  if (!state) return

  const [cx, cy] = worldToCanvas(state.coords[0], state.coords[1])

  if (cx < -12 || cx > CANVAS_W + 12 || cy < -12 || cy > CANVAS_H + 12) return

  ctx.save()

  // Outer pulse ring.
  ctx.strokeStyle  = c.accent
  ctx.lineWidth    = 1
  ctx.globalAlpha  = 0.35
  ctx.beginPath()
  ctx.arc(cx, cy, 12, 0, Math.PI * 2)
  ctx.stroke()

  // Solid dot.
  ctx.globalAlpha = 1
  ctx.fillStyle   = c.accent
  ctx.beginPath()
  ctx.arc(cx, cy, 5, 0, Math.PI * 2)
  ctx.fill()

  ctx.restore()
}

function drawCardinals(ctx: CanvasRenderingContext2D, c: ThemeColors): void {
  ctx.save()
  ctx.fillStyle  = c.muted
  ctx.font       = '11px ui-monospace, monospace'
  ctx.globalAlpha = 0.7
  ctx.textAlign  = 'center'
  ctx.fillText('N', CANVAS_W / 2, 14)
  ctx.fillText('S', CANVAS_W / 2, CANVAS_H - 4)
  ctx.textAlign = 'left'
  ctx.fillText('W', 4, CANVAS_H / 2 + 4)
  ctx.textAlign = 'right'
  ctx.fillText('E', CANVAS_W - 4, CANVAS_H / 2 + 4)
  ctx.restore()
}

// ── Pan / zoom gesture state ───────────────────────────────────────────────────
let panActive         = false
let panAnchorWorld:  [number, number] = [0, 0]
let mouseDownCanvas: [number, number] = [0, 0]

function beginPan(cx: number, cy: number): void {
  panActive      = true
  panAnchorWorld = canvasToWorld(cx, cy)
  mouseDownCanvas = [cx, cy]
}

// Keeps panAnchorWorld fixed under (cx, cy) as the pointer moves.
function updatePan(cx: number, cy: number): void {
  viewX.value = panAnchorWorld[0] - (cx - CANVAS_W / 2) / zoom.value
  viewY.value = panAnchorWorld[1] + (cy - CANVAS_H / 2) / zoom.value
  requestDraw()
}

// Adjusts zoom while keeping the world point at (cx, cy) stationary.
function applyZoom(factor: number, cx: number, cy: number): void {
  const [wx, wy] = canvasToWorld(cx, cy)
  const newZoom  = clampZoom(zoom.value * factor)
  zoom.value     = newZoom
  viewX.value    = wx - (cx - CANVAS_W / 2) / newZoom
  viewY.value    = wy + (cy - CANVAS_H / 2) / newZoom
  requestDraw()
}

// ── Mouse events ───────────────────────────────────────────────────────────────
function onMouseDown(e: MouseEvent): void {
  if (e.button !== 0) return
  beginPan(...clientToCanvas(e))
  window.addEventListener('mousemove', onWindowMouseMove)
  window.addEventListener('mouseup',   onWindowMouseUp)
}

function onWindowMouseMove(e: MouseEvent): void {
  if (!panActive) return
  updatePan(...clientToCanvas(e))
}

function onWindowMouseUp(e: MouseEvent): void {
  if (e.button !== 0) return
  window.removeEventListener('mousemove', onWindowMouseMove)
  window.removeEventListener('mouseup',   onWindowMouseUp)
  panActive = false

  const [cx, cy] = clientToCanvas(e)
  if (Math.hypot(cx - mouseDownCanvas[0], cy - mouseDownCanvas[1]) < 6) {
    handleTap(cx, cy)
  }
}

function onWheel(e: WheelEvent): void {
  e.preventDefault()
  const factor = e.deltaY < 0 ? 1.15 : 1 / 1.15
  applyZoom(factor, ...clientToCanvas(e))
}

// ── Touch events ───────────────────────────────────────────────────────────────
let activeTouches      = 0
let pinchStartDist     = 0
let pinchStartZoom     = 0
let pinchAnchorWorld: [number, number] = [0, 0]
let tapStartCanvas:  [number, number] = [0, 0]

function touchMidpoint(e: TouchEvent): [number, number] {
  const rect = canvas.value!.getBoundingClientRect()
  const scaleX = CANVAS_W / rect.width
  const scaleY = CANVAS_H / rect.height

  if (e.touches.length === 1) {
    return [
      (e.touches[0].clientX - rect.left) * scaleX,
      (e.touches[0].clientY - rect.top)  * scaleY,
    ]
  }
  return [
    ((e.touches[0].clientX + e.touches[1].clientX) / 2 - rect.left) * scaleX,
    ((e.touches[0].clientY + e.touches[1].clientY) / 2 - rect.top)  * scaleY,
  ]
}

function touchDistance(e: TouchEvent): number {
  return Math.hypot(
    e.touches[0].clientX - e.touches[1].clientX,
    e.touches[0].clientY - e.touches[1].clientY,
  )
}

function onTouchStart(e: TouchEvent): void {
  e.preventDefault()
  activeTouches = e.touches.length
  const [mx, my] = touchMidpoint(e)

  if (activeTouches === 1) {
    tapStartCanvas = [mx, my]
    beginPan(mx, my)
  } else if (activeTouches === 2) {
    panActive        = false
    pinchStartDist   = touchDistance(e)
    pinchStartZoom   = zoom.value
    pinchAnchorWorld = canvasToWorld(mx, my)
  }
}

function onTouchMove(e: TouchEvent): void {
  e.preventDefault()
  const [mx, my] = touchMidpoint(e)

  if (e.touches.length === 1 && panActive) {
    updatePan(mx, my)
  } else if (e.touches.length === 2) {
    const newZoom = clampZoom(pinchStartZoom * (touchDistance(e) / pinchStartDist))
    zoom.value    = newZoom
    viewX.value   = pinchAnchorWorld[0] - (mx - CANVAS_W / 2) / newZoom
    viewY.value   = pinchAnchorWorld[1] + (my - CANVAS_H / 2) / newZoom
    requestDraw()
  }
}

function onTouchEnd(e: TouchEvent): void {
  e.preventDefault()
  const wasSingle = activeTouches === 1
  activeTouches   = e.touches.length
  panActive       = activeTouches > 0

  if (wasSingle && e.changedTouches.length > 0) {
    const rect   = canvas.value!.getBoundingClientRect()
    const scaleX = CANVAS_W / rect.width
    const scaleY = CANVAS_H / rect.height
    const cx     = (e.changedTouches[0].clientX - rect.left) * scaleX
    const cy     = (e.changedTouches[0].clientY - rect.top)  * scaleY
    if (Math.hypot(cx - tapStartCanvas[0], cy - tapStartCanvas[1]) < 10) {
      handleTap(cx, cy)
    }
  }
}

// ── Tap / click ────────────────────────────────────────────────────────────────
function handleTap(cx: number, cy: number): void {
  let nearest: MapMarker | null = null
  let nearestDist = MARKER_HIT_R

  for (const marker of mapStore.markers) {
    const [mcx, mcy] = worldToCanvas(marker.coords[0], marker.coords[1])
    const d = Math.hypot(cx - mcx, cy - mcy)
    if (d < nearestDist) { nearestDist = d; nearest = marker }
  }

  // Clicking away from any marker clears the selection.
  selectedMarker.value = nearest
  requestDraw()
}

// ── Control actions ────────────────────────────────────────────────────────────
function centerOnPlayer(): void {
  if (!player.state) return
  viewX.value = player.state.coords[0]
  viewY.value = player.state.coords[1]
  zoom.value  = minZoom() * 10
  selectedMarker.value = null
  requestDraw()
}

function clearSelection(): void {
  selectedMarker.value = null
  requestDraw()
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

// ── Lifecycle ──────────────────────────────────────────────────────────────────
onMounted(() => {
  const el = canvas.value!
  el.addEventListener('wheel',      onWheel,      { passive: false })
  el.addEventListener('touchstart', onTouchStart, { passive: false })
  el.addEventListener('touchmove',  onTouchMove,  { passive: false })
  el.addEventListener('touchend',   onTouchEnd,   { passive: false })
})

onUnmounted(() => {
  const el = canvas.value
  if (el) {
    el.removeEventListener('wheel',      onWheel)
    el.removeEventListener('touchstart', onTouchStart)
    el.removeEventListener('touchmove',  onTouchMove)
    el.removeEventListener('touchend',   onTouchEnd)
  }
  window.removeEventListener('mousemove', onWindowMouseMove)
  window.removeEventListener('mouseup',   onWindowMouseUp)
})

watch(
  () => [player.state?.coords, mapStore.markers, mapStore.activeWaypointID] as const,
  requestDraw,
  { deep: true },
)
</script>

<template>
  <section class="surface map-view">

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
      </div>
    </div>

    <!-- Interactive canvas -->
    <canvas
      ref="canvas"
      :width="CANVAS_W"
      :height="CANVAS_H"
      class="map-canvas"
      @mousedown="onMouseDown"
    />

    <!-- Selected marker info panel -->
    <div v-if="selectedMarker" class="marker-panel">
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
        <button class="btn" title="Dismiss" @click="clearSelection">✕</button>
      </div>
    </div>

    <!-- Legend -->
    <div class="map-legend">
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

.map-canvas {
  display: block;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  /* Prevent browser scroll/pinch-zoom gestures on the canvas. */
  touch-action: none;
  cursor: crosshair;
  max-width: 100%;
}

.map-canvas:active {
  cursor: grabbing;
}

/* ── Selected marker panel ──────────────────────────────────────────────── */

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

/* ── Badges ─────────────────────────────────────────────────────────────── */

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

/* ── Legend ─────────────────────────────────────────────────────────────── */

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
  background: transparent;
  border: 2px dashed var(--c-warn);
  width: 11px;
  height: 11px;
}
</style>

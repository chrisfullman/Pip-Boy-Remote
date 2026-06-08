# Pip-Boy Remote — Development Progress

Last updated: 2026-06-07

This file is maintained by Claude between sessions as a quick-resume anchor.
It summarises what has been built, what is pending, and the key decisions made along the way.

---

## Overall status: ~70% complete

The core communication pipeline (backend WebSocket → JSON → frontend Vue) is implemented
end-to-end.  Player vitals, inventory, and the interactive map are all wired up.
What remains is map-marker broadcasting from the backend, packaging the frontend into
the mod ZIP, the contribution/style docs the spec requires, and general polish.

---

## Component status

### Backend (`backend/`)

| File | Status | Notes |
|------|--------|-------|
| `Plugin.cpp` | ✅ Complete | F4SE load hook, messaging handler, lifecycle |
| `WebSocketServer.cpp/.h` | ✅ Complete | uWS server, thread-safe broadcast, heartbeat timer, action dispatch |
| `GameStatePoller.cpp/.h` | ✅ Complete | Per-frame sampling; player vitals, inventory; throttled to ~10 Hz |
| `JsonMessages.cpp/.h` | ✅ Complete | Builds heartbeat, state_update, inventory_update, action_response |
| `Config.cpp/.h` | ✅ Complete | INI reading via GetPrivateProfileInt/String; test stub under PIPBOY_TESTING |
| `GameState.h` | ✅ Complete | POD structs: PlayerState, LimbConditions, InventoryItem, InventorySnapshot |
| `PCH.h` | ✅ Complete | Precompiled header wiring CommonLibF4 + RE headers |
| `xmake.lua` | ✅ Complete | nlohmann_json, uWebSockets, catch2; PipBoyRemote.dll + _Tests targets |

**Backend TODOs:**
- `nextLevelXP` — always emitted as `0.0f`.  Formula requires reading `iXPBase` and
  `fXPLevelUpMult` GMSTs from the game.  Frontend treats `0` as "unavailable" (hides XP bar).
- **Map markers** — `map_markers_update` message is not yet broadcast.  The frontend and
  schema are fully ready; the backend needs:
  1. `MapMarker` struct added to `GameState.h`
  2. `BuildMapMarkersUpdate()` added to `JsonMessages`
  3. `SampleMapMarkers()` added to `GameStatePoller` — scan the game's map marker list via
     `RE::TESDataHandler` / `RE::MapMarker` form type; emit on connect and when the
     player's discovered-location set changes

### Frontend (`frontend/`)

| File | Status | Notes |
|------|--------|-------|
| `src/main.ts` | ✅ Complete | Vue + Pinia mount |
| `src/App.vue` | ✅ Complete | Three-column layout, dark-mode toggle |
| `src/style.css` | ✅ Complete | Full design-token system, shared utility classes |
| `src/types.ts` | ✅ Complete | All TS interfaces mirroring schema contracts |
| `src/schemas.ts` | ✅ Complete | AJV 2020-12 validators for all 5 message types |
| `stores/connection.ts` | ✅ Complete | WS connect/disconnect, auto-reconnect, message routing |
| `stores/player.ts` | ✅ Complete | |
| `stores/inventory.ts` | ✅ Complete | |
| `stores/preferences.ts` | ✅ Complete | localStorage persistence |
| `stores/map.ts` | ✅ Complete | Markers, active waypoint, worldspace |
| `components/ConnectionBanner.vue` | ✅ Complete | Host/port form, status dot |
| `components/PlayerStatus.vue` | ✅ Complete | HP/AP/weight meters, limb grid, level/XP |
| `components/InventoryList.vue` | ✅ Complete | Category tabs, search, equip/consume actions |
| `components/MapView.vue` | ✅ Complete | Pan/zoom canvas, 3 worldspaces, map image + grid fallback, marker icons, selection panel |
| `test/schema-validation.test.ts` | ✅ Complete | 25 tests; all passing |
| `tsconfig.json` | ✅ Complete | Added `skipLibCheck: true` to suppress Vite/Vitest node type noise |

**Frontend TODOs:**
- **Vue component tests** — no component-level tests exist yet.  `@vue/test-utils` is
  already installed.  Priority areas: ConnectionBanner (connect/disconnect flow),
  InventoryList (filter/action buttons).
- **MapView coordinate calibration** — world bounds are approximate (±130 000 / ±60 000).
  Once real in-game positions are tested, update the `WORLDSPACES` constants in `MapView.vue`.

### Scripts / tooling (`scripts/`)

| File | Status | Notes |
|------|--------|-------|
| `replay.js` | ✅ Complete | Synthetic mode + file replay; 10 fps; loops; speed multiplier |
| `package-mod.sh` | ⚠️ Incomplete | Packages DLL + INI only. **Does not build or include the frontend.** Must add `npm ci && npm run build` and copy `frontend/dist/` into `Data/F4SE/Plugins/PipBoyRemote/`. |
| `download-assets.sh` | ✅ Complete | Downloads 3 map images + 73 SVG markers from Mappalachia |

**Scripts TODO:**
- Sample log file (`logs/sample.jsonl`) — replay.js has no recorded data to replay.
  Generate one with representative messages (heartbeat, state_update with movement,
  inventory_update with mixed categories, a couple of map markers).

### Schemas (`schema/`)

All 5 schemas complete: `heartbeat`, `state_update`, `inventory_update`,
`action_response`, `map_markers_update`.  All use JSON Schema draft 2020-12.

### CI (`.github/workflows/windows-package.yml`)

| Step | Status | Notes |
|------|--------|-------|
| Checkout + submodules | ✅ | |
| MSVC dev env | ✅ | |
| XMake install + backend build | ✅ | |
| Backend unit tests | ✅ | |
| Locate DLL + assemble ZIP | ✅ | |
| Upload artifacts + GitHub Release | ✅ | |
| **Frontend build** | ❌ Missing | No `npm ci && npm run build` step; frontend dist is not in the ZIP |

### Documentation (`docs/`, root)

| File | Status |
|------|--------|
| `README.md` | ✅ Complete |
| `docs/example.ini` | ✅ Complete |
| `docs/ASSETS.md` | ✅ Complete (Mappalachia GPL-3.0 attribution) |
| `docs/packaging.md` | ❌ Missing (referenced by README) |
| `CONTRIBUTING.md` | ❌ Missing (required by spec) |
| `CODE_STYLE.md` | ❌ Missing (required by spec) |
| `CHANGELOG.md` | ❌ Missing (required by spec) |
| Pre-commit hooks | ❌ Missing (required by spec) |

---

## Prioritised next steps

### P0 — Breaks the actual product
1. **`package-mod.sh` + CI: add frontend build.**
   - In `package-mod.sh`: run `npm ci && npm run build` in `frontend/`, then copy
     `frontend/dist/` to `Data/F4SE/Plugins/PipBoyRemote/` inside the ZIP.
   - In the CI workflow: add a Node.js setup step and a frontend build step before
     the "Assemble installable mod package" step.

### P1 — Core feature still missing
2. **Backend: map markers** — Implement `SampleMapMarkers()` in `GameStatePoller`.
   The frontend/schema side is fully ready; only the backend is missing.

### P2 — Required by project spec (CLAUDE.md)
3. `CONTRIBUTING.md`
4. `CODE_STYLE.md` + pre-commit hooks (`clang-format` for C++, `prettier` for frontend)
5. `CHANGELOG.md`
6. `docs/packaging.md`

### P3 — Quality / developer experience
7. Vue component tests (`ConnectionBanner`, `InventoryList`)
8. Sample log file `logs/sample.jsonl` for offline replay development
9. `nextLevelXP` formula (research `iXPBase` + `fXPLevelUpMult` GMSTs)
10. MapView coordinate calibration (requires real game session)

---

## Key architectural decisions

- **CommonLibF4 as git submodule** at `backend/commonlibf4`.  XMake picks it up via
  `includes("commonlibf4")` and the `commonlibf4.plugin` rule generates the F4SEPlugin_Version export.
- **uWebSockets** for the WS server.  `Loop::defer()` is used for cross-thread broadcast
  so all uWS mutations stay on the uWS thread.
- **Heartbeat timer** uses a raw `us_timer_t` (libuSockets) wired into the uWS event loop,
  firing every 5 s.  This keeps the loop alive even when no state updates flow.
- **Inventory change detection** by comparing `cachedWeight` as a cheap proxy; full rescan
  only when weight changes.  This avoids per-frame inventory iteration at 60 fps.
- **AJV 2020-12** requires `import Ajv from 'ajv/dist/2020'` (not the default export)
  because the schemas declare `$schema: draft/2020-12` and the default AJV 8 export
  uses draft-07.
- **Map assets are not in git** — downloaded by `scripts/download-assets.sh` from
  Mappalachia/Commonwealth_Cartography (GPL-3.0) into `frontend/public/maps/` which is
  gitignored.  Falls back to a vector grid if absent.
- **MapView SVG icons** are white-on-transparent; drawn over a coloured circle to match
  the in-game marker style.  Loaded lazily and cached in `iconCache`.
- **`skipLibCheck: true`** added to `frontend/tsconfig.json` to suppress spurious errors
  from Vite/Vitest node_modules type definitions that expect `@types/node`.

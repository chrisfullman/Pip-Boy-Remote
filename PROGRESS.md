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
- **Map markers** — ✅ Complete (2026-06-08).  `MapMarker`/`MapMarkersSnapshot` in `GameState.h`,
  `BuildMapMarkersUpdate()` in `JsonMessages`, `SampleMapMarkers()` in `GameStatePoller`.
  Scans `RE::TESDataHandler::GetFormArray<RE::BGSLocation>()`, resolves `worldLocMarker`
  handles, reads `ExtraMapMarker::mapMarkerData`.  Rescans on worldspace change and every
  ~10 s (600 frames) for newly-discovered locations.  `activeWaypointID` deferred (P3).

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
- **Vue component tests** — ✅ Complete (2026-06-08).  `ConnectionBanner` (14 tests: status display,
  input state, button rendering, connect/disconnect flow) and `InventoryList` (20 tests: empty state,
  rendering, tab/search filtering, action buttons) added in `test/`.  Total: 59 tests, all passing.
- **MapView coordinate calibration** — world bounds are approximate (±130 000 / ±60 000).
  Once real in-game positions are tested, update the `WORLDSPACES` constants in `MapView.vue`.

### Scripts / tooling (`scripts/`)

| File | Status | Notes |
|------|--------|-------|
| `replay.js` | ✅ Complete | Synthetic mode + file replay; 10 fps; loops; speed multiplier |
| `package-mod.sh` | ✅ Complete | Builds frontend with `npm ci && npm run build` (skippable via `PIPBOY_SKIP_FRONTEND_BUILD=1`), copies `frontend/dist/` to `Data/F4SE/Plugins/PipBoyRemote/` inside the ZIP. |
| `download-assets.sh` | ✅ Complete | Downloads 3 map images + 73 SVG markers from Mappalachia |

**Scripts TODO:** ✅ Sample log file `logs/sample.jsonl` added (2026-06-08) — 8 messages covering
heartbeat, player movement, inventory with 6 item categories, map markers for Commonwealth,
and level-up (experience gained).  Use with `node scripts/replay.js --loop logs/sample.jsonl`.

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
| **Frontend build** | ✅ Complete | Node.js 20 setup → `npm ci` → `npm run build`; script uses `PIPBOY_SKIP_FRONTEND_BUILD=1` |

### Documentation (`docs/`, root)

| File | Status |
|------|--------|
| `README.md` | ✅ Complete |
| `docs/example.ini` | ✅ Complete |
| `docs/ASSETS.md` | ✅ Complete (Mappalachia GPL-3.0 attribution) |
| `docs/packaging.md` | ✅ Complete (2026-06-08) |
| `CONTRIBUTING.md` | ✅ Complete (2026-06-08) |
| `CODE_STYLE.md` | ✅ Complete (2026-06-08) |
| `CHANGELOG.md` | ✅ Complete (2026-06-08) |
| `.clang-format` | ✅ Complete (2026-06-08) |
| `frontend/.prettierrc` | ✅ Complete (2026-06-08) |
| `.pre-commit-config.yaml` | ✅ Complete (2026-06-08) |

---

## Prioritised next steps

### P0 — ✅ Done (2026-06-08)
- `package-mod.sh` + CI frontend build step: complete.

### P1 — ✅ Done (2026-06-08)
- Backend map markers: complete.

### P2 — ✅ Done (2026-06-08)
- `CONTRIBUTING.md`, `CODE_STYLE.md`, `CHANGELOG.md`, `docs/packaging.md`,
  `.clang-format`, `frontend/.prettierrc`, `.pre-commit-config.yaml`: all complete.

### P3 — Partially done
- ✅ Vue component tests (59 total, all passing) — 2026-06-08
- ✅ `logs/sample.jsonl` — 2026-06-08
- ⏳ `nextLevelXP` formula — requires confirming GMST names in a live game session
- ⏳ MapView coordinate calibration — requires real in-game position testing

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

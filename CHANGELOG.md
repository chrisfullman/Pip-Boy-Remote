# Changelog

All notable changes to Pip-Boy Remote are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Backend F4SE plugin** — CommonLibF4-based DLL that registers with F4SE and manages the WebSocket server lifecycle alongside the game session.
- **WebSocket server** — uWebSockets-based server on port 11104 (configurable via INI). Full-duplex; broadcasts to all connected clients. Thread-safe broadcast via `Loop::defer()`.
- **Game state polling** — `GameStatePoller` samples player vitals, inventory, and map markers at ~10 Hz via `AddTaskPermanent`. Throttled per-frame; idle when no clients are connected.
- **Map marker broadcasting** — Scans `BGSLocation` forms in the current worldspace via `TESDataHandler`; resolves `worldLocMarker` handles; reads `ExtraMapMarker` data. Rescans on worldspace change and every ~10 seconds.
- **INI configuration** — `PipBoyRemote.ini` with `Enable`, `WebSocketPort`, `BindIP`, and `LogLevel` keys. Documented in `docs/example.ini`.
- **JSON Schema definitions** — Five message schemas (draft 2020-12): `heartbeat`, `state_update`, `inventory_update`, `action_response`, `map_markers_update`.
- **Frontend Vue 3 app** — Three-column layout with dark/light mode toggle. Components: `ConnectionBanner`, `PlayerStatus`, `InventoryList`, `MapView`.
- **MapView** — HTML5 Canvas interactive map with pan, zoom (mouse wheel + pinch), and tap-to-select. Supports Commonwealth, Far Harbor, and Nuka-World worldspaces. Renders Mappalachia SVG marker icons over coloured discs; falls back to vector grid when map images are absent.
- **Pinia stores** — `connection`, `player`, `inventory`, `preferences` (localStorage-backed), `map`.
- **AJV validation** — All incoming WebSocket messages validated against JSON Schema 2020-12 schemas before being applied to stores.
- **Frontend actions** — `equip`, `unequip`, `consume`, `set_waypoint`, `fast_travel` sent as JSON over the WebSocket; responses validated by `action_response` schema.
- **Offline replay harness** — `scripts/replay.js` replays a JSONL log file (or generates synthetic messages) over a WebSocket so the frontend can be developed without a running game.
- **Packaging script** — `scripts/package-mod.sh` builds the frontend, assembles the Mod Organizer 2–compatible ZIP (`Data/F4SE/Plugins/PipBoyRemote.dll` + `.ini` + `PipBoyRemote/` web app).
- **GitHub Actions CI** — `.github/workflows/windows-package.yml` builds backend with MSVC, runs unit tests, builds frontend with Node.js 20, assembles and uploads the mod ZIP, and creates a GitHub Release on tag push.
- **Map asset pipeline** — `scripts/download-assets.sh` fetches 3 map images and 73 SVG location marker icons from Mappalachia/Commonwealth_Cartography (GPL-3.0). Assets are gitignored; attribution documented in `docs/ASSETS.md`.
- **Backend unit tests** — Catch2 tests for `Config` and `JsonMessages` under the `PipBoyRemote_Tests` xmake target. `PIPBOY_TESTING` preprocessor guard stubs Windows INI API calls for cross-platform test runs.
- **Frontend schema tests** — 25 Vitest tests covering all five message schemas (valid payloads, missing required fields, wrong types, additional properties).
- **Contributing and style guides** — `CONTRIBUTING.md`, `CODE_STYLE.md`, `.clang-format`, `frontend/.prettierrc`, `.pre-commit-config.yaml`.
- **Packaging documentation** — `docs/packaging.md` covering build prerequisites, local packaging, and CI artifact retrieval.

### Known limitations
- `nextLevelXP` is always emitted as `0` (frontend hides the XP bar when `0`). Requires reading `iXPBase` / `fXPLevelUpMult` GMSTs from the game.
- `activeWaypointID` in `map_markers_update` is always `0`. Reading the active compass waypoint from the quest journal is deferred.
- Map world-coordinate bounds are approximate (±130 000 for Commonwealth, ±60 000 for DLC worldspaces) and should be calibrated against real in-game positions.

[Unreleased]: https://github.com/chrisfullman/Pip-Boy-Remote/compare/HEAD

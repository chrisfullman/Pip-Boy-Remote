# Pip-Boy Remote
The Pip-Boy Remote is a mod-based second-screen companion to a Fallout 4 player's active gaming session, replicating much of the in-game Pip-Boy functionality in a modern browser interface that can be accessible on the same computer's secondary monitor or on another device connected to the same network.

This replaces the now-decomissioned mobile companion app that Bethesda Game Studios officially offered for Android, iOS/iPadOS, and Windows Phone devices, utilizing a proprietary closed-source web socket API.

### This Fallout 4 mod comprises of two main components:
1. An efficient backend [Fallout 4 Script Extender](https://f4se.silverlock.org) ("F4SE") client that creates a full-duplex WebSocket used by the frontend client to send and receive realtime gameplay information, affecting what the player sees on the Pip-Boy Remote interface as it pertains to their active gaming session.
2. A modern frontend Vue + Vite web application interface that connects to the backend WebSocket-based API, accessible either from the computer running the Fallout 4 game on a secondary display, or a separate device connected to the same network.

### Backend API Features
- The backend portion of this Fallout 4 mod will automatically spawn a server providing a WebSocket API when there is an active gaming session (e.g., a new game or a loaded save game has been selected).
  - The spawned WebSocket API will be accessible via port 11104 by default, but can be customized by the gamer via the mod's INI configuration file.
- The WebSocket API will utilize efficient, human-readable JSON data that can be easily inspected and tested in the web console or third-party REST-based development tools.
- Provide low-latency, realtime information via a heartbeat implementation, only providing updates when it detects an active connection, and idling when it does not detect an active connection.

### Frontend Client Features
- Replicates primary in-game Pip-Boy data, such as:
  - the player's condition status (armor, health, buffs/debuffs, and active effects)
  - the player's realtime location coordinates in the game that can be displayed on an interactive map component, including map markers from the game's stock content and any other mod-installed custom map locations and their active waypoint
  - the ability for the player to equip/unequip armor, clothing, and weapons, as well as consume beverages/food/medication
  - the ability for the player to set a waypoint in the map, affecting their in-game compass target waypoint
  - the ability to fast travel to any exterior map marker locations, when the game allows fast travel (which excludes most indoor locations)
- Stores user preferences such as light/dark mode, UI size preferences, and togglable/drag-and-drop reorderable widgets, as well as the web socket API's IP address and port it should connect to.
- Accepts cursor and touch input, accomodating for pinch and zoom on maps and swiping for lists and map panning.
- Usable on modern desktop browsers, tablets, and mobile device screens.
- Utilizes vector-based SVG and Canvas elements whenever possible, and raster-based images for the map background.

## Build Architecture & Workflow
- The backend API will only target x64 Windows 10+ host computers running the latest versions of the Next Generation ("NG") and Anniversary Edition ("AE") Fallout 4 game runtime executables provided by Steam.
  - Fallout 4 (NG) runtime version 1.10.984 uses F4SE build version 0.7.2
  - Fallout 4 (AE) runtime version 1.11.221 uses F4SE build version 0.7.8
- The frontend web application will be a static HTML, CSS, and JavaScript browser-based client running Vue and Vite.

Extensive documentation for F4SE and any other required Fallout 4 mod development-related frameworks and libraries are listed in a section below.

## Code Style & Quality
- Always prioritize readable code over brevity, commenting classes, functions, and variables when possible.
- Always use early returns to reduce nesting.
- Find and incorporate Git submodules when possible, instead of manually downloading and extracting compressed archives.
- For command prompt instructions and code, ensure that any paths are enclosed in quotes or appropriately escaped when there are spaces or other characters that may cause a script or command to fail.
- Never use magic numbers; extract to named constants.
- Never delete code unless explicitly instructed.
- Always prefer spaces for indentation.
- Maintain the `.gitignore` file at all times, excluding any NPM, Node, and other package manager module folders that would be re-created by the CI build system; only store relevant project files in this project folder
- IMPORTANT: Always prefer explicit code over implicit magic.
- IMPORTANT: Always keep functions small and focused on one task, but allow them to be further customized by additional external functions, similar to WordPress' actions/filters functionality, instead of building complex switch/conditional statements.

## Testing & Debugging/Problem Solving
- Always prefer test-driven development when planning new features.
- IMPORTANT: Always treat compiler warnings like errors and fix immediately.
  - I will provide you any F4SE error logs or error information relayed by the Fallout 4 runtime executable.
- **Backend (C++):** Use Catch2 or GoogleTest for unit tests where logic is separable from game hooks. Provide a harness to run logic tests locally.
- **Frontend (Vue):** Use Vitest + Vue Testing Library for unit testing components and API contract conformance tests. Use AJV to validate messages against JSON Schema during tests.
- **Integration:** Provide a message-replay harness in `scripts/replay.js` that can replay saved JSON messages over a websocket so the frontend can be developed offline.
- Because you will be unable to play Fallout 4 with this mod installed, you will require me to handle the gameplay and rely on me to explain what I had done to result in specific API I/O between the backend and frontend.

## API Debugging / Replay Tool
- Implement `scripts/replay.js` (Node) that reads a JSON log and replays messages to the frontend websocket. This is useful for developing UI without a running game.

## Tech Stack
- **Frontend:** Vue 3 with Vite for dev server and build (modern, fast HMR). Use TypeScript where practical. UI components can use plain CSS, Tailwind, or a component library you prefer.
- **Backend:** F4SE Fallout 4 mod package, comprised of C++ (MSVC-compatible ABI)-compiled .DLL files, and other static supporting assets, targeting x64 Windows-based Fallout 4 (NG) and Fallout 4 (AE) game executables
- **API contract & validation:** JSON Schema for all websocket message types; validate messages in both frontend (AJV) and backend (if feasible).
- **Release Fallout 4 mod package:** A compressed .zip file including the mod's DLL(s), web application files, static assets, INI configuration file, in the standard Fallout 4 F4SE directory structure so it can be easily installable via Mod Organizer 2.

## INI & Runtime Config
- Document expected INI keys and defaults in an example file `docs/example.ini`. Suggested keys:
  - `PipBoyRemote.Enable = 1`
  - `PipBoyRemote.WebSocketPort = 12345`
  - `PipBoyRemote.BindIP = 0.0.0.0`
  - `PipBoyRemote.LogLevel = info`

## Cross-compile & CI recommendations
- Use GitHub Actions with a MSVC runner for official release builds.
  - REFERENCE: https://github.com/marketplace/actions/setup-msvc-developer-command-prompt
- Add a `ci/windows-build.yml` that:
  - Installs required SDKs and dependencies
  - Builds backend with MSVC
  - Builds frontend and archives static assets
  - Attaches compiled plugin `.dll` and frontend package as CI artifacts
  - Compiles the Fallout 4 mod's installable ZIP package in a release folder

## Contribution & Code Style
- Add `CONTRIBUTING.md` with branch, PR, commit message, and review guidance.
- Add `CODE_STYLE.md` with formatting and lint commands (e.g., `clang-format` for C++, `prettier` for frontend).
- Add pre-commit hooks to run linters.

## Assets & Licensing
- Use open-source assets where possible.
- Document sources in `docs/ASSETS.md`.
- I will also provide assets when you need them.

## Versioning & Changelog
- Adopt semantic versioning for releases and keep a `CHANGELOG.md` with release notes.

## Documentation for F4SE/Fallout 4 mod development frameworks/libraries
- F4SE (runtime source):
  - GitHub repository: https://github.com/ianpatt/f4se
  - The latest release in this GitHub repository is outdated; refer to the repo's tags for version-specific F4SE code
  - Assume the runtime library is already installed and activated on the host computer; no need to provide instructions on how to download and install it
- commonlibf4 (library):
  - CommonLibF4 is intended to replace F4SE as a static dependency. However, the runtime component of F4SE is still required.
  - Library Git repository: https://github.com/libxse/commonlibf4
  - Documentation wiki: https://deepwiki.com/libxse/commonlibf4/1-overview
    - Reverse Engineered (RE) Game Systems: https://deepwiki.com/libxse/commonlibf4/3.1-re-namespace:-reverse-engineered-game-systems
    - F4SE Integration Layer Overview: https://deepwiki.com/libxse/commonlibf4/3.2-f4se-integration-layer
    - F4SE API Reference: https://deepwiki.com/libxse/commonlibf4/4-api-reference
    - Reference the wiki's main navigation and subnavigation for additional knowledge sources
- F4SE Next-Gen Plugin Scaffold:
  - GitHub repository: https://github.com/Ez0n3/F4SE-Plugins/
  - Example plugin file/folder structure: https://github.com/Ez0n3/F4SE-Plugins/tree/main/f4se_plugins/ez_f4se_plugin_example
  - Common plugin definitions: https://github.com/Ez0n3/F4SE-Plugins/tree/main/f4se_plugins/plugins_common
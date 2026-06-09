# Pip-Boy Remote

Pip-Boy Remote is a Fallout 4 companion mod backend that publishes game state over a websocket.

## What this repo contains

- `backend/` — F4SE plugin scaffold and websocket server logic.
- `frontend/` — Vue 3 web client and UI.
- `schema/` — JSON schemas for websocket messages.
- `docs/` — documentation and example INI config.
- `scripts/` — helper scripts for replay and packaging.

## Frontend setup — map assets

The map images and SVG marker icons are not stored in this repository.
Download them once before running the dev server:

```bash
bash scripts/download-assets.sh
```

This fetches the Commonwealth, Far Harbor, and Nuka-World map images plus 73 SVG
location marker icons from the
[Commonwealth Cartography](https://github.com/Mappalachia/Commonwealth_Cartography)
project (GPL-3.0) into `frontend/public/maps/`.  The frontend falls back to a vector
grid if the images are absent.

See [`docs/ASSETS.md`](docs/ASSETS.md) for full attribution details and GPL-3.0
redistribution obligations.

## Offline development — replaying a game session

The replay harness lets you develop and test the frontend without a running game.
It starts a WebSocket server on the same port the game uses and feeds recorded
messages to the frontend exactly as the backend would.

```bash
cd scripts
npm install          # first time only
```

**Using a recorded session from Chrome DevTools:**

Open Chrome DevTools → Network → WS → click the `ws://` connection → Messages
tab → select all rows → copy and save as a `.txt` file.  Then:

```bash
# Play back at real speed (~11 min for a typical session)
node scripts/replay.js "logs/WebSocket Log.txt"

# Play back at 5× speed (~2 min)
node scripts/replay.js --speed 5 "logs/WebSocket Log.txt"

# Loop indefinitely at 3× speed
node scripts/replay.js --speed 3 --loop "logs/WebSocket Log.txt"
```

**Using the bundled sample log** (a short scripted sequence for quick UI checks):

```bash
node scripts/replay.js logs/sample.jsonl
```

**No recorded data?** Run with no arguments for a synthetic demo that slowly
drains health and moves a player dot across the map:

```bash
node scripts/replay.js
```

Once the harness is running, open the frontend (`npm run dev` in `frontend/`),
connect to `ws://127.0.0.1:11104`, and the UI will behave as if a live game
session is active.

> **Note:** The replay harness and the game backend both use port `11104` by
> default.  Do not run them at the same time.  Use `--port` to override if you
> need both running simultaneously (e.g. `--port 11105`).

## Dependency notice

This project includes a local copy of `commonlibf4` under `backend/commonlibf4`. You can build the backend using that copy directly, or point CMake to another `commonlibf4` or F4SE SDK installation using `F4SE_SDK_PATH` or `COMMONLIBF4_PATH`.

Example for a local SDK or commonlibf4 clone:

```bash
mkdir -p backend/build
cd backend/build
cmake -DCMAKE_BUILD_TYPE=Release \
  -DF4SE_SDK_PATH=/path/to/f4se/sdk \
  -DCOMMONLIBF4_PATH=/path/to/commonlibf4 ..
cmake --build . --config Release
```

If you are cross-compiling on macOS with Homebrew `mingw-w64`, `scripts/package-mod.sh` can also use the toolchain path to find MinGW runtime DLLs.

## Create an installable mod package

1. Build the backend plugin (Windows build recommended).
2. Run `scripts/package-mod.sh`.
3. Use the generated `build/PipBoyRemote-mod.zip` to install into Fallout 4.

If you are cross-compiling on macOS with Homebrew `mingw-w64`, you can pass the toolchain path as a third argument to `scripts/package-mod.sh` so the script includes MinGW runtime DLLs.

For details, see `docs/packaging.md`.

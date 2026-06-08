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

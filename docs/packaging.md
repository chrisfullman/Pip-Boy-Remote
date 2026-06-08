# Building and Packaging Pip-Boy Remote

This guide covers building the backend plugin, building the frontend web app, assembling the installable mod ZIP, and retrieving CI-built packages.

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| MSVC (Visual Studio 2022+) | v143 toolset | Compiling the C++ F4SE plugin |
| XMake | latest | Backend build system |
| Node.js | 20+ | Building the Vue frontend |
| npm | bundled with Node | Frontend dependency management |
| zip / unzip | any | Assembling the mod ZIP (macOS/Linux) or PowerShell `Compress-Archive` (Windows) |

> **Windows only for the DLL.** The backend `.dll` must be compiled with MSVC targeting x64 Windows. The frontend and packaging script can run on any platform with Node.js and `bash`.

## 1 — Build the backend

```bash
cd backend

# Configure for release (MSVC x64)
xmake config --plat=windows --arch=x64 --mode=release -y

# Build the plugin DLL
xmake build PipBoyRemote -y

# (Optional) Run unit tests
xmake build PipBoyRemote_Tests -y
xmake run  PipBoyRemote_Tests  -y
```

The compiled DLL is written to `backend/build/windows/x64/release/PipBoyRemote.dll`.

## 2 — Build the frontend

```bash
cd frontend

# Install dependencies (only needed once, or after package.json changes)
npm install

# Download map assets (only needed once per clone)
bash ../scripts/download-assets.sh

# Production build — output goes to frontend/dist/
npm run build
```

## 3 — Assemble the mod ZIP

`scripts/package-mod.sh` handles everything in one step.  Run it from the repository root:

```bash
bash scripts/package-mod.sh [dll_path] [output_dir]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `dll_path` | `backend/build/windows/x64/release/PipBoyRemote.dll` | Path to the compiled DLL |
| `output_dir` | `build/` | Directory that receives the ZIP |

Example (explicit paths):
```bash
bash scripts/package-mod.sh \
  "backend/build/windows/x64/release/PipBoyRemote.dll" \
  "build/"
```

The script:
1. Builds the frontend with `npm ci && npm run build` (skipped when `PIPBOY_SKIP_FRONTEND_BUILD=1`).
2. Creates the staging tree under `build/package/`.
3. Copies `PipBoyRemote.dll`, `PipBoyRemote.ini`, and `frontend/dist/` into the correct locations.
4. Zips the tree to `build/PipBoyRemote-mod.zip`.

### ZIP layout

```
PipBoyRemote-mod.zip
└── Data/
    └── F4SE/
        └── Plugins/
            ├── PipBoyRemote.dll
            ├── PipBoyRemote.ini
            └── PipBoyRemote/        ← frontend web app
                ├── index.html
                └── assets/
```

## 4 — Install into Fallout 4

### Mod Organizer 2

1. Open Mod Organizer 2.
2. Drag `PipBoyRemote-mod.zip` onto the MO2 window, or use **Install a mod from an archive** in the toolbar.
3. Enable the mod in the left pane.

### Vortex

1. Click **Install From File** and select `PipBoyRemote-mod.zip`.
2. Enable the mod after installation.

### Manual installation

Extract `PipBoyRemote-mod.zip` so the `Data/` folder merges with your Fallout 4 `Data/` directory (usually `C:\Program Files (x86)\Steam\steamapps\common\Fallout 4\Data\`).

## 5 — CI-built packages

Every push to `main` and every tag push triggers the GitHub Actions workflow.  Artifacts are available for 90 days on the **Actions** tab of the repository.

On tag pushes (e.g. `git tag v1.0.0 && git push --tags`), a GitHub Release is created automatically with `PipBoyRemote-mod.zip` attached.

## 6 — Configuration

After installation, edit `Data/F4SE/Plugins/PipBoyRemote.ini` to customise the server:

```ini
[PipBoyRemote]
Enable        = 1
WebSocketPort = 11104
BindIP        = 0.0.0.0
LogLevel      = info
```

See [`example.ini`](example.ini) for all available keys and their defaults.

The frontend connects to `ws://<host>:<port>` where `<host>` defaults to `localhost` and `<port>` matches `WebSocketPort`.  The connection settings can be changed in the browser UI.

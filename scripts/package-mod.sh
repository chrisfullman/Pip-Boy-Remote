#!/usr/bin/env bash
# package-mod.sh — Assembles the installable Fallout 4 mod ZIP package.
#
# Usage:
#   bash scripts/package-mod.sh [dll_path] [output_dir] [mingw_toolchain_path]
#
# Arguments (all optional):
#   dll_path          Path to the compiled PipBoyRemote.dll
#                     Default: backend/build/windows/x64/release/PipBoyRemote.dll
#   output_dir        Directory where the ZIP is written
#                     Default: build/
#   mingw_toolchain   Optional path to MinGW-w64 prefix (for experimental macOS
#                     cross-compile only); adds required MinGW runtime DLLs.
#
# The resulting ZIP follows the Mod Organizer 2 / Vortex directory layout:
#   Data/
#     F4SE/
#       Plugins/
#         PipBoyRemote.dll
#         PipBoyRemote.ini
#         PipBoyRemote/       <- frontend web app (index.html + assets/)

set -euo pipefail

DLL_PATH="${1:-backend/build/windows/x64/release/PipBoyRemote.dll}"
OUTPUT_DIR="${2:-build}"
MINGW_PREFIX="${3:-}"

PACKAGE_ROOT="${OUTPUT_DIR}/package"
PLUGINS_DIR="${PACKAGE_ROOT}/Data/F4SE/Plugins"
WEB_DIR="${PLUGINS_DIR}/PipBoyRemote"

# Set PIPBOY_SKIP_FRONTEND_BUILD=1 to skip this step when the frontend has
# already been built by a prior CI step or manual invocation.
if [[ "${PIPBOY_SKIP_FRONTEND_BUILD:-0}" == "1" ]]; then
    echo "==> Skipping frontend build (PIPBOY_SKIP_FRONTEND_BUILD=1)"
else
    echo "==> Building frontend"
    (
        cd frontend
        npm ci --prefer-offline
        npm run build
    )
    echo "    Frontend build complete -> frontend/dist/"
fi

echo "==> Assembling mod package"

rm -rf "${PACKAGE_ROOT}"
mkdir -p "${PLUGINS_DIR}"
mkdir -p "${WEB_DIR}"

# Copy DLL
if [[ ! -f "${DLL_PATH}" ]]; then
    echo "ERROR: DLL not found at ${DLL_PATH}" >&2
    exit 1
fi
cp "${DLL_PATH}" "${PLUGINS_DIR}/PipBoyRemote.dll"
echo "    Copied ${DLL_PATH} -> ${PLUGINS_DIR}/PipBoyRemote.dll"

# Copy example INI (renamed to the canonical plugin INI name)
if [[ -f "docs/example.ini" ]]; then
    cp "docs/example.ini" "${PLUGINS_DIR}/PipBoyRemote.ini"
    echo "    Copied docs/example.ini -> ${PLUGINS_DIR}/PipBoyRemote.ini"
fi

# Copy frontend web app (built by Vite into frontend/dist/)
FRONTEND_DIST="frontend/dist"
if [[ ! -d "${FRONTEND_DIST}" ]]; then
    echo "ERROR: frontend/dist/ not found — did the Vite build succeed?" >&2
    exit 1
fi
cp -r "${FRONTEND_DIST}/." "${WEB_DIR}/"
echo "    Copied ${FRONTEND_DIST}/ -> ${WEB_DIR}/"

# Optional: copy MinGW runtime DLLs for experimental cross-compiled builds
if [[ -n "${MINGW_PREFIX}" ]]; then
    for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
        runtime_dll="${MINGW_PREFIX}/lib/${dll}"
        if [[ -f "${runtime_dll}" ]]; then
            cp "${runtime_dll}" "${PLUGINS_DIR}/"
            echo "    Copied MinGW runtime: ${dll}"
        fi
    done
fi

# Create the ZIP.
# 'zip' is available on macOS / Linux; Windows runners ship 7-Zip instead.
mkdir -p "${OUTPUT_DIR}"
ZIP_PATH="${OUTPUT_DIR}/PipBoyRemote-mod.zip"
rm -f "${ZIP_PATH}"
if command -v zip &>/dev/null; then
    (cd "${PACKAGE_ROOT}" && zip -r "../PipBoyRemote-mod.zip" "Data/")
elif command -v 7z &>/dev/null; then
    (cd "${PACKAGE_ROOT}" && 7z a -tzip "../PipBoyRemote-mod.zip" "Data/")
else
    echo "ERROR: Neither 'zip' nor '7z' found. Cannot create ZIP." >&2
    exit 1
fi
echo "==> Package written to ${ZIP_PATH}"

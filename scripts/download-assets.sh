#!/usr/bin/env bash
# download-assets.sh
#
# Downloads map images and SVG marker icons from the Mappalachia /
# Commonwealth Cartography project into frontend/public/maps/.
#
# These files are excluded from the git repository (see .gitignore) due to
# their size and third-party provenance.  Run this script once before
# `npm run dev` or before building the frontend.
#
# Attribution:
#   Commonwealth Cartography by Mappalachia
#   https://github.com/Mappalachia/Commonwealth_Cartography
#   License: GNU General Public License v3.0
#
# Usage:
#   bash scripts/download-assets.sh

set -euo pipefail

RAW_BASE="https://raw.githubusercontent.com/Mappalachia/Commonwealth_Cartography/master/CommonwealthCartography/img"
DEST_DIR="$(dirname "$0")/../frontend/public/maps"

# ---------------------------------------------------------------------------
# Map images
# ---------------------------------------------------------------------------
MAP_FILES=(
    "Commonwealth.jpg"
    "DLC03FarHarbor.jpg"
    "NukaWorld.jpg"
)

# ---------------------------------------------------------------------------
# Marker SVGs
# ---------------------------------------------------------------------------
MARKER_FILES=(
    "AirfieldMarker.svg"     "BrownstoneMarker.svg"    "BunkerHillMarker.svg"
    "BunkerMarker.svg"       "CamperMarker.svg"         "CarMarker.svg"
    "CastleMarker.svg"       "CaveMarker.svg"           "ChurchMarker.svg"
    "CityMarker.svg"         "CustomHouseMarker.svg"    "DiamondCityMarker.svg"
    "DisciplesMarker.svg"    "DriveInMarker.svg"        "ElevatedHighwayMarker.svg"
    "EncampmentMarker.svg"   "FactoryMarker.svg"        "FaneuilHallMarker.svg"
    "FarmMarker.svg"         "FillingStationMarker.svg" "ForestedMarker.svg"
    "GalacticMarker.svg"     "GoodneighborMarker.svg"   "GraveyardMarker.svg"
    "HospitalMarker.svg"     "IndustrialDomeMarker.svg" "IndustrialStacksMarker.svg"
    "InstituteMarker.svg"    "IrishPrideMarker.svg"     "JunkyardMarker.svg"
    "LandmarkMarker.svg"     "LibertaliaMarker.svg"     "LowRiseMarker.svg"
    "MechanistMarker.svg"    "MetroMarker.svg"          "MilitaryBaseMarker.svg"
    "MonorailMarker.svg"     "MonumentMarker.svg"       "ObservatoryMarker.svg"
    "OfficeMarker.svg"       "OperatorsMarker.svg"      "PackMarker.svg"
    "PierMarker.svg"         "PoliceStationMarker.svg"  "PondLakeMarker.svg"
    "PrydwenMarker.svg"      "QuarryMarker.svg"         "RadioTowerMarker.svg"
    "RadioactiveAreaMarker.svg" "RaiderSettlementMarker.svg" "RailroadFactionMarker.svg"
    "RailroadMarker.svg"     "RidesMarker.svg"          "SafariMarker.svg"
    "SalemMarker.svg"        "SancHillsMarker.svg"      "SatelliteMarker.svg"
    "SchoolMarker.svg"       "SentinelMarker.svg"       "SettlementMarker.svg"
    "SewerMarker.svg"        "ShipwreckMarker.svg"      "SkyscraperMarker.svg"
    "SubmarineMarker.svg"    "SwanPondMarker.svg"       "TownMarker.svg"
    "TownRuinsMarker.svg"    "USSConstitutionMarker.svg" "UrbanRuinsMarker.svg"
    "VaultMarker.svg"
)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
download() {
    local url="$1"
    local dest="$2"
    if [ -f "$dest" ]; then
        echo "  [skip] $dest (already exists)"
        return
    fi
    echo "  [get]  $dest"
    curl -fsSL "$url" -o "$dest"
}

# ---------------------------------------------------------------------------
# Map images
# ---------------------------------------------------------------------------
mkdir -p "$DEST_DIR"
echo "Downloading map images -> $DEST_DIR"
for f in "${MAP_FILES[@]}"; do
    download "$RAW_BASE/$f" "$DEST_DIR/$f"
done

# ---------------------------------------------------------------------------
# Marker SVGs
# ---------------------------------------------------------------------------
mkdir -p "$DEST_DIR/markers"
echo "Downloading marker SVGs -> $DEST_DIR/markers"
for f in "${MARKER_FILES[@]}"; do
    download "$RAW_BASE/mapmarker/$f" "$DEST_DIR/markers/$f"
done

echo ""
echo "Done. Assets saved to frontend/public/maps/"
echo ""
echo "Attribution: Commonwealth Cartography by Mappalachia (GPL-3.0)"
echo "  https://github.com/Mappalachia/Commonwealth_Cartography"

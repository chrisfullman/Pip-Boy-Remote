# Asset Credits

This document lists all third-party assets used by Pip-Boy Remote and their respective
licenses and attribution requirements.

---

## Map Images and Marker Icons

**Source:** Commonwealth Cartography by Mappalachia
**Repository:** <https://github.com/Mappalachia/Commonwealth_Cartography>
**License:** GNU General Public License v3.0 (GPL-3.0)
**Contact:** mappalachia.feedback@gmail.com

### Files used

| File | Description |
|------|-------------|
| `Commonwealth.jpg` | Commonwealth worldspace map (4096×4096) |
| `DLC03FarHarbor.jpg` | Far Harbor DLC worldspace map (4096×4096) |
| `NukaWorld.jpg` | Nuka-World DLC worldspace map (4096×4096) |
| `mapmarker/*.svg` | 73 SVG map marker icons (white on transparent) |

### How assets are obtained

These files are **not stored in this repository** due to their size and third-party
provenance. Run the provided download script once before starting the dev server or
building the frontend:

```bash
bash scripts/download-assets.sh
```

This downloads the files from the Mappalachia GitHub repository into
`frontend/public/maps/`, which is excluded from version control via `.gitignore`.

### Attribution notice

Commonwealth Cartography is a non-commercial, community-developed tool for Fallout 4
players. The underlying game assets (worldspace map geometry, location data) originate
from Fallout 4 by Bethesda Game Studios / ZeniMax Media Inc. Commonwealth Cartography,
and by extension Pip-Boy Remote, uses these assets for non-commercial community benefit
under fair use. Neither project is affiliated with or endorsed by Bethesda Softworks LLC
or ZeniMax Media Inc.

Fallout 4 is a registered trademark of Bethesda Softworks LLC.

### GPL-3.0 obligations

Because these assets are distributed under GPL-3.0, any redistribution of the compiled
mod package that includes the downloaded map images or marker SVGs must:

1. Make the full GPL-3.0 license text available alongside the distribution.
2. Provide (or offer) access to the source of the Commonwealth Cartography project.
3. Preserve all copyright notices from the original project.

See <https://www.gnu.org/licenses/gpl-3.0.html> for the full license text.

---

*If you believe any asset has been included incorrectly or you have concerns about
attribution, please open an issue in this repository or contact the Mappalachia project
directly at mappalachia.feedback@gmail.com.*

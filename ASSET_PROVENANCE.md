<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# FreeWorks Asset Provenance Ledger

This ledger records the origin of every **binary image asset** shipped under the
FreeWorks fork's tracked image paths (at minimum `src/Gui/FreeWorks/Resources/`).
It is the single source of truth consulted by `tools/fw-provenance-guard.sh`,
which fails CI if any tracked binary image lacks a row here.

## Legal posture (D-03)

- **No verbatim SolidWorks proprietary assets.** Every icon/theme image is a
  *recreated original* — in-house authored look-alike art, never a copied,
  traced, or derived proprietary file.
- Each image below is marked **recreated-original** with its author and the date
  it entered the repository. A row is **required before an image lands**; the
  guard rejects any unlisted image.
- The asset type column distinguishes the image kind (e.g. `svg-icon`).

## How to add a new image

1. Add the image under a tracked image path (e.g.
   `src/Gui/FreeWorks/Resources/icons/`).
2. Add one row to the table below: the **repo-relative path**, asset type,
   `recreated-original`, source/author, license, and the date.
3. Run `bash tools/fw-provenance-guard.sh` locally — it must exit 0.

The path in column 1 must match the repo-relative path exactly (the guard does a
path-membership check against this column).

## Provenance table

One row per binary image asset.

| Path | Asset type | Provenance | Source / Author | License | Date |
|------|-----------|------------|-----------------|---------|------|
| `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg` | svg-icon | recreated-original | In-house authored FreeWorks workbench-selector icon (neutral "FW" mark; no proprietary third-party asset reproduced, traced, or derived) | LGPL-2.1-or-later | 2026-06-07 |

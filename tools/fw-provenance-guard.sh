#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# fw-provenance-guard.sh — FreeWorks asset-provenance CI guard (SHELL-02 / D-03,
# threat T-01-10).
#
# Enumerates every binary image file under the repo's tracked image path(s) and
# fails (exit non-zero) if any image is NOT recorded as a row in
# ASSET_PROVENANCE.md. This blocks an unlisted or proprietary-derived asset from
# silently entering the repo / a distribution.
#
# Usage:
#   tools/fw-provenance-guard.sh [SCAN_DIR ...]
#
#   With no argument, the guard scans the default tracked image path
#   (src/Gui/FreeWorks/Resources/) relative to the repo root.
#
#   With one or more SCAN_DIR arguments, it scans those directories instead.
#   This is the SELF-TEST seam: point it at a directory containing an unlisted
#   image and the guard must exit non-zero (used by CI to prove rejection
#   works).
#
# Determinism: file enumeration is sorted; the only inputs are the filesystem
# and ASSET_PROVENANCE.md. Runnable identically locally and in CI.
#
# An image is considered "listed" when its repo-relative path appears verbatim
# in a table row of ASSET_PROVENANCE.md (inside the first backtick-quoted cell).

set -euo pipefail

# --- locate repo root + the provenance ledger -------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
LEDGER="${REPO_ROOT}/ASSET_PROVENANCE.md"

if [[ ! -f "${LEDGER}" ]]; then
    echo "fw-provenance-guard: FATAL: ASSET_PROVENANCE.md not found at ${LEDGER}" >&2
    exit 2
fi

# --- binary-image extensions we track ---------------------------------------
# SVG is XML-based but is the workbench icon format and is treated as a tracked
# image asset for provenance purposes (D-03: recreated-original art).
IMAGE_EXTENSIONS=(png jpg jpeg gif bmp ico svg webp tif tiff)

# --- scan targets ------------------------------------------------------------
declare -a SCAN_DIRS
if [[ $# -gt 0 ]]; then
    SCAN_DIRS=("$@")
else
    SCAN_DIRS=("${REPO_ROOT}/src/Gui/FreeWorks/Resources")
fi

# Build the find expression for the image extensions.
declare -a FIND_EXPR=()
for ext in "${IMAGE_EXTENSIONS[@]}"; do
    FIND_EXPR+=(-iname "*.${ext}" -o)
done
# drop trailing -o
unset 'FIND_EXPR[${#FIND_EXPR[@]}-1]'

# --- helper: is a given path recorded in the ledger? -------------------------
# We require the back-ticked path to be the FIRST cell of a real Markdown table
# row: a leading "| " and a trailing " |" around the back-ticked path. This
# prevents (a) a substring of a longer back-ticked string and (b) a path that
# only appears in prose (e.g. the "How to add" example) from falsely satisfying
# the check. The relative path is regex-escaped so metacharacters (., /, etc.)
# match literally.
is_listed() {
    local rel="$1"
    # Escape regex-significant characters in the path for safe use in grep -E.
    local rel_esc
    rel_esc="$(printf '%s' "${rel}" | sed -e 's/[][\.*^$/+?(){}|]/\\&/g')"
    grep -qE "^\| \`${rel_esc}\` \|" "${LEDGER}"
}

# --- enumerate + check -------------------------------------------------------
missing=0
checked=0

for dir in "${SCAN_DIRS[@]}"; do
    if [[ ! -d "${dir}" ]]; then
        # A non-existent default dir means there is simply nothing to check.
        # A non-existent explicit arg is a usage error.
        if [[ $# -gt 0 ]]; then
            echo "fw-provenance-guard: FATAL: scan dir does not exist: ${dir}" >&2
            exit 2
        fi
        continue
    fi

    # Deterministic, NUL-safe enumeration.
    while IFS= read -r -d '' img; do
        checked=$((checked + 1))

        # Compute the path as recorded in the ledger:
        # - default scan: repo-relative (matches the ledger convention).
        # - explicit self-test dir: use the path as found (self-test images are
        #   intentionally NOT in the ledger, so they must be reported).
        if [[ "${img}" == "${REPO_ROOT}/"* ]]; then
            rel="${img#"${REPO_ROOT}/"}"
        else
            rel="${img}"
        fi

        if is_listed "${rel}"; then
            echo "fw-provenance-guard: OK   listed   ${rel}"
        else
            echo "fw-provenance-guard: FAIL unlisted ${rel}" >&2
            echo "  -> add a provenance row for '${rel}' to ASSET_PROVENANCE.md" >&2
            missing=$((missing + 1))
        fi
    done < <(find "${dir}" -type f \( "${FIND_EXPR[@]}" \) -print0 | sort -z)
done

echo "fw-provenance-guard: scanned ${checked} image(s); ${missing} unlisted."

if [[ ${missing} -gt 0 ]]; then
    echo "fw-provenance-guard: FAIL — ${missing} binary image(s) lack an ASSET_PROVENANCE.md row." >&2
    exit 1
fi

exit 0

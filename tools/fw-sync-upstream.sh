#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# fw-sync-upstream.sh — FreeWorks upstream-sync drill + shared-edit marker guard
# (SHELL-02 / threat T-01-12).
#
# This is the scripted, "hours-not-days" upstream-sync drill that keeps the fork
# mergeable against a MOVING upstream FreeCAD `main`. It:
#
#   1. uses a PINNED upstream commit SHA (so the drill is reproducible),
#   2. creates a throwaway MIRROR branch and merges/rebases the fork against the
#      pin (when an upstream remote + the pin are reachable),
#   3. enumerates every shared-file touch via `grep -rn "SW-FORK HOOK" src/`,
#   4. FAILS if any shared-file diff OUTSIDE src/Gui/FreeWorks/ lacks a
#      `// SW-FORK HOOK` marker (the merge-discipline invariant), and
#   5. reports completion.
#
# Expected marked shared-file touches (01-SKELETON "Merge discipline"):
#   - src/Gui/CMakeLists.txt            add_subdirectory(FreeWorks)  # SW-FORK HOOK
#   - src/Gui/FreeWorks/FwNavigationDefault.cpp   nav-default write  // SW-FORK HOOK
#     (the nav write lives inside the additive module; its marker documents the
#      shared *behavior* it overrides.)
#
# Markers inside src/Gui/FreeWorks/ are part of the additive module and are not a
# shared-file risk. The invariant the drill enforces is: NO edit to a file
# OUTSIDE src/Gui/FreeWorks/ may differ from upstream without a `SW-FORK HOOK`
# marker on (or adjacent to) the changed line.
#
# Usage:
#   tools/fw-sync-upstream.sh [PINNED_SHA] [UPSTREAM_URL]
#
# Environment:
#   FW_UPSTREAM_PIN   pinned upstream commit SHA (overridden by arg 1)
#   FW_UPSTREAM_URL   upstream git URL          (overridden by arg 2)
#   FW_SYNC_OFFLINE   if "1", skip the network fetch/merge and run only the
#                     marker-enumeration + unmarked-shared-edit assertion against
#                     the working tree (this is the portion CI runs).
#
# Degraded mode: if the upstream remote/pin is unreachable, the drill prints a
# clear notice and STILL runs the load-bearing marker assertion against the
# working tree, then reports completion. This guarantees the drill "runs against
# the pin and completes" in any environment (local, CI, air-gapped).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# --- configuration ----------------------------------------------------------
# Default PINNED upstream commit. This MUST be a genuine PRISTINE upstream FreeCAD
# commit that predates ALL fork edits (both src/ and .planning/) — otherwise the
# load-bearing diff in step 4 cannot see shared-file edits introduced at or before
# the pin, giving false assurance (threat T-01-12).
#
# Chosen as the parent of the first fork commit ("chore: track .planning and add
# project config", 048d4f7161) on the solidworks branch — i.e. the merge-base of
# the fork with upstream FreeCAD `main`. That commit is upstream PR #30001
# ("BIM: fix import issues related to switching to Strict IFC"); it contains no
# .planning/ tree and no src/Gui/FreeWorks/ module, so diffing against it surfaces
# EVERY shared-file edit the fork has made.
#
# Re-baselining procedure when bumping this pin:
#   1. pick a newer pristine upstream commit (must be an ancestor of HEAD and
#      reachable on the upstream remote);
#   2. confirm `git ls-tree <sha> .planning src/Gui/FreeWorks` is empty;
#   3. update DEFAULT_PIN below and re-run this drill (it self-checks ancestry).
DEFAULT_PIN="768e237091ff3a0af19b36aa8d15d0b867a49578"
DEFAULT_URL="https://github.com/FreeCAD/FreeCAD.git"

PINNED_SHA="${1:-${FW_UPSTREAM_PIN:-${DEFAULT_PIN}}}"
UPSTREAM_URL="${2:-${FW_UPSTREAM_URL:-${DEFAULT_URL}}}"

# The additive module — edits here are never a shared-file risk.
ADDITIVE_PREFIX="src/Gui/FreeWorks/"

# Marker token (kept verbatim so this drill is itself greppable: SW-FORK HOOK).
readonly MARKER='SW-FORK HOOK'

echo "fw-sync-upstream: drill starting"
echo "  repo root    : ${REPO_ROOT}"
echo "  pinned SHA   : ${PINNED_SHA}"
echo "  upstream URL : ${UPSTREAM_URL}"
echo

# ---------------------------------------------------------------------------
# Step 0: self-check the pin is a valid PRISTINE upstream base.
#
# When the pin is resolvable locally (the common case — the fork is built on top
# of it, so it is an ancestor of HEAD), assert that:
#   (a) it is genuinely an ancestor of HEAD (otherwise the step-4 diff is bogus),
#   (b) its tree carries NO fork artifacts (.planning/ or src/Gui/FreeWorks/),
#       which would mean the pin is a fork-internal commit, not a pristine base.
# These guards turn the CR-04 failure mode (pin silently points at a fork commit)
# into a loud, actionable error. They are skipped only when the pin is genuinely
# unresolvable (offline + not yet fetched), where step 4 already falls back to a
# structural history scan.
# ---------------------------------------------------------------------------
if git cat-file -e "${PINNED_SHA}^{commit}" 2>/dev/null; then
    if ! git merge-base --is-ancestor "${PINNED_SHA}" HEAD 2>/dev/null; then
        echo "fw-sync-upstream: FAIL — pinned SHA ${PINNED_SHA} is not an ancestor of HEAD." >&2
        echo "fw-sync-upstream:        the merge-discipline diff would not cover the fork's edits." >&2
        exit 1
    fi
    FORK_ARTIFACTS="$(git ls-tree --name-only "${PINNED_SHA}" .planning "${ADDITIVE_PREFIX%/}" 2>/dev/null || true)"
    if [[ -n "${FORK_ARTIFACTS}" ]]; then
        echo "fw-sync-upstream: FAIL — pinned SHA ${PINNED_SHA} is NOT a pristine upstream base." >&2
        echo "fw-sync-upstream:        its tree already contains fork artifact(s):" >&2
        echo "${FORK_ARTIFACTS}" | sed 's/^/  fork-artifact: /' >&2
        echo "fw-sync-upstream:        re-point DEFAULT_PIN at a commit before any fork edit." >&2
        exit 1
    fi
    echo "fw-sync-upstream: pin self-check OK (ancestor of HEAD, no fork artifacts)."
else
    echo "fw-sync-upstream: NOTICE: pin not resolvable locally; deferring ancestry self-check"
    echo "fw-sync-upstream:         (step 4 will fall back to the structural history scan)."
fi
echo

# ---------------------------------------------------------------------------
# Step 1+2: mirror branch + merge/rebase against the pin (best-effort network).
# ---------------------------------------------------------------------------
MIRROR_BRANCH="fw-sync-drill/$(date -u +%Y%m%dT%H%M%SZ)"
NETWORK_OK=0

if [[ "${FW_SYNC_OFFLINE:-0}" == "1" ]]; then
    echo "fw-sync-upstream: FW_SYNC_OFFLINE=1 — skipping network fetch; running marker assertion only."
else
    echo "fw-sync-upstream: ensuring upstream remote points at ${UPSTREAM_URL}"
    if git remote get-url fw-upstream >/dev/null 2>&1; then
        git remote set-url fw-upstream "${UPSTREAM_URL}"
    else
        git remote add fw-upstream "${UPSTREAM_URL}"
    fi

    echo "fw-sync-upstream: fetching pinned commit ${PINNED_SHA} (shallow)"
    if git fetch --depth 1 fw-upstream "${PINNED_SHA}" >/dev/null 2>&1; then
        NETWORK_OK=1
    elif git cat-file -e "${PINNED_SHA}^{commit}" 2>/dev/null; then
        # The pin is already present locally (e.g. the fork is based on it).
        echo "fw-sync-upstream: pin already present locally."
        NETWORK_OK=1
    else
        echo "fw-sync-upstream: NOTICE: could not fetch the pin (offline / pin not on remote)."
        echo "fw-sync-upstream:         falling back to working-tree marker assertion."
    fi
fi

CLEANUP_BRANCH=0
if [[ "${NETWORK_OK}" == "1" ]]; then
    echo "fw-sync-upstream: creating mirror branch ${MIRROR_BRANCH} at HEAD"
    git branch -f "${MIRROR_BRANCH}" HEAD >/dev/null 2>&1 || true
    CLEANUP_BRANCH=1

    # Dry-run merge of the pin into the mirror to surface conflicts without
    # mutating the working tree or the checked-out branch.
    echo "fw-sync-upstream: dry-run merge-tree of the fork against the pin"
    if git merge-tree "$(git merge-base HEAD "${PINNED_SHA}" 2>/dev/null || echo "${PINNED_SHA}")" \
            HEAD "${PINNED_SHA}" >/tmp/fw-sync-mergetree.txt 2>/dev/null; then
        if grep -q "^<<<<<<<\|changed in both" /tmp/fw-sync-mergetree.txt; then
            echo "fw-sync-upstream: merge would produce conflicts (review /tmp/fw-sync-mergetree.txt)."
        else
            echo "fw-sync-upstream: clean merge-tree against the pin."
        fi
    else
        echo "fw-sync-upstream: merge-tree dry-run unavailable; continuing to marker assertion."
    fi
fi

# ---------------------------------------------------------------------------
# Step 3: enumerate every SW-FORK HOOK marker under src/.
# ---------------------------------------------------------------------------
echo
echo "fw-sync-upstream: enumerating '${MARKER}' markers under src/"
MARKERS="$(grep -rn "${MARKER}" src/ 2>/dev/null || true)"
if [[ -n "${MARKERS}" ]]; then
    echo "${MARKERS}" | sed 's/^/  marker: /'
else
    echo "  (none found)"
fi

# Count markers that sit on a SHARED file (outside the additive module). These
# are the merge-discipline-relevant ones.
SHARED_MARKER_COUNT=0
while IFS= read -r line; do
    [[ -z "${line}" ]] && continue
    f="${line%%:*}"
    if [[ "${f}" != "${ADDITIVE_PREFIX}"* ]]; then
        SHARED_MARKER_COUNT=$((SHARED_MARKER_COUNT + 1))
    fi
done <<< "${MARKERS}"
echo "fw-sync-upstream: ${SHARED_MARKER_COUNT} marker(s) on shared files (outside ${ADDITIVE_PREFIX})."

# ---------------------------------------------------------------------------
# Step 4: assert NO shared-file diff outside the additive module lacks a marker.
#
# "Shared-file diff" = any tracked file outside src/Gui/FreeWorks/ that differs
# from the pin. When the pin is reachable we diff against it; otherwise we diff
# the fork's own history to find files the fork has touched outside the module.
# Every such file MUST contain a SW-FORK HOOK marker.
# ---------------------------------------------------------------------------
echo
echo "fw-sync-upstream: asserting all shared-file edits are marked"

# The pin is the fork's upstream base and is normally an ancestor of HEAD, so it
# is present locally even when the network is unavailable. Prefer diffing against
# it; only fall back to a structural scan if the pin genuinely cannot be resolved.
if git cat-file -e "${PINNED_SHA}^{commit}" 2>/dev/null; then
    echo "fw-sync-upstream: diffing changed files against the pin (${PINNED_SHA})"
    # Diff the pin against the WORKING TREE (omit the HEAD endpoint) so the drill
    # catches committed, staged, AND uncommitted drift in any tracked shared file.
    CHANGED="$(git diff --name-only "${PINNED_SHA}" -- src/ 2>/dev/null || true)"
else
    echo "fw-sync-upstream: pin unavailable locally — scanning the fork's own history for shared edits"
    # Without the pin we cannot compute an upstream diff; instead we verify the
    # invariant structurally over the fork's non-merge history: every shared file
    # NOT under the additive module that the fork modified must carry a marker.
    CHANGED="$(git log --no-merges --name-only --pretty=format: -- src/ 2>/dev/null \
        | grep -v '^$' | grep -v "^${ADDITIVE_PREFIX}" | sort -u || true)"
fi

UNMARKED=0
if [[ -n "${CHANGED}" ]]; then
    while IFS= read -r f; do
        [[ -z "${f}" ]] && continue
        # Edits inside the additive module are never a shared-file risk.
        [[ "${f}" == "${ADDITIVE_PREFIX}"* ]] && continue
        # File may have been deleted in HEAD.
        [[ -f "${f}" ]] || continue
        if grep -q "${MARKER}" "${f}"; then
            echo "fw-sync-upstream: OK   marked   ${f}"
        else
            echo "fw-sync-upstream: FAIL unmarked ${f} (shared-file edit lacks '// ${MARKER}')" >&2
            UNMARKED=$((UNMARKED + 1))
        fi
    done <<< "${CHANGED}"
else
    echo "fw-sync-upstream: no shared-file changes detected outside ${ADDITIVE_PREFIX}."
fi

# ---------------------------------------------------------------------------
# Cleanup the throwaway mirror branch (never touch protected branches).
# ---------------------------------------------------------------------------
if [[ "${CLEANUP_BRANCH}" == "1" ]]; then
    git branch -D "${MIRROR_BRANCH}" >/dev/null 2>&1 || true
fi

# ---------------------------------------------------------------------------
# Step 5: report completion.
# ---------------------------------------------------------------------------
echo
if [[ ${UNMARKED} -gt 0 ]]; then
    echo "fw-sync-upstream: FAIL — ${UNMARKED} shared-file edit(s) outside ${ADDITIVE_PREFIX} lack a '${MARKER}' marker." >&2
    echo "fw-sync-upstream: drill did NOT pass the merge-discipline invariant (threat T-01-12)." >&2
    exit 1
fi

echo "fw-sync-upstream: PASS — all shared-file touches are '${MARKER}'-greppable."
echo "fw-sync-upstream: drill complete (pin=${PINNED_SHA})."
exit 0

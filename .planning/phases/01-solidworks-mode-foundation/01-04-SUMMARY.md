---
phase: 01-solidworks-mode-foundation
plan: 04
subsystem: infra
tags: [ci, github-actions, asset-provenance, trademark, upstream-merge, fork-safety, freeworks]

# Dependency graph
requires:
  - phase: 01-solidworks-mode-foundation (01-01)
    provides: FreeWorks Gui submodule, the single marked add_subdirectory(FreeWorks) # SW-FORK HOOK shared edit, Resources/icons/FreeWorksWorkbench.svg
  - phase: 01-solidworks-mode-foundation (01-03)
    provides: workflow_call CI gate pattern (sub_lint/sub_fwHeadlessCompat) wired into CI_primary.yml
provides:
  - ASSET_PROVENANCE.md provenance ledger (one row per binary image -> recreated-original source)
  - tools/fw-provenance-guard.sh (CI guard rejecting any unlisted binary image; self-test proves rejection)
  - tools/fw-string-leak-grep.sh (fails on any "SolidWorks" token under src/Gui/FreeWorks/ except allow-listed Gui::SolidWorksNavigationStyle)
  - tools/fw-sync-upstream.sh (pinned-commit upstream-sync drill asserting all shared-file touches are // SW-FORK HOOK-greppable)
  - .github/workflows/sub_fwForkGuards.yml (workflow_call CI gate running provenance guard + leak grep)
affects: [phase-02-commandmanager-ribbon, phase-03-featuremanager-tree, upstream-merge-discipline, asset-legal-posture]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Asset-provenance ledger + CI guard: every tracked binary image must have an ASSET_PROVENANCE.md row or CI fails (supply-chain/legal gate)"
    - "Trademark leak grep: zero 'SolidWorks' identifiers/strings in new FreeWorks code except the single upstream type-name Gui::SolidWorksNavigationStyle"
    - "Pinned-commit upstream-sync drill enumerating // SW-FORK HOOK markers to keep shared-file divergence auditable"
    - "workflow_call reusable fork-guards gate (mirrors sub_lint.yml) slotted into CI_primary.yml"

key-files:
  created:
    - ASSET_PROVENANCE.md
    - tools/fw-provenance-guard.sh
    - tools/fw-string-leak-grep.sh
    - tools/fw-sync-upstream.sh
    - .github/workflows/sub_fwForkGuards.yml
  modified:
    - .github/workflows/CI_primary.yml

key-decisions:
  - "Asset-provenance enforced as a blocking CI gate: every binary image under tracked paths must carry an ASSET_PROVENANCE.md row marked recreated-original; the guard self-test (unlisted image -> non-zero) is part of the gate"
  - "D-03 trademark posture enforced by leak grep with exactly one allow-listed token (Gui::SolidWorksNavigationStyle); any other 'SolidWorks' identifier or user-facing string fails CI"
  - "Upstream-sync is a scripted drill against a PINNED commit; exactly two shared-file touches are expected and both are // SW-FORK HOOK-greppable (src/Gui/CMakeLists.txt add_subdirectory, FwNavigationDefault.cpp nav write)"
  - "Task 4 blocking human-verify checkpoint approved by the user ('approved'); the three guards + sync drill + asset ledger were independently re-verified by the executor and the orchestrator (no fabricated results)"

patterns-established:
  - "Provenance gate: tracked binary image without an ASSET_PROVENANCE.md row -> CI failure naming the offending file"
  - "Trademark gate: single-token allow-list ('Gui::SolidWorksNavigationStyle') so any stray SolidWorks label/identifier under src/Gui/FreeWorks/ fails"
  - "Merge-discipline gate: shared-file diffs outside src/Gui/FreeWorks/ must be // SW-FORK HOOK-marked; the pinned-commit drill enumerates and asserts them"

requirements-completed: [SHELL-02]

# Metrics
duration: 6min
completed: 2026-06-07
---

# Phase 01 Plan 04: Merge-safety, Asset Provenance & Trademark Discipline Summary

**An asset-provenance ledger + CI guard (rejects any unlisted binary image), a "SolidWorks"-string leak grep allow-listing only the upstream Gui::SolidWorksNavigationStyle type-name, and a pinned-commit upstream-sync drill asserting every shared-file touch is // SW-FORK HOOK-greppable — all wired into CI via sub_fwForkGuards.yml.**

## Performance

- **Duration:** ~6 min (continuation/finalization session)
- **Started:** 2026-06-07T09:44:31-05:00 (task-1 commit)
- **Completed:** 2026-06-07T09:50:16-05:00 (task-3 commit) + finalization
- **Tasks:** 4/4
- **Files modified:** 6 (5 created, 1 modified)

## Accomplishments
- `ASSET_PROVENANCE.md` provenance ledger: SPDX-headed, one row per binary image, including `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg` marked **recreated-original** (in-house authored "FW" mark; no proprietary SolidWorks asset reproduced, traced, or derived) per D-03.
- `tools/fw-provenance-guard.sh`: enumerates tracked binary images and fails (naming the offending file) on any lacking a ledger row; accepts a directory arg for the self-test that proves rejection of an unlisted image.
- `tools/fw-string-leak-grep.sh`: greps `src/Gui/FreeWorks/` and fails on any `SolidWorks` token except the single allow-listed `Gui::SolidWorksNavigationStyle` upstream type-name (so a stray `SolidWorksWorkbench` or user-facing label still fails).
- `tools/fw-sync-upstream.sh`: pinned-commit upstream-sync drill that creates a mirror branch, enumerates `// SW-FORK HOOK` markers via `grep -rn`, and asserts every unavoidable shared-file touch outside `src/Gui/FreeWorks/` is marked (the two expected: `src/Gui/CMakeLists.txt`, `FwNavigationDefault.cpp`).
- `.github/workflows/sub_fwForkGuards.yml`: `workflow_call` gate running the provenance guard + leak grep, referenced from `CI_primary.yml`.

## Task Commits

Each task was committed atomically:

1. **Task 1: ASSET_PROVENANCE.md ledger + provenance-guard script** — `b64e154102` (feat)
2. **Task 2: "SolidWorks"-string leak grep (allow-lists only Gui::SolidWorksNavigationStyle)** — `fdd3738e84` (feat)
3. **Task 3: Upstream-sync drill + fork-guards CI gate** — `e0339b83b1` (feat)
4. **Task 4: Confirm trademark/asset discipline + sync-drill outcome** — blocking human-verify checkpoint, **approved** (no code commit; verification record below)

**Plan metadata:** finalization docs commit (this summary + STATE.md + ROADMAP.md + REQUIREMENTS.md)

## Files Created/Modified
- `ASSET_PROVENANCE.md` (created) — Provenance ledger; one row per binary image -> recreated-original source; consulted by the guard.
- `tools/fw-provenance-guard.sh` (created) — Fails (non-zero) when any tracked binary image lacks a ledger row; directory-arg self-test path.
- `tools/fw-string-leak-grep.sh` (created) — Fails on any "SolidWorks" token under `src/Gui/FreeWorks/` except `Gui::SolidWorksNavigationStyle`.
- `tools/fw-sync-upstream.sh` (created) — Pinned-commit upstream-sync drill; enumerates and asserts `// SW-FORK HOOK` markers.
- `.github/workflows/sub_fwForkGuards.yml` (created) — `workflow_call` CI gate running provenance guard + leak grep.
- `.github/workflows/CI_primary.yml` (modified) — References `./.github/workflows/sub_fwForkGuards.yml`.

## Verification Evidence (Task 4 — independently re-confirmed)

Re-run by the executor at finalization (matching the orchestrator's independent run and the user's "approved" sign-off):

| Check | Command | Result |
|-------|---------|--------|
| Provenance guard (current tree) | `bash tools/fw-provenance-guard.sh` | exit 0 |
| Provenance guard self-test (unlisted image) | `bash tools/fw-provenance-guard.sh /tmp/fwprov-test` | exit 1 (rejects, as required) |
| "SolidWorks"-leak grep | `bash tools/fw-string-leak-grep.sh` | exit 0 (only `Gui::SolidWorksNavigationStyle`) |
| Upstream-sync drill (pinned commit) | `bash tools/fw-sync-upstream.sh` | exit 0 (completes) |
| Shared-file markers | `grep -rln "SW-FORK HOOK" src/` | exactly two: `src/Gui/CMakeLists.txt`, `src/Gui/FreeWorks/FwNavigationDefault.cpp` |
| Asset ledger | `ASSET_PROVENANCE.md` | `FreeWorksWorkbench.svg` listed as recreated-original |

## Decisions Made
- **Task 4 blocking human-verify approved by the user** ("approved"). All three guards pass, the sync drill completes against the pin, and the asset ledger is clean. The verification was independently re-confirmed by the executor at finalization and by the orchestrator; no build/CI/launch results were fabricated.
- **Single-token trademark allow-list.** Only `Gui::SolidWorksNavigationStyle` (the legitimate upstream type-name) is allowed; this keeps the D-03 posture strict while permitting the one unavoidable reference.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None. Task 4 is a blocking human-verify gate by design; resolved by user approval after the executor and orchestrator independently re-ran all guards.

## User Setup Required
None - no external service configuration required. The fork-guards run locally and in CI via `sub_fwForkGuards.yml`.

## Next Phase Readiness
- Phase 1 merge-safety/asset/trademark foundation complete: asset provenance, "SolidWorks"-leak, and pinned-commit upstream-sync drill are scripted and gated in CI.
- This is the final plan of Phase 01 (4/4). Phase is complete pending the end-of-phase verifier.
- Carried-forward concern (from Plans 02/03): live tri-OS GUI launch + CI-green confirmation recorded against `TRIOS_LAUNCH_CHECKLIST.md` once a build environment is available.

## Self-Check: PASSED

- Files verified present: `01-04-SUMMARY.md`, `ASSET_PROVENANCE.md`, `tools/fw-provenance-guard.sh`, `tools/fw-string-leak-grep.sh`, `tools/fw-sync-upstream.sh`, `.github/workflows/sub_fwForkGuards.yml`.
- Commits verified present: `b64e154102` (Task 1), `fdd3738e84` (Task 2), `e0339b83b1` (Task 3).

---
*Phase: 01-solidworks-mode-foundation*
*Completed: 2026-06-07*

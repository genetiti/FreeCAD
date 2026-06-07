---
phase: 01-solidworks-mode-foundation
plan: 02
subsystem: ui
tags: [navigation, freecad-gui, preferences, macos, navigationstyle, freeworks]

# Dependency graph
requires:
  - phase: 01-solidworks-mode-foundation (Plan 01)
    provides: FreeWorks Gui module + FwWorkbench seam, // SW-FORK HOOK marker discipline, permanent dock names
provides:
  - FwNavigationDefault — defaults the NavigationStyle preference to Gui::SolidWorksNavigationStyle when unset (no-clobber)
  - FwWorkbench::activated() wiring that applies the nav default on first activation
  - MACOS_NAV_PROFILE.md — documented macOS modifier-emulated-MMB default, GestureNavigationStyle one-click alternative, full mapping table, and Mac-hardware spike checklist
affects: [ribbon-phase-2, gestures-phase-6, nav-02-nav-03-nav-04, plan-03-gtest, plan-04-merge-safety-leak-grep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Preference-default-when-unset: read NavigationStyle key, write only if empty/absent (note A2, never clobber user choice)"
    - "// SW-FORK HOOK marker on the single shared-state write (greppable by Plan 04 sync drill)"
    - "Upstream-type-string exception (D-03): Gui::SolidWorksNavigationStyle is the sole permitted 'SolidWorks' identifier in the module"
    - "Doc-as-source-of-truth for platform UX substitutes (MACOS_NAV_PROFILE.md owns the macOS chord + spike checklist)"

key-files:
  created:
    - src/Gui/FreeWorks/FwNavigationDefault.h
    - src/Gui/FreeWorks/FwNavigationDefault.cpp
    - src/Gui/FreeWorks/MACOS_NAV_PROFILE.md
  modified:
    - src/Gui/FreeWorks/CMakeLists.txt
    - src/Gui/FreeWorks/FwWorkbench.cpp

key-decisions:
  - "NAV-01 is a preference default, not new navigation code — Gui::SolidWorksNavigationStyle already ships upstream; the fork only defaults the existing NavigationStyle key when unset."
  - "macOS default = modifier-emulated MMB (Option + left-drag), keeping the same NavigationStyle so SolidWorks button semantics are identical; Gui::GestureNavigationStyle offered as a one-click native-trackpad alternative."
  - "Task 3 (Mac-hardware feel-test) approved by documentation; live real-hardware validation deferred to the MACOS_NAV_PROFILE.md spike checklist (no Mac build tree available in this environment)."

patterns-established:
  - "Pattern: preference-default-when-unset with // SW-FORK HOOK marker for any fork-level shared-state write"
  - "Pattern: platform UX substitute documented in a dedicated profile doc that doubles as the checkpoint agenda + spike checklist"

requirements-completed: [NAV-01]

# Metrics
duration: ~2min (commits 09:19–09:21); finalized as continuation after blocking checkpoint approval
completed: 2026-06-07
---

# Phase 01 Plan 02: SolidWorks Navigation Default + macOS Substitute Profile Summary

**FwNavigationDefault defaults the FreeCAD NavigationStyle key to Gui::SolidWorksNavigationStyle when unset (no-clobber), wired into FwWorkbench::activated(), plus a documented macOS modifier-emulated-MMB profile with a GestureNavigationStyle alternative and a real-hardware spike checklist.**

## Performance

- **Duration:** ~2 min task work (09:19–09:21, 2026-06-07); finalized as a continuation after blocking-checkpoint approval
- **Started:** 2026-06-07T09:19:33-05:00 (first task commit)
- **Completed:** 2026-06-07
- **Tasks:** 3 of 3
- **Files modified:** 5 (3 created, 2 modified)

## Accomplishments
- SolidWorks mouse navigation is now the FreeWorks default on Windows/Linux out of the box (rotate = MMB drag, pan = Ctrl+MMB, zoom-to-cursor, roll = Alt+MMB, dolly = Shift+MMB, middle-click-entity then middle-drag rotates about it) — achieved by defaulting the existing NavigationStyle preference to Gui::SolidWorksNavigationStyle when unset.
- No-clobber guarantee: an existing user-chosen NavigationStyle is never overwritten (write occurs only when the key is empty/absent — note A2).
- Single shared-state write carries the `// SW-FORK HOOK` marker; `Gui::SolidWorksNavigationStyle` is the only "SolidWorks" string in the module (D-03 upstream-type exception, allow-listed for the Plan 04 leak grep).
- Explicit macOS no-middle-button / trackpad profile documented and selectable: modifier-emulated MMB (Option + left-drag) preserving SolidWorks chord layering, plus `Gui::GestureNavigationStyle` as a one-click native-trackpad alternative, a full rotate/pan/zoom/roll/dolly mapping table, and a Mac-hardware spike checklist.

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement FwNavigationDefault (default NavigationStyle to Gui::SolidWorksNavigationStyle when unset)** — `8e1f101` (feat)
2. **Task 2: Author the macOS navigation substitute profile + GestureNavigationStyle alternative** — `6081604` (docs)
3. **Task 3: Manual Mac-hardware navigation verification** — checkpoint (blocking human-verify); approved by user. No code commit; outcome recorded below.

**Plan metadata:** committed with this SUMMARY (docs: complete 01-02 plan)

## Files Created/Modified
- `src/Gui/FreeWorks/FwNavigationDefault.h` - Declares `FreeWorksGui::FwNavigationDefault` with `static void applyDefault()`.
- `src/Gui/FreeWorks/FwNavigationDefault.cpp` - Reads `User parameter:BaseApp/Preferences/View` NavigationStyle key; writes `Gui::SolidWorksNavigationStyle` only when unset, marked `// SW-FORK HOOK`; macOS path records the modifier-emulated-MMB profile.
- `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` - macOS modifier-emulated-MMB default profile, GestureNavigationStyle one-click alternative, full mapping table, and Mac-hardware spike checklist (OQ-3).
- `src/Gui/FreeWorks/CMakeLists.txt` - Adds FwNavigationDefault.{h,cpp} to module sources.
- `src/Gui/FreeWorks/FwWorkbench.cpp` - Calls `FwNavigationDefault::applyDefault()` from `activated()` alongside layout install.

## Decisions Made
- **NAV-01 implemented as a preference default, not new navigation code** — `Gui::SolidWorksNavigationStyle` already ships and is registered upstream; the fork only defaults the existing key, minimizing merge surface.
- **macOS default = modifier-emulated MMB (Option + left-drag)** keeping the same NavigationStyle so SolidWorks button semantics are identical across platforms; `Gui::GestureNavigationStyle` offered as a one-click native-trackpad alternative. The Option-based chord avoids the macOS-reserved Control+click = secondary-click conflict.
- **Task 3 approved by documentation, live hardware feel-test deferred** — there is no real Mac build tree in this environment, so the live trackpad/2-button-mouse feel-test is intentionally carried by the spike checklist in MACOS_NAV_PROFILE.md §4. No hardware test results were fabricated.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None during the planned task work. Task 3 is a blocking human-verify checkpoint by design; it was reached, the user responded "approved", and execution resumed as a continuation to finalize.

## Checkpoint Resolution (Task 3 — blocking human-verify)

- **Type:** checkpoint:human-verify (gate="blocking")
- **User response:** "approved"
- **Resolution:** The SolidWorks-style navigation default and the documented Option-chord macOS profile are approved as-is. The live Mac-hardware feel-test is **deferred** to the MACOS_NAV_PROFILE.md §4 spike checklist, which remains the record for later real-hardware validation (no Mac build tree available here). No hardware results were fabricated.
- **Status:** Approved (verified-by-documentation, with hardware validation deferred to the spike checklist).

## User Setup Required
None - no external service configuration required.

For real-hardware validation later: build FreeCAD with FreeWorks on a Mac, then walk the spike checklist in `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` §4 (rotate/pan/zoom/roll/dolly feel, gesture conflicts, modifier ergonomics, GestureNavigationStyle alternative, no-clobber, 2-button-mouse path).

## Next Phase Readiness
- NAV-01 satisfied: SolidWorks navigation is the default on Windows/Linux, with an explicit, selectable macOS profile.
- Plan 03 can assert the NavigationStyle default via its GTest (`NavigationStyle` default + no-clobber).
- Plan 04's leak grep should allow-list exactly `Gui::SolidWorksNavigationStyle` and verify the single `// SW-FORK HOOK` write.
- Deferred: live Mac-hardware navigation feel-test (tracked in MACOS_NAV_PROFILE.md §4 spike checklist).

## Self-Check: PASSED

- FwNavigationDefault.{h,cpp}, MACOS_NAV_PROFILE.md, 01-02-SUMMARY.md all present on disk.
- Task commits 8e1f101 and 6081604 present in git history.

---
*Phase: 01-solidworks-mode-foundation*
*Completed: 2026-06-07*

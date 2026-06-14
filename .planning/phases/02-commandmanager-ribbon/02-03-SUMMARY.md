---
phase: 02-commandmanager-ribbon
plan: 03
subsystem: ui
tags: [qt6, ribbon, qtoolbar, qmainwindow-savestate, parametergrp, freeworks, commandmanager]

# Dependency graph
requires:
  - phase: 02-01
    provides: FwRibbon base (QTabWidget shell) + headless GUI test bootstrap (ensureGuiTestBootstrap)
  - phase: 02-02
    provides: FwRibbon build API (buildFromCuratedMap/buildAutoDerived/setCurrentTab), curated 3-tab map, unique Fw_RibbonPanel_* objectNames
  - phase: 01
    provides: FwLayout::install dock shell + FwWorkbench activation lifecycle
provides:
  - FwLayout::mountRibbon/unmountRibbon — ribbon wrapped in a real Gui::ToolBar (Fw_RibbonToolBar) added via addToolBar(Qt::TopToolBarArea, wrapper); idempotent
  - FwLayout::hideStockChrome/restoreStockChrome — reversible, snapshot-driven stock menu bar + toolbar hide/restore (macOS native-menu aware)
  - FwWorkbench activated() mounts+hides, deactivated() restores+unmounts (FreeWorks-scoped)
  - FwRibbon "More commands…" overflow corner widget (D-12 escape hatch) reaching the uncurated command set
  - FwRibbon saveTabState/restoreTabState — selected tab persisted separately via a FreeWorks ParameterGrp key (D-14)
affects: [02-04, phase-7-theming, phase-3-featuremanager]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Mount custom ribbon widget as a REAL QToolBar wrapper (objectName-stable) so QMainWindow::saveState() serializes it — never a bare QTabWidget child of a ToolBarAreaWidget"
    - "Reversible, FreeWorks-scoped chrome hide/restore via snapshot (toolbar names + menu-bar visibility + native-menu setting), ForceHidden/RestoreDefault round-trip"
    - "Two-layer persistence: QMainWindow toolbar state (real QToolBar) + separate ParameterGrp key for QTabWidget-only state (selected tab)"
    - "Discoverability escape hatch as a QTabWidget tab-bar corner widget, lazily populated from CommandManager::getAllCommands()"

key-files:
  created: []
  modified:
    - src/Gui/FreeWorks/FwLayout.h
    - src/Gui/FreeWorks/FwLayout.cpp
    - src/Gui/FreeWorks/FwWorkbench.h
    - src/Gui/FreeWorks/FwWorkbench.cpp
    - src/Gui/FreeWorks/FwRibbon.h
    - src/Gui/FreeWorks/FwRibbon.cpp
    - tests/src/Gui/FwRibbonWidget.cpp

key-decisions:
  - "Mount via real Gui::ToolBar wrapper (Fw_RibbonToolBar) + addToolBar(Qt::TopToolBarArea, ...), NOT toolBarAreaWidget — the REVIEW-corrected seam that actually round-trips through saveState"
  - "Selected tab persisted separately via ParameterGrp (User parameter:BaseApp/Preferences/FreeWorks/Ribbon, currentTab int) because QMainWindow::saveState does not cover a QTabWidget's selected tab"
  - "Chrome restore is snapshot-driven (setVisible(snapshot) + setNativeMenuBar(snapshot)), never an unconditional reveal, to avoid clobbering fullscreen/native state on macOS"
  - "Overflow menu reuses each command's existing QAction via Command::addTo() — thin trigger, no new backend, destructive commands keep their downstream confirmation"
  - "Build suppresses the transient currentChanged storm (m_suppressTabStateSave) so a rebuild's clear+repopulate cannot clobber the stored tab selection before restoreTabState runs"

patterns-established:
  - "Real-QToolBar-wrapper mount seam for any future custom top-area surface that must persist"
  - "Snapshot/restore chrome toggling scoped to a workbench's activate/deactivate"

requirements-completed: [RIBBON-01]

# Metrics
duration: 6min
completed: 2026-06-13
---

# Phase 2 Plan 03: Mount the Ribbon as the App's Command Surface Summary

**FwRibbon wired into the live window as a real Gui::ToolBar (Fw_RibbonToolBar) in Qt::TopToolBarArea, with reversible snapshot-driven stock-chrome hide/restore (D-11), a "More commands…" overflow escape hatch (D-12), and two-layer persistence — QMainWindow toolbar state + a separate ParameterGrp tab-state key (D-14).**

## Performance

- **Duration:** ~6 min
- **Started:** 2026-06-13T23:58:23Z
- **Completed:** 2026-06-14T00:04:00Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- `FwLayout::mountRibbon()` builds the ribbon (curated map; auto-derive fallback for an uncurated active workbench), wraps it in a **real** `Gui::ToolBar` (`objectName Fw_RibbonToolBar`, non-movable/non-floatable) and mounts it via `Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea, wrapper)` — a genuine `QMainWindow::saveState()` participant (REVIEW concerns 3 & 4). Idempotent: a second activation reuses the existing wrapper instead of adding a duplicate; `unmountRibbon()` removes it predictably.
- `FwLayout::hideStockChrome()/restoreStockChrome()` hide every stock toolbar (by name, excluding the ribbon wrapper) via `ToolBarManager::setState(..., ForceHidden)` and the menu bar via `menuBar()->hide()`, after SNAPSHOTTING the hidden toolbar names + menu-bar visibility + (macOS) native-menu setting; restore replays exactly that snapshot (`RestoreDefault` + `setNativeMenuBar(snapshot)` + `setVisible(snapshot)`).
- `FwWorkbench::activated()` mounts+hides after `StdWorkbench::activated()`; a new `deactivated()` override restores chrome + unmounts so stock workbenches keep their chrome (Pitfall 3).
- `FwRibbon` gained a pinned **"More commands…"** overflow `QToolButton` installed as the top-right tab-bar **corner widget** (`setCornerWidget`), its menu built lazily from `CommandManager::getAllCommands()` grouped by app module — each entry the command's existing `QAction` (D-12, no stranded commands).
- `FwRibbon::saveTabState()/restoreTabState()` persist the selected tab index separately to a FreeWorks `ParameterGrp` key (`SetInt`/`GetInt`), because `QMainWindow::saveState()` does not serialize a `QTabWidget`'s selected tab (REVIEW concern 4).

## Task Commits

Each task was committed atomically:

1. **Task 1: Mount ribbon in TopToolBarArea + reversible chrome hide/restore (D-11)** - `310f6175c7` (feat)
2. **Task 2: Discoverability escape hatch (D-12) + two-layer persistence (D-14)** - `bcc3b5b5f1` (feat)

**Plan metadata:** _(final docs commit below)_

## Files Created/Modified
- `src/Gui/FreeWorks/FwLayout.h` - Declares `mountRibbon`/`unmountRibbon`/`hideStockChrome`/`restoreStockChrome` + chrome-snapshot statics.
- `src/Gui/FreeWorks/FwLayout.cpp` - Real-QToolBar wrapper mount via `addToolBar(Qt::TopToolBarArea, ...)`; idempotent reuse; snapshot-driven chrome hide/restore (`ForceHidden`/`RestoreDefault`, menu-bar + native-menu snapshot).
- `src/Gui/FreeWorks/FwWorkbench.{h,cpp}` - `activated()` mounts+hides; new `deactivated()` restores+unmounts.
- `src/Gui/FreeWorks/FwRibbon.{h,cpp}` - "More commands…" overflow corner widget + lazy menu from `getAllCommands()`; `saveTabState`/`restoreTabState` against a ParameterGrp key; rebuild-suppression guard.
- `tests/src/Gui/FwRibbonWidget.cpp` - QTEST_MAIN tests against a real `Gui::MainWindow`: mount in TopToolBarArea, idempotent remount, chrome round-trip, overflow corner-widget presence + reachable menu, toolbar saveState/restoreState restores area, tab-index round-trip restores index 2.

## Mount, Chrome & Persistence Detail (per <output> spec)

- **Mount seam:** `FwRibbon` → wrapped in `Gui::ToolBar` (`objectName Fw_RibbonToolBar`, `setMovable(false)`, `setFloatable(false)`, `addWidget(fwRibbon)`) → `Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea, wrapper)`. Idempotent find-or-reuse via `mw->findChild<QToolBar*>("Fw_RibbonToolBar")`. **Not** `toolBarAreaWidget(...)` (grep-asserted 0 occurrences).
- **Chrome hide sequence:** snapshot `{hidden toolbar names, menuBar()->isVisible(), menuBar()->isNativeMenuBar()}` → `ToolBarManager::setState(names, ForceHidden)` + `menuBar()->hide()`.
- **Chrome restore sequence:** `ToolBarManager::setState(names, RestoreDefault)` → `menuBar()->setNativeMenuBar(snapshotNative)` → `menuBar()->setVisible(snapshotVisible)`. No unconditional reveal (grep-asserted 0 `menuBar()->show()`).
- **Escape hatch:** pinned "More commands…" corner-widget `QToolButton` (lazy `QMenu` from `getAllCommands()` grouped by module, each a `Command::addTo()` of the existing `QAction`) + the baseline that `QAction` shortcuts survive `menuBar()->hide()`.
- **Two-layer persistence:** (1) toolbar presence/area/order via `QMainWindow::saveState()/restoreState()` (real `Fw_RibbonToolBar` QToolBar); (2) selected tab via `ParameterGrp` key `User parameter:BaseApp/Preferences/FreeWorks/Ribbon` → `currentTab` int.

## Decisions Made
See `key-decisions` frontmatter. All decisions honor the load-bearing REVIEW corrections (real-QToolBar wrapper mount, separate tab-state key, snapshot-driven chrome restore) exactly as the dependency gate required.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Suppress transient currentChanged storm during rebuild**
- **Found during:** Task 2 (tab-state persistence)
- **Issue:** Wiring `QTabWidget::currentChanged` → `saveTabState()` meant a rebuild's `clearTabs()` + re-`addTab()` would fire `currentChanged` with transient indices, persisting (and clobbering) the user's stored selection BEFORE `restoreTabState()` could re-apply it.
- **Fix:** Added an `m_suppressTabStateSave` guard set for the duration of `buildFromCuratedMap()`/`buildAutoDerived()`; `saveTabState()` early-returns while suppressed; the flag is cleared immediately before `restoreTabState()`.
- **Files modified:** src/Gui/FreeWorks/FwRibbon.h, src/Gui/FreeWorks/FwRibbon.cpp
- **Verification:** `test_TabStateRoundTripRestoresIndex` asserts a fresh ribbon picks up index 2 after build (CI-green when the matrix runs).
- **Committed in:** `bcc3b5b5f1` (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 bug).
**Impact on plan:** Necessary for the persistence requirement to actually hold; no scope creep.

## Issues Encountered
None during planned work beyond the deviation above.

## Deferred Obligations (no build tree in this environment)

Per the established Phase 1 / 02-01 / 02-02 constraint, there is **no FreeCAD build tree, compiler, Qt, or ctest** here. The following remain deferred (NOT fabricated):

- **`ctest -R "Gui_tests_run|FwRibbonWidget_Tests_run" --output-on-failure`** — the headless QTEST_MAIN assertions (mount in `Qt::TopToolBarArea`, idempotent remount, chrome round-trip, overflow corner widget + reachable menu, toolbar saveState/restoreState restores area, tab-index restore) are authored compile-intended and CI-green-by-construction, but cannot be executed here. Run when a build matrix is available.
- **Live menu-bar round-trip + macOS native-menu restore** — VALIDATION.md manual checklist item (requires a live GUI on macOS).
- **True app-restart persistence** (close FreeCAD, relaunch FreeWorks, confirm toolbar area + selected tab restored) — VALIDATION.md manual checklist item; the in-process round-trips are covered headless.
- **Flyout popup visual** — VALIDATION.md manual item (carried from 02-02).

`tools/fw-string-leak-grep.sh` WAS run locally and is clean (only the allow-listed `Gui::SolidWorksNavigationStyle` token present).

## Next Phase Readiness
- Plan 02-04 (`FwRibbonContext`) can subscribe to edit signals and call the existing `FwRibbon::setCurrentTab` to drive context tab switching — the mount/persistence/escape-hatch surface is in place and self-contained (additive module, no shared edits).
- No blockers. The ribbon is now the FreeWorks command surface end-to-end (mount + chrome replacement + discoverability + persistence), pending the deferred live-build verifications.

## Self-Check: PASSED

All modified files exist on disk; both task commits (`310f6175c7`, `bcc3b5b5f1`) exist in git history.

---
*Phase: 02-commandmanager-ribbon*
*Completed: 2026-06-13*

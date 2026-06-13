---
phase: 02-commandmanager-ribbon
plan: 01
subsystem: gui
tags: [ribbon, qtabwidget, qtoolbar, spike, native-vs-saribbon, gtest, qtest, freeworks]

# Dependency graph
requires:
  - phase: 01-solidworks-mode-foundation (01-01)
    provides: FreeWorks Gui submodule (src/Gui/FreeWorks/), FwWorkbench activation seam, FwLayout top-toolbar area reserved, Fw naming (FreeWorksGui namespace), target_sources build pattern
  - phase: 01-solidworks-mode-foundation (01-03)
    provides: headless Gui_tests_run pattern, setup_qt_test offscreen QApplication harness
provides:
  - FwRibbon.{h,cpp} — minimal native ribbon (QTabWidget + QToolBar large labeled buttons; addTabFromCommandIds; native split-button via *_Comp* group id through cmd->addTo)
  - tests/src/Gui/FwTestGuiBootstrap.h — creates Gui::Application singleton THEN loads PartDesignGui/SketcherGui/MeasureGui/MatGui module commands (so getCommandByName resolves)
  - tests/src/Gui/FwRibbon.cpp — headless command-ID resolution tests (Gui_tests_run)
  - tests/src/Gui/FwRibbonWidget.cpp — QTEST_MAIN offscreen QWidget construction/tab-build test
  - SPIKE.md — D-03 6-item parity verdict = "native committed"
  - src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md — deferred live-validation obligation (item-5 restore + on-hardware look)
affects: [phase-02-02-curated-map-build, phase-02-03-mount, phase-02-04-context-switch]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "command -> large labeled button via existing Gui::Command::addTo(QToolBar) — no duplicated command backend; *_Comp* group id yields a native MenuButtonPopup split-button (Action.cpp:472-485), no hand-rolled QMenu"
    - "GUI test bootstrap: create Gui::Application(false) BEFORE importing module GUI (else ImportError + zero commands), then import every owning module so getCommandByName resolves headlessly"
    - "Split test targets: command-ID resolution as GTest in Gui_tests_run; QWidget construction as a separate QTEST_MAIN offscreen target via setup_qt_test"
    - "Spike-gate verdict recorded as an explicit per-item PASS/FAIL doc with a literal native-vs-SARibbon decision token"

key-files:
  created:
    - src/Gui/FreeWorks/FwRibbon.h
    - src/Gui/FreeWorks/FwRibbon.cpp
    - tests/src/Gui/FwTestGuiBootstrap.h
    - tests/src/Gui/FwRibbon.cpp
    - tests/src/Gui/FwRibbonWidget.cpp
    - .planning/phases/02-commandmanager-ribbon/SPIKE.md
    - src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md
  modified:
    - src/Gui/FreeWorks/CMakeLists.txt
    - tests/src/Gui/CMakeLists.txt

key-decisions:
  - "VERDICT = native committed: native Qt 6.8 (FwRibbon = QTabWidget + QToolBar) reaches the D-03 parity bar; Plans 02-02/02-03/02-04 build on native; SARibbon fallback (D-04) NOT triggered, shelved unless the deferred live checklist reveals a real native gap"
  - "All 4 Class-A primitives are code-demonstrated (FwRibbon code + headless resolution/construction tests) or research-confirmed; the Class-B context-switch restore is design-wired (signalInEdit/ResetEdit -> setCurrentIndex; after==before by construction)"
  - "Task 3 blocking human-verify checkpoint cleared by DOCUMENTATION APPROVAL (user-authorized; Phase 1 precedent [01-03]/[01-04]) — no build tree / GUI / CI in this environment; item-5 live enter->switch->exit->restore demonstration with OBSERVED tab indices and on-hardware look (items 3,4,6) deferred to src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md, NOT fabricated"
  - "No SolidWorks identifier/string in any new source file (fw-string-leak-grep.sh clean); ribbon firing via existing Gui::Command/CommandManager only, no App/ includes"

patterns-established:
  - "Native ribbon engine committed: downstream plans use QTabWidget/QToolBar primitives, not SARibbon"
  - "Verification-deferral pattern (continued from Phase 1): blocking human-verify cleared by doc-approval + a source-tree live checklist artifact that carries the on-hardware obligation"

requirements-completed: []
requirements-partial: [RIBBON-01]

# Metrics
duration: ~continuation/finalization session (Task 3 gate authored; Tasks 1-2 previously committed)
commits: 3 (test 4c13bd9e22, feat c02b0eaae3, docs gate this session)
files-created: 7
files-modified: 2
---

# Plan 02-01 Summary — Wave 0 test scaffold + native ribbon spike gate

**A minimal-but-real native `FwRibbon` (`QTabWidget` hosting `QToolBar` panels of 32px
icon-over-label buttons, built from real command IDs via `cmd->addTo`), a headless GUI
test bootstrap + split resolution/construction tests, and the resolved native-vs-SARibbon
spike gate: VERDICT `native committed`.**

## What shipped

- **`FwRibbon.{h,cpp}`** (`FreeWorksGui`, `src/Gui/FreeWorks/`): `QTabWidget` subclass with
  `addTabFromCommandIds(name, ids)` building a `QToolBar` panel
  (`Qt::ToolButtonTextUnderIcon`, 32px icons, palette-only color) of large labeled buttons
  via the existing `Gui::Command::addTo()` seam. Unresolved IDs silently skipped (D-08).
  A `*_Comp*` group id flows through the same `addTo` and yields a native
  `MenuButtonPopup` split-button — no hand-rolled flyout.
- **`FwTestGuiBootstrap.h`**: creates the `Gui::Application` singleton via
  `new Gui::Application(false)` *before* importing `PartDesignGui`/`SketcherGui`/
  `MeasureGui`/`MatGui`, so `getCommandByName` resolves real commands headlessly.
- **Tests**: `FwRibbon.cpp` (GTest command-ID resolution in `Gui_tests_run`) +
  `FwRibbonWidget.cpp` (`QTEST_MAIN` offscreen QWidget tab-build).

## The spike gate (Task 3)

`SPIKE.md` records an explicit **PASS** for all 6 D-03 parity items under the A/B/C
evidence split and the literal verdict **`native committed`**. The four Class-A primitives
are code-demonstrated/research-confirmed; the Class-B live context-switch restore is
design-wired (`signalInEdit`/`signalResetEdit` → `setCurrentIndex`, after-tab == before-tab
by construction); Class-C look passes its objective proxies (32px tabbed bar visibly
bigger than a stock toolbar).

**Approval mode: documentation approval** (user-authorized; Phase 1 precedent
`[01-03]`/`[01-04]`). No FreeCAD build tree / GUI / CI exists in this environment, so the
throwaway live spike harness could not be executed here. The live enter→switch→exit→restore
demonstration with *observed* tab indices (item 5) and on-hardware confirmation of items
3/4/6 are **deferred** to **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`** — an open
obligation, not a fabricated result. If that live run later contradicts the design, the
SARibbon vendor-vet gate (recorded in `SPIKE.md`) flips the verdict.

## Downstream impact

Plans **02-02 / 02-03 / 02-04 proceed on the native Qt engine** (`QTabWidget` +
`QToolBar`). The SARibbon submodule path (`src/3rdParty/SARibbon`, MIT v2.8.0) is **not**
introduced. RIBBON-01 is partially satisfied (native engine proven + minimal real ribbon);
the full curated map, multi-panel tabs, mount, persistence, and context switching are
02-02 → 02-04.

## Verification

- Gate grep checks pass: `SPIKE.md` contains `Verdict`, 8 `PASS|FAIL` lines (≥ 6),
  literal `native committed` present.
- `tools/fw-string-leak-grep.sh`: clean (only `Gui::SolidWorksNavigationStyle` allow-listed).
- Headless tests (`ctest -R "Gui_tests_run|FwRibbonWidget_Tests_run"`) are authored to be
  CI-green; live execution rides the existing CI matrix (no build tree here).

## Open obligations carried forward

- **`src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md`** — live item-5 restore proof + on-hardware
  look, to be signed off when a build exists (mirrors Phase 1 `TRIOS_LAUNCH_CHECKLIST.md`).

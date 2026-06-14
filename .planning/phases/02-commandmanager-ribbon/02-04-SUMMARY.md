---
phase: 02-commandmanager-ribbon
plan: 04
subsystem: gui
tags: [ribbon, context-switch, edit-signals, fastsignals, qpointer, sketch, freeworks, RIBBON-02]

# Dependency graph
requires:
  - phase: 02-02
    provides: FwRibbon::setCurrentTab/currentIndex seam + curated "Sketch" tab (index 1)
  - phase: 02-03
    provides: FwLayout::mountRibbon/unmountRibbon (real Fw_RibbonToolBar wrapper) + idempotent mount lifecycle
  - phase: 01
    provides: FwWorkbench activate/deactivate lifecycle that drives mount/unmount
provides:
  - FwRibbonContext — event-driven subscriber to Gui::Application signalInEdit/signalResetEdit that drives the active ribbon tab (RIBBON-02)
  - FwRibbonContext pure switch core (decideOnEnter/decideOnReset → TabAction enum) — headless-testable state machine, no ribbon/index coupling
  - FwRibbonContext::isSketchType — sketch identity by the type-name string "SketcherGui::ViewProviderSketch" (zero Sketcher link dependency)
  - FwLayout owns/binds/tears-down one FwRibbonContext per mounted ribbon (s_ribbonContext), released on unmountRibbon()
affects: [phase-3-featuremanager, phase-5-selection, phase-7-theming]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Contextual ribbon tab switch via app-level edit signals (signalInEdit/signalResetEdit) — event-driven, never polling Control::activeDialog() (Pitfall 1)"
    - "Module-decoupled type identity: detect a foreign view provider by getTypeId().getName() STRING, no cross-module include (Pitfall 2)"
    - "Pure switch core (decideOnEnter/decideOnReset returning an action enum, no widget/index) so the concern-7 state machine is unit-testable with no QWidget"
    - "Explicit contextActive_ state machine gates nested enters (NoOp, no re-stash) and stray resets (NoOp) so the remembered tab is never corrupted"
    - "QPointer<widget> for a signal subscriber's non-owning target so a signal firing after teardown is a guarded no-op, not a dangling deref"
    - "Scoped fastsignals connections owned by a unique_ptr context; teardown resets the owner BEFORE the target is destroyed"

key-files:
  created:
    - src/Gui/FreeWorks/FwRibbonContext.h
    - src/Gui/FreeWorks/FwRibbonContext.cpp
  modified:
    - src/Gui/FreeWorks/FwLayout.h
    - src/Gui/FreeWorks/FwLayout.cpp
    - src/Gui/FreeWorks/CMakeLists.txt
    - tests/src/Gui/FwRibbon.cpp
    - tests/src/Gui/FwRibbonWidget.cpp

key-decisions:
  - "RIBBON-02 wired to Gui::Application signalInEdit/signalResetEdit (event-driven) — the documented reinterpretation of D-09's 'Gui::Control active-dialog/edit state'; Control's accessor has no change signal so polling it would be laggy/racy (Pitfall 1)"
  - "Sketch identified ONLY by the type-name literal 'SketcherGui::ViewProviderSketch' — zero compile/link dependency on the Sketcher module (Pitfall 2); the literal is an external contract requiring live re-validation (REVIEW LOW)"
  - "Pure core returns a TabAction enum (SwitchToSketch/NoOp/RestorePrevious), NEVER a raw tab index — the ribbon-bound layer resolves the name/index; keeps the state machine headless-testable (REVIEW MEDIUM)"
  - "Explicit contextActive_ guard: a nested/repeated sketch enter is NoOp and does NOT re-stash previousIndex_ (concern 7); a stray reset while inactive is NoOp (never forces a stale tab)"
  - "Ribbon held via QPointer<FwRibbon>; scoped fastsignals connections released in disconnect()/dtor; FwLayout resets s_ribbonContext BEFORE tearing down the ribbon so no signal fires into a half-removed ribbon (REVIEW MEDIUM / threat T-02-09)"

patterns-established:
  - "Event-driven contextual-tab subscriber pattern for any future selection/edit-driven ribbon behavior (broader contextual tabs are deferred past v1)"
  - "Pure decision-core + thin signal-bound adapter split as the headless-testability pattern for signal-driven Gui logic"

requirements-completed: [RIBBON-02]

# Metrics
duration: 5min
completed: 2026-06-14
---

# Phase 2 Plan 04: Context Tab Switching (FwRibbonContext) Summary

**RIBBON-02 delivered: `FwRibbonContext` subscribes to `Gui::Application::Instance->signalInEdit`/`signalResetEdit` and drives the active ribbon tab — entering a sketch auto-activates the "Sketch" tab (context wins, even over a manual selection), and leaving restores the previously active tab — via an explicit `contextActive_` state machine that ignores nested sketch enters and no-ops stray resets, a pure `TabAction`-returning core (no hardcoded indices), a `QPointer` guard against post-teardown signals, and sketch identity by the type-name string `"SketcherGui::ViewProviderSketch"` with ZERO Sketcher link dependency. `FwLayout` owns one context bound to the mounted ribbon and releases it on teardown.**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-06-14T00:08:36Z
- **Completed:** 2026-06-14T00:13:41Z
- **Tasks:** 2 (Task 1 TDD: RED + GREEN)
- **Files created:** 2; **modified:** 5

## What shipped

### Task 1 — `FwRibbonContext.{h,cpp}` (RED `1974548a43`, GREEN `945fe2223b`)

- **Pure switch core (headless-testable, no ribbon/index):**
  - `TabAction decideOnEnter(bool isSketch, int currentIndex)` — first sketch enter while
    `!contextActive_`: stash `previousIndex_=currentIndex`, set `contextActive_=true`, return
    `SwitchToSketch` (D-09 context wins / D-10 remember). A nested/repeated sketch enter while
    already active: `NoOp` and **no re-stash** (the concern-7 guard — so reset restores the
    ORIGINAL pre-sketch tab, never Sketch). A non-sketch enter: `NoOp`, state untouched (D-09
    sketch-only v1).
  - `TabAction decideOnReset()` — `RestorePrevious` + clear `contextActive_` when active;
    `NoOp` when inactive (a stray reset never forces a stale tab — REVIEW MEDIUM).
  - `bool isSketchType(const std::string&)` — `== "SketcherGui::ViewProviderSketch"` only.
- **Signal subscription:** `connect()` mirrors `OverlayManager.cpp:406-409`, storing the
  returned `fastsignals::scoped_connection`s as members. The inEdit lambda reads
  `vp.getTypeId().getName()`, runs `isSketchType`→`decideOnEnter`, and on `SwitchToSketch`
  (guarded by a non-null `QPointer`) calls `ribbon_->setCurrentTab("Sketch")`. The resetEdit
  lambda runs `decideOnReset()` and on `RestorePrevious` calls
  `ribbon_->setCurrentIndex(previousIndex_)`. `connect()` is idempotent (reassigning the
  scoped connections drops any prior subscription). `disconnect()` (and the destructor)
  release them.
- **D-09 reinterpretation** documented in a code comment: D-09's "wired to `Gui::Control`
  active-dialog/edit state" is realized via the edit signals because Control's accessor has no
  change signal (Pitfall 1) — faithful, not a deviation.
- **RED tests** in `tests/src/Gui/FwRibbon.cpp` (`Gui_tests_run`, pure logic, no QWidget):
  type-name match, sketch-enter→`SwitchToSketch`+remember, **nested-enter→`NoOp`+previous
  unchanged (concern-7 regression)**, non-sketch→`NoOp`, reset-active→`RestorePrevious`,
  reset-inactive→`NoOp`, context-wins-then-restore-manual.

### Task 2 — bind via `FwLayout` (`d98675eee2`)

- `FwLayout` owns one `std::unique_ptr<FwRibbonContext> s_ribbonContext` for the FreeWorks-mode
  lifetime. `bindRibbonContext(ribbon)` lazily constructs it, `setRibbon(ribbon)` (QPointer)
  and `connect()`s — called from `mountRibbon()` on BOTH the fresh-mount and idempotent-reuse
  paths so the subscription always drives the current ribbon, never a stale/duplicated one.
- `unmountRibbon()` calls `s_ribbonContext.reset()` **first** (running `~FwRibbonContext()` →
  `disconnect()`) so the scoped connections drop before the ribbon wrapper is removed — no
  signal fires into a half-removed ribbon (REVIEW MEDIUM / threat T-02-09). Exactly one context
  per mounted ribbon.
- **Widget test** (`FwRibbonWidget.cpp`, QTEST_MAIN): a context bound to a real built ribbon
  resolves `decideOnEnter(true, current)`→`SwitchToSketch` to `setCurrentTab("Sketch")`, then
  `decideOnReset()`→`RestorePrevious` returns to the original (manual) index. Documents the
  live type-name re-validation obligation.

## Edit-signal subscription pattern (per <output> spec)

- **Source:** `Gui::Application::Instance->signalInEdit` / `signalResetEdit`
  (`fastsignals::signal<void(const Gui::ViewProviderDocumentObject&)>`, `Application.h:154-156`),
  the same relay `OverlayManager` consumes.
- **Storage:** `fastsignals::scoped_connection inEditConn_/resetEditConn_` members — auto-drop on
  reassignment (idempotent `connect()`) and on destruction.
- **Sketch identity contract:** `vp.getTypeId().getName() == "SketcherGui::ViewProviderSketch"`.
  This single literal is the only coupling to Sketcher and it is a STRING, not an include — it
  **must be re-validated live** (a pure test cannot prove it still matches upstream — REVIEW LOW).
- **State machine:** `contextActive_` (bool) + `previousIndex_` (int). Enter while active → NoOp,
  no re-stash (concern 7). Reset while inactive → NoOp.
- **Dangling guard:** `QPointer<FwRibbon> ribbon_` — a signal after ribbon teardown is a no-op.
- **Ownership/teardown:** `FwLayout::s_ribbonContext` (unique_ptr) bound on mount, reset on unmount
  before the ribbon is removed.

## Deviations from Plan

None — plan executed exactly as written. The pure-core/state-machine factoring, the
`TabAction` enum (no hardcoded indices), the `QPointer` guard, the scoped-connection ownership,
the type-name-only sketch identity, and the FwLayout lifecycle/teardown all match the plan's
`<action>`/`<behavior>`/`<artifacts_this_phase_produces>` specifications. (The header doc
comments mention `Gui::Control::activeDialog()` and the Sketcher include only to explain what is
deliberately NOT done; the gated greps target `FwRibbonContext.cpp`, which is clean: 0 occurrences
of `activeDialog` and 0 of `Mod/Sketcher`.)

## Verification

Static/local checks run here (acceptance greps + leak-grep):

- `FwRibbonContext.h`: declares `class FwRibbonContext`, `enum class TabAction`, `decideOnEnter`,
  `decideOnReset`, `QPointer<FwRibbon>` (grep count 9).
- `FwRibbonContext.cpp`: contains `signalInEdit` (2), `signalResetEdit` (2),
  `SketcherGui::ViewProviderSketch` (2), `contextActive_` (5), `setCurrentTab` (2), the D-09
  reinterpretation comment (1); `grep -c 'Mod/Sketcher'` = **0** (Pitfall 2);
  `grep -c 'activeDialog'` = **0** (Pitfall 1).
- `CMakeLists.txt`: `FwRibbonContext.cpp` present in `FreeWorks_CPP_SRCS` (and `.h` in HPP).
- `FwLayout.cpp`: `FwRibbonContext` (4), `s_ribbonContext->connect()` (1),
  `s_ribbonContext.reset()` (1), `grep -c 'Mod/Sketcher'` = 0.
- **TDD gate sequence present in git log:** `test(02-04)` `1974548a43` → `feat(02-04)`
  `945fe2223b` → `feat(02-04)` `d98675eee2`.
- `tools/fw-string-leak-grep.sh`: **clean** (only the allow-listed
  `Gui::SolidWorksNavigationStyle` token present). No "SolidWorks" identifier in any new file.

## Deferred Obligations (no build tree / Qt / ctest in this environment)

Per the established Phase 1 / 02-01 / 02-02 / 02-03 constraint — there is **no FreeCAD build
tree, compiler, Qt, or ctest** here. The following are authored compile-intended and
CI-green-by-construction but cannot be executed here; they are NOT fabricated:

- **`ctest -R Gui_tests_run --output-on-failure`** — the seven pure-logic `FwRibbonContextLogic`
  tests (incl. the concern-7 nested-enter→NoOp+previous-unchanged guard and the
  reset-while-inactive→NoOp test).
- **`ctest -R "Gui_tests_run|FwRibbonWidget_Tests_run" --output-on-failure`** — the
  `test_ContextBoundToRibbonResolvesSketchTabAndRestores` widget assertion (Sketch tab via
  `setCurrentTab("Sketch")`, reset to the original index) under the offscreen QApplication.
- **Compilation** of `FwRibbonContext.{cpp,h}` + the FwLayout additions into FreeCADGui.
- **Live edit-lifecycle round-trip (the open item-5 obligation in `SPIKE_LIVE_CHECKLIST.md`):**
  in the running app, enter a sketch → confirm the Sketch tab activates → exit → confirm the
  prior tab is restored. This is GUI-only and stays a VALIDATION manual item.
- **Live re-validation of the `"SketcherGui::ViewProviderSketch"` type-name literal** — only the
  running app can prove the literal still matches upstream (REVIEW LOW); the manual sketch
  enter/exit test is what catches an upstream rename.

`tools/fw-string-leak-grep.sh` WAS run locally and is clean.

## Phase 2 status

This is the LAST plan of Phase 02 — **the phase implementation is complete.** RIBBON-01
(02-02/02-03) and RIBBON-02 (02-04) are both delivered: a curated tabbed ribbon mounted as the
single command surface with chrome replacement, an escape hatch, two-layer persistence, and now
context-driven tab switching. Remaining before the phase gate is the deferred live-build / GUI
verification track (CI matrix + tri-OS / SPIKE_LIVE_CHECKLIST manual sign-off), consistent with
the no-build-tree precedent carried since Phase 1.

## Self-Check: PASSED

- Files: `FwRibbonContext.h`, `FwRibbonContext.cpp` — FOUND on disk.
- Commits: `1974548a43`, `945fe2223b`, `d98675eee2` — FOUND in git log.

---
*Phase: 02-commandmanager-ribbon*
*Completed: 2026-06-14*

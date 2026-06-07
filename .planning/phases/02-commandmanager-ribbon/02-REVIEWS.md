---
phase: 2
cycle: 2
reviewers: [codex]
reviewed_at: 2026-06-07T18:11:13Z
plans_reviewed: [02-01-PLAN.md, 02-02-PLAN.md, 02-03-PLAN.md, 02-04-PLAN.md]
prior_cycle_high: 8
current_cycle_high: 4
---

# Cross-AI Plan Review — Phase 2 (Convergence Cycle 2)

> Cycle 1 raised 8 HIGH concerns. The plans were revised to address all 8 (each plan now
> cites "REVIEW concern N" inline). This cycle (1) confirms whether each cycle-1 HIGH is
> genuinely resolved and (2) surfaces remaining / newly-introduced HIGH concerns. Codex was
> run inside the repo so it could check claims against the live FreeCAD/Qt APIs; the
> orchestrator independently re-verified every load-bearing NEW HIGH against source before
> recording it (see "Orchestrator Verification" below).

## Codex Review

## Plan 02-01

**Summary**
The revised plan correctly separates widget construction into a Qt test target and recognizes that module GUI init is required for PartDesign/Sketcher command IDs. The remaining flaw is the bootstrap: importing `PartDesignGui`/`SketcherGui` requires a live `Gui::Application::Instance`, and the plan only specifies `tests::initApplication()` (App-only) plus creating a `QApplication`.

**Strengths**
- Correctly moves `FwRibbon : QTabWidget` construction out of `Gui_tests_run` into `setup_qt_test`/`QTEST_MAIN`.
- Correctly identifies that `PartDesign_*`/`Sketcher_*` commands register in module GUI init, not by linking `FreeCADGui`.
- Avoids QWidget construction in the GTest file.
- Uses the real `Gui::CommandManager::getCommandByName()` path.

**Concerns**
- **HIGH:** `tests::initApplication()` only calls `App::Application::init()`; it does NOT create `Gui::Application::Instance`. In this checkout, `PYMOD_INIT(PartDesignGui)` rejects loading with `ImportError: "Cannot load Gui module in console application."` when `Gui::Application::Instance` is null (verified: `AppPartDesignGui.cpp:103-105`). The proposed module-import bootstrap therefore fails — and the command-ID resolution test (the entire typo guard for RIBBON-01 SC2) asserts nothing useful.
- **MEDIUM:** A `QApplication` created in the bootstrap must be static/process-lifetime-owned, not a local temporary.
- **MEDIUM:** Tests are not gated on `BUILD_PART_DESIGN`/`BUILD_SKETCHER`/`BUILD_MEASURE` module build options.

**Suggestions**
- Add an explicit bootstrap sequence: create/verify `QApplication`; construct/initialize a `Gui::Application` singleton if `Gui::Application::Instance == nullptr`; THEN import the GUI modules. Assert `Gui::Application::Instance != nullptr` before any `commandManager()` use.
- Share the module-import list with Plan 02 and include `MeasureGui`.

**Risk Assessment: HIGH** — QWidget harness is fixed, but command-registration tests still likely fail/crash because the GUI application singleton is never created.

## Plan 02-02

**Summary**
Fixes the live-source and flyout-verification issues well: `Workbench::getToolbarItems()` is the correct value-copy source, and inspecting the produced `QToolButton` is the right flyout proof. The main blocker is command loading for the curated Evaluate tab.

**Strengths**
- Correctly avoids the transient `setupToolBars()` tree, uses `Workbench::getToolbarItems()` (copied value list).
- Correctly handles the `"Separator"` sentinel.
- Correctly requires actual widget inspection for flyouts: `MenuButtonPopup` + populated menu.
- Listed command IDs are largely real in this checkout.

**Concerns**
- **HIGH:** The curated map pins `Std_Measure` and `Std_MassProperties` on the Evaluate tab, but both are registered by the Measure GUI module (verified: `src/Mod/Measure/Gui/Command.cpp:48,94`), NOT by `PartDesignGui`/`SketcherGui`. Plan 01's bootstrap imports only `PartDesignGui, SketcherGui`, so the per-row resolution test (which Plan 02 mandates must assert EVERY curated row resolves non-null) will fail, and the runtime Evaluate tab would silently omit those buttons via D-08.
- **HIGH:** The runtime build path still depends on modules being loaded; because unresolved IDs are silently skipped (D-08), a missing module import degrades the ribbon silently rather than loudly.
- **MEDIUM:** `cmd->addTo(QToolBar*)` gives native `QToolButton`s, but two-line word-wrap / no-ellipsis is not guaranteed by the standard toolbar path.
- **MEDIUM:** Auto-derive/overflow should preserve command order/grouping deterministically; the plan implies but does not require a stable-ordering test.

**Suggestions**
- Add a curated preload list (`PartDesignGui`, `SketcherGui`, `PartGui`, `MeasureGui`, `MatGui` as needed) and make the per-row resolution test fail on ANY unresolved row including `Std_Measure`.
- Keep the flyout test exactly as revised (inspect the real `QToolButton`).

**Risk Assessment: HIGH** — the two cycle-1 concerns for this plan are mostly fixed, but Evaluate-tab command registration is not yet load-bearing.

## Plan 02-03

**Summary**
Genuinely corrects the bad mount seam: wrapping the ribbon in a real `QToolBar` mounted via `addToolBar(Qt::TopToolBarArea, ...)` matches Qt/FreeCAD persistence behavior. Remaining risks are test setup and chrome-hiding precision.

**Strengths**
- Correctly avoids `ToolBarManager::toolBarAreaWidget(QWidget*)` (it finds the area containing a widget; not a top-area lookup).
- Correctly uses a real `QToolBar` wrapper with stable `objectName` for `QMainWindow::saveState()`.
- Correctly persists the selected tab separately via `ParameterGrp`.
- Adds idempotent mount/unmount and a discoverability escape hatch.

**Concerns**
- **HIGH:** Task 1/2 test language says "construct or reuse a `QMainWindow`," but production `mountRibbon()` uses `Gui::getMainWindow()`. A generic `QMainWindow` does not set FreeCAD's `MainWindow` instance, so tests against it do not validate the production seam — they can pass while the real path is broken. Tests must construct/use `Gui::MainWindow`.
- **MEDIUM:** `hideStockChrome()` must explicitly exclude `Fw_RibbonToolBar`; a broad `ForceHidden` pass could hide the ribbon it just mounted.
- **MEDIUM:** The "More commands…" menu reuses existing `QAction`s, but `Command::getAction()` can be null until `initAction()`/`addTo()` has run; the action creation/ownership path must be specified.
- **MEDIUM:** If `unmountRibbon()` runs before app shutdown, the wrapper is absent when `QMainWindow::saveState()` is written — true-restart persistence needs a defined save point.

**Suggestions**
- Require Plan 03 tests to use a real `Gui::MainWindow` plus the fixed GUI bootstrap.
- Snapshot stock toolbar names before adding the ribbon, or explicitly filter out `Fw_RibbonToolBar`.
- Build overflow entries via `cmd->addTo(menu)` (or `initAction()` then safe `getAction()`).

**Risk Assessment: HIGH** — production mount concept is correct, but the current test plan can miss or fail the actual FreeCAD main-window path.

## Plan 02-04

**Summary**
The strongest revised plan. Addresses the restore-tab bug with an explicit `contextActive_` guard, uses the correct edit signals, avoids a Sketcher link dependency, and adds a dangling-pointer guard.

**Strengths**
- Explicitly prevents repeated `signalInEdit` from overwriting the original tab (the concern-7 fix).
- Uses `signalInEdit`/`signalResetEdit`, which exist on `Gui::Application`.
- Uses the real type-name contract `SketcherGui::ViewProviderSketch`.
- Holds the ribbon via `QPointer`.
- Tests the pure state machine in GTest without widgets.

**Concerns**
- **MEDIUM:** Signal-connected tests still depend on the corrected `Gui::Application` bootstrap (shared with Plan 01).
- **LOW:** Proving `previousIndex_` is unchanged after a nested enter needs a read-only test accessor or an action result carrying the index.
- **LOW:** Exact type-name matching is fine for v1, but derived/custom sketch view providers would not match.

**Suggestions**
- Store two scoped signal connections explicitly and disconnect both on teardown.
- Add a tiny read-only test accessor for `previousIndex_`/`contextActive_`.
- Keep the live manual validation of the sketch type-name literal.

**Risk Assessment: LOW-MEDIUM** — state machine and signal seam are sound; remaining risk is mostly shared bootstrap/lifecycle setup.

## Cycle-1 Resolution Table

| # | Cycle-1 HIGH | Status | Justification |
|---|---|---|---|
| 1 | Headless harness can't construct QWidgets | **RESOLVED** | Widget construction moved to a `QTEST_MAIN`/`setup_qt_test` target with `QT_QPA_PLATFORM=offscreen`; GTest kept widget-free. |
| 2 | Command IDs need module GUI init | **PARTIALLY RESOLVED** | Plans now import owning GUI modules, but the bootstrap never creates `Gui::Application::Instance`, which those imports require (`AppPartDesignGui.cpp:103-105`); `MeasureGui` also missing for Evaluate IDs. |
| 3 | Mount-seam API mismatch | **RESOLVED** | Plan 03 no longer uses `toolBarAreaWidget()` as a top-area lookup; mounts via `addToolBar(Qt::TopToolBarArea, wrapper)`; explicit `grep -c 'toolBarAreaWidget' == 0` gate. |
| 4 | Persistence model wrong | **RESOLVED** | Plan 03 wraps the ribbon in a real `QToolBar` for `QMainWindow` state and persists tab selection separately via `ParameterGrp`; round-trip tests assert restored AREA + tab index, not mere existence. |
| 5 | Auto-derive live source undefined | **RESOLVED** | Plan 02 uses public `Workbench::getToolbarItems()` value data, not the consumed/deleted `setupToolBars()` tree; `"Separator"` sentinel handled. |
| 6 | Spike gate vs implementation mismatch | **PARTIALLY RESOLVED** | The gate now splits A (code-demonstrated primitives) from B (reachable-only); but context-switch (item 5) is "reachable on native" and there is still no live enter-sketch/restore proof, so the original gate mismatch is reduced, not fully closed. |
| 7 | Restore-tab state-machine edge case | **RESOLVED** | Plan 04 adds `contextActive_`, no-ops nested sketch enters, no-ops stray resets, returns a `TabAction` enum, and adds the concern-7 regression test. |
| 8 | Flyout verification metadata-only | **RESOLVED** | Plan 02 requires inspecting the actual `QToolButton` from `addTo()` for `popupMode()==MenuButtonPopup` and `menu()->actions().size()>1`. |

**Resolution tally:** 6 fully RESOLVED, 2 PARTIALLY RESOLVED (#2, #6).

## Remaining / New HIGH Concerns (this cycle)

1. **HIGH — Incomplete GUI test bootstrap (Plan 02-01, extends concern #2).** The bootstrap as specified (`tests::initApplication()` App-only + a `QApplication`) does not create `Gui::Application::Instance`. Importing `PartDesignGui`/`SketcherGui` requires that singleton — without it the import raises `ImportError` and registers zero commands, so every command-ID resolution test silently asserts nothing. The bootstrap must construct/init a `Gui::Application` singleton before the module imports.
2. **HIGH — Evaluate-tab commands need MeasureGui (Plan 02-02).** `Std_Measure` and `Std_MassProperties` are registered by `src/Mod/Measure/Gui/Command.cpp`, not by PartDesignGui/SketcherGui. The bootstrap imports neither MeasureGui nor the other owning modules for the Evaluate row IDs, so the mandated per-row resolution test fails (or, at runtime, those buttons silently vanish via D-08).
3. **HIGH — Plan 03 mount tests use a generic `QMainWindow` (Plan 02-03).** Production `mountRibbon()` calls `Gui::getMainWindow()`; a test built against a plain `QMainWindow` does not exercise the FreeCAD `Gui::MainWindow` instance and can pass while the production seam is broken. Tests must use a real `Gui::MainWindow`.
4. **HIGH — Spike gate still reachability-only, not a live restore proof (Plan 02-01, concern #6 remainder).** The gate can PASS on "context switch reachable" without a live enter-sketch → Sketch-tab → exit → restore demonstration, so it does not fully de-risk the headline RIBBON-02 behavior at the gate. (Partial closure of cycle-1 concern #6.)

## Orchestrator Verification

The orchestrator independently re-verified the load-bearing NEW HIGH findings against source in this checkout:

- **GUI singleton guard (NEW HIGH 1):** CONFIRMED — `src/Mod/PartDesign/Gui/AppPartDesignGui.cpp:103-105` does `if (!Gui::Application::Instance) { PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application."); return nullptr; }`. The Plan 02-01 bootstrap never creates that singleton.
- **MeasureGui registration (NEW HIGH 2):** CONFIRMED — `Std_Measure` (`Command("Std_Measure")`, `Command.cpp:48`) and `Std_MassProperties` (`Command("Std_MassProperties")`, `Command.cpp:94`) live in `src/Mod/Measure/Gui/Command.cpp`. Neither is in the Plan 02-01 import list (`PartDesignGui, SketcherGui`).
- **Mount seam fix (concern #3):** CONFIRMED resolved — Plan 03 mounts via `addToolBar(Qt::TopToolBarArea, wrapper)` and adds a `grep -c 'toolBarAreaWidget' == 0` acceptance gate.
- **Persistence fix (concern #4):** CONFIRMED resolved — real `QToolBar` wrapper `Fw_RibbonToolBar` + separate `ParameterGrp` tab key; tests assert `toolBarArea(...)==Qt::TopToolBarArea` and `currentIndex()==2`.
- **State-machine fix (concern #7):** CONFIRMED resolved — `decideOnEnter`/`decideOnReset` with `contextActive_` guard and an explicit nested-enter regression test.

## Consensus Summary

Single external reviewer (Codex) this cycle, cross-checked against source by the orchestrator. Cycle 2 made strong progress: **6 of 8** cycle-1 HIGH concerns are fully resolved (mount seam, persistence model, auto-derive source, state machine, flyout verification, QWidget harness split). The revisions are real mechanism changes, not citations.

The phase is **not yet HIGH-clean**. Four HIGH concerns remain, three of them clustered on the same root cause: **the GUI test bootstrap is underspecified.** It does not create `Gui::Application::Instance` (so module imports fail), it omits `MeasureGui` (so the Evaluate tab does not resolve), and the Plan 03 mount tests use a generic `QMainWindow` instead of `Gui::MainWindow`. The fourth is the residual spike-gate looseness (reachability instead of a live restore proof). All four are concrete and fixable in a focused bootstrap/test-harness revision plus a tightened gate; none require an architecture change.

### Agreed Strengths
- The mount/persistence rewrite (real `QToolBar` + `addToolBar(Qt::TopToolBarArea)` + separate `ParameterGrp` tab state) is the correct Qt/FreeCAD seam.
- The auto-derive source switch to `Workbench::getToolbarItems()` is correct and live.
- The Plan 04 `contextActive_` state machine + `QPointer` guard cleanly closes the cycle-1 restore bug.
- The flyout test now inspects the real `QToolButton`/menu instead of metadata.

### Agreed Concerns (highest priority)
1. GUI bootstrap must create `Gui::Application::Instance` before importing GUI modules (else commands never register).
2. Evaluate-tab IDs (`Std_Measure`, `Std_MassProperties`) need `MeasureGui` in the bootstrap import list.
3. Plan 03 mount tests must use `Gui::MainWindow`, not a generic `QMainWindow`.
4. Spike gate should require a live sketch enter/restore proof, not just "reachable."

### Divergent Views
None — single reviewer this cycle.

---

## Action Routing

To incorporate this feedback:

```
/gsd-plan-phase 2 --reviews
```

Priority fixes before execution: (1) specify a complete GUI test bootstrap that creates `Gui::Application::Instance` and imports all owning GUI modules including `MeasureGui` (closes NEW HIGH 1 + 2 and cycle-1 concern #2); (2) require Plan 03 tests to use `Gui::MainWindow` (NEW HIGH 3); (3) tighten the Plan 01 spike gate to require a live sketch enter/restore demonstration (NEW HIGH 4 / cycle-1 concern #6 remainder).

---
phase: 2
reviewers: [codex]
reviewed_at: 2026-06-07T17:41:14Z
plans_reviewed: [02-01-PLAN.md, 02-02-PLAN.md, 02-03-PLAN.md, 02-04-PLAN.md]
---

# Cross-AI Plan Review — Phase 2

## Codex Review

## Overall Summary

The wave sequencing is directionally sound and the additive `src/Gui/FreeWorks` discipline is strong, but several plans treat unverified Qt/FreeCAD integration assumptions as settled. The biggest risks are in `02-03`: the planned top-area mount and persistence mechanism do not match the current APIs. `ToolBarManager::toolBarAreaWidget()` is not an enum lookup for `TopToolBarArea`, and `ToolBarAreaWidget::saveState()` only persists `QToolBar` children, not a plain `QTabWidget`. Also, the headless tests are likely weaker than claimed because QWidget construction requires a `QApplication`, and PartDesign/Sketcher command IDs will not resolve unless their GUI modules are explicitly loaded.

## Plan 02-01

**Summary**
Good as a thin spike/test scaffold, but the spike gate is not actually supported by the implementation tasks. It asks humans to verify mounted top-area tabs, context switching, and convincing visual parity before those capabilities exist. The automated test setup also appears incomplete for QWidget construction and module command registration.

**Strengths**
- Starts with a minimal vertical slice instead of committing to a full ribbon prematurely.
- Correctly uses `Gui::CommandManager`/`Command::addTo()` rather than duplicating command backends.
- Makes the native-vs-SARibbon decision explicit and blocks downstream plans on the verdict.
- Keeps work additive in `src/Gui/FreeWorks` and tests.

**Concerns**
- **HIGH:** `tests::initApplication()` only initializes App, not a `QApplication` or necessarily `Gui::Application::Instance`; constructing `QTabWidget` headless without a `QApplication` will fail. See [InitApplication.h](/Users/nguyenthuong/Repository/FreeCAD/tests/src/App/InitApplication.h:10).
- **HIGH:** `PartDesign_Pad` resolution will likely fail unless `PartDesignGui` is loaded; PartDesign GUI commands are registered from module GUI init, not by linking `FreeCADGui` alone. See [AppPartDesignGui.cpp](/Users/nguyenthuong/Repository/FreeCAD/src/Mod/PartDesign/Gui/AppPartDesignGui.cpp:123).
- **HIGH:** The D-03 human spike checklist includes top-area mounting, three tabs, context switching, and live sketch restore, but Tasks 1-2 only build a minimal unmounted `QTabWidget`. The gate may become subjective/manual theater rather than a real engineering decision.
- **MEDIUM:** `FwRibbon` lacks an export macro while tests instantiate it from `Gui_tests_run`; this is a possible Windows DLL visibility issue.
- **MEDIUM:** “No QApplication event loop” is fine, but “no QApplication” is not. The plan wording risks conflating those.

**Suggestions**
- Add a small test GUI bootstrap: create a `QApplication` object and initialize `Gui::Application` if absent, without calling `exec()`.
- Explicitly load `FreeCADGui`, `SketcherGui`, and `PartDesignGui` in the test setup, or move command-resolution tests to a target that already initializes module GUI commands.
- Split the spike gate into primitives that the code actually builds: native tab widget, large action buttons, one verified `ActionGroup` split button, and a throwaway mounted prototype.
- If the gate requires live top-area mounting/context switch, create a throwaway spike-only harness or defer those checklist items to the plans that implement them.

**Risk Assessment**
**HIGH.** The concept is good, but the first automated tests are likely to fail or give false confidence unless GUI/module initialization is fixed.

## Plan 02-02

**Summary**
The curated declarative map is the right architectural move, and using command IDs as data preserves App/Gui separation. The weak spots are auto-derive sourcing, flyout verification, and UI fidelity assumptions about native `QToolBar`/`QToolButton` behavior.

**Strengths**
- `FwRibbonMap` is additive, merge-friendly, and keeps command curation separate from rendering.
- D-08 omit-missing is a good safety policy.
- Using `Command::addTo(panel)` correctly preserves existing QAction shortcuts, icons, enabled-state, and backend behavior.
- Exposing `setCurrentTab()` early helps Plan 02-04.

**Concerns**
- **HIGH:** Auto-derive needs a real source for the workbench `ToolBarItem` tree. By activation time, `setupToolBars()` has been consumed/deleted by `Workbench::activate()`. Plan 02-03 says `buildAutoDerived(...)` when no curated map exists, but does not define how to obtain the tree.
- **HIGH:** The flyout test proposal is muddled. `CommandManager::getGroupCommands()` groups by command group metadata, not necessarily by a group command ID. The robust test is: resolve command, call `addTo(QToolBar)`, find the resulting `QToolButton`, assert `MenuButtonPopup` and menu action count.
- **MEDIUM:** `ToolBarItem` separators are commonly the literal `"Separator"`, not `command()==""`; the auto-derive logic may miss separators. See [Workbench.cpp](/Users/nguyenthuong/Repository/FreeCAD/src/Gui/Workbench.cpp:293).
- **MEDIUM:** “2-line wrapped labels, never ellipsis” is not guaranteed by plain `QToolBar` plus `Qt::ToolButtonTextUnderIcon`; native `QToolButton` text wrapping needs explicit validation or a custom tool button.
- **MEDIUM:** The `kKnownGaps` allow-list can undermine the “verified pinned IDs” contract if it silently accepts typos.

**Suggestions**
- Define auto-derive from either active `QToolBar` widgets after `ToolBarManager::setup()` or from `Workbench::getToolbarItems()`, not from a transient protected tree unless you add a lifecycle seam.
- Make flyout tests inspect actual widget/action output, not only registry metadata.
- Treat `"Separator"` as the separator sentinel.
- Keep `kKnownGaps` empty initially; represent real product gaps as comments without fake command IDs.
- Add a screenshot/manual validation item for label wrapping and button size, since headless tests cannot prove visual fidelity.

**Risk Assessment**
**MEDIUM-HIGH.** The map architecture is solid, but auto-derive and flyout verification need correction to satisfy RIBBON-01 reliably.

## Plan 02-03

**Summary**
This is the riskiest plan. The requirements are correct, but the proposed mount and persistence seams do not match the current FreeCAD/Qt implementation. A plain `FwRibbon : QTabWidget` added to a toolbar-area layout will not participate in `QMainWindow::saveState()`, and the planned public API for fetching `TopToolBarArea` does not exist.

**Strengths**
- Correctly targets reversible chrome hiding as FreeWorks-scoped behavior.
- Avoids editing `MainWindow.cpp`, which is important for upstream sync.
- Includes a D-12 discoverability path instead of simply hiding all legacy command surfaces.
- Recognizes unique object names as necessary for persistence.

**Concerns**
- **HIGH:** `ToolBarManager::toolBarAreaWidget(QWidget*)` does not fetch by `ToolBarArea::TopToolBarArea`; it only looks up area widgets containing an existing widget, and currently searches status/menu corner area widgets. See [ToolBarManager.h](/Users/nguyenthuong/Repository/FreeCAD/src/Gui/ToolBarManager.h:181) and [ToolBarManager.cpp](/Users/nguyenthuong/Repository/FreeCAD/src/Gui/ToolBarManager.cpp:581).
- **HIGH:** `QMainWindow::saveState()` will not persist a plain child `QTabWidget`; FreeCAD’s `ToolBarAreaWidget::saveState()` also only iterates `QToolBar` children. See [ToolBarAreaWidget.cpp](/Users/nguyenthuong/Repository/FreeCAD/src/Gui/ToolBarAreaWidget.cpp:121).
- **HIGH:** `hideStockChrome()` needs a concrete, safe list of toolbar names. `ToolBarManager::toolBars()` is protected, and a blanket `findChildren<QToolBar*>()` must exclude the ribbon wrapper.
- **MEDIUM:** `menuBar()->show()` on restore may clobber prior user/fullscreen/platform state, especially on macOS native menu bars.
- **MEDIUM:** The overflow “right edge” placement is under-specified for a `QTabWidget`; use `QTabWidget::setCornerWidget()` or a wrapper layout.
- **MEDIUM:** The proposed save/restore headless test can trivially pass if the widget is pre-created; it must assert restored area/order/state, not merely object existence.

**Suggestions**
- Wrap the ribbon in a real `Gui::ToolBar`/`QToolBar` with stable objectName, add the `QTabWidget` via `addWidget()`, and mount it with `Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea, ribbonToolBar)`. That gives Qt a real state participant.
- Persist custom tab/panel state separately if “tab layout” means selected tab, panel order, or collapsed state; `QMainWindow::saveState()` only handles docks/toolbars.
- Snapshot menu bar visibility/native-menu setting and restore exactly that snapshot.
- Make mount/unmount idempotent: no duplicate ribbon on reactivation, and teardown removes or hides the wrapper predictably.
- Add explicit macOS manual validation for menu bar hiding/restoring.

**Risk Assessment**
**HIGH.** As written, this plan can fail SC1 and SC5 even if all tests pass, because the integration seam and persistence model are wrong.

## Plan 02-04

**Summary**
The event-driven edit-signal approach is the right architecture and avoids Sketcher linkage. The switch/restore state machine needs tightening, and the “pure function returns Sketch index” API is not coherent unless it has access to tab mapping.

**Strengths**
- Uses `signalInEdit`/`signalResetEdit`, which is better than polling `Control::activeDialog()`.
- Avoids compile/link dependency on Sketcher by using the view-provider type-name string.
- Plans scoped signal connections and teardown, which is necessary for workbench switching.
- Tests context wins and restore behavior, not just construction.

**Concerns**
- **HIGH:** Repeated sketch-enter signals can overwrite `previousTab_` with the Sketch tab, causing reset to restore to Sketch instead of the original tab unless the context tracks “already context-switched.”
- **MEDIUM:** `resolveTabForEnter(vpTypeName, currentIndex)` cannot honestly return the Sketch tab index without either hardcoding tab order or querying the ribbon. Hardcoding index `1` is brittle.
- **MEDIUM:** A raw non-owning `FwRibbon*` is unsafe if signals fire after teardown; use `QPointer<FwRibbon>` or guarantee disconnect-before-destroy.
- **MEDIUM:** Reset should be a no-op unless the active context switch was caused by a sketch enter; otherwise any reset signal can force an old stale tab.
- **LOW:** Exact string `"SketcherGui::ViewProviderSketch"` is acceptable for v1, but should be live-validated because type-name contracts can move across upstream changes.

**Suggestions**
- Model the state machine explicitly: `bool contextActive_`, `int previousIndex_`, ignore nested sketch enters while active, clear state on reset.
- Make the pure function return an action enum (`SwitchToSketch`, `NoOp`, `RestorePrevious`) and let the ribbon-bound layer resolve tab names to indices.
- Use `QPointer<FwRibbon>` and guard all signal callbacks.
- Add a live/manual test that enters and exits a real sketch, because pure tests cannot prove the type-name literal is correct in the running app.

**Risk Assessment**
**MEDIUM.** The architecture is right, but the restore logic has enough edge cases to break RIBBON-02 unless the state machine is made explicit.

---

## Consensus Summary

Only one external reviewer (Codex) was invoked for this cycle, so "consensus" reflects a single independent review cross-checked against the codebase by the orchestrator. Where claims were verifiable, the orchestrator confirmed them against source.

### Agreed Strengths

- Wave sequencing and the native-first → spike-gate → full-build progression are architecturally sound.
- Strict additive `src/Gui/FreeWorks/` discipline with no `MainWindow.cpp` edits preserves upstream-merge safety.
- Driving the ribbon from the existing command registry via `Command::addTo()` correctly avoids a duplicated command backend (RIBBON-01 SC2).
- The curated declarative `FwRibbonMap` cleanly separates curation data from rendering and keeps App/Gui separation intact.
- The edit-signal (`signalInEdit`/`signalResetEdit`) approach for context switching is the correct event-driven realization of D-09 (vs. polling `Control::activeDialog()`).

### Agreed Concerns (highest priority)

The following HIGH concerns were verified against the codebase and are load-bearing for the phase:

1. **Headless test harness cannot construct QWidgets (Plan 02-01).** `tests::initApplication()` initializes only `App::Application` (confirmed: `tests/src/App/InitApplication.h`). The existing `FwWorkbench.cpp` test only constructs a QObject `Workbench`, never a QWidget. Constructing `FwRibbon : QTabWidget` requires a live `QApplication`. As written, every Plan 02-01/02/03 automated test that builds the ribbon will not compile/run, or will give false confidence.
2. **Command IDs won't resolve without module GUI init (Plan 02-01/02-02).** `PartDesign_*` / `Sketcher_*` commands are registered by their GUI modules, not by linking `FreeCADGui`. The per-row resolution test is the entire typo guard for the pinned IDs — if modules aren't loaded, it asserts nothing useful.
3. **Mount seam API mismatch (Plan 02-03).** `ToolBarManager::toolBarAreaWidget(QWidget*)` looks up the area containing an existing widget — it is NOT an enum lookup for `TopToolBarArea` (confirmed: `ToolBarManager.h:181`). The planned `key_links` mount call does not exist as described.
4. **Persistence model is wrong (Plan 02-03).** A plain `QTabWidget` child does not participate in `QMainWindow::saveState()`; FreeCAD's `ToolBarAreaWidget::saveState()` iterates only `QToolBar` children (confirmed: `ToolBarAreaWidget.cpp:121`). D-14/SC5 (layout persists across restart) can fail even with all headless tests green. Fix: wrap the ribbon in a real `QToolBar` mounted via `addToolBar(Qt::TopToolBarArea, ...)`, and/or persist tab/panel state separately.
5. **Auto-derive source undefined (Plan 02-02/02-03).** The workbench `ToolBarItem` tree from `setupToolBars()` is consumed/deleted by `Workbench::activate()`; the plans call `buildAutoDerived(...)` without defining a valid, live source for the tree.
6. **Spike gate vs. implementation mismatch (Plan 02-01).** The D-03 human checklist requires top-area mounting, 3 tabs, context switching, and live sketch restore — but Tasks 1–2 only build a minimal unmounted `QTabWidget`. The gate risks becoming subjective theater unless it is scoped to primitives the spike code actually builds (or a throwaway mounted harness is specified).
7. **Restore-tab state machine edge case (Plan 02-04).** Repeated `signalInEdit` while already in a sketch can overwrite `previousTab_` with the Sketch index, so reset restores to Sketch instead of the original tab. Needs an explicit `contextActive_` guard.
8. **Flyout verification is metadata-only (Plan 02-02).** Asserting `getGroupCommands()` count does not prove a working `MenuButtonPopup` split-button. Test must inspect the actual `QToolButton`/menu produced by `addTo()`.

### Divergent Views

None — single reviewer this cycle. No reviewer-vs-reviewer disagreements to adjudicate. The LOW item (type-name literal stability, Plan 02-04) is noted but not blocking.

---

## Action Routing

To incorporate this feedback into the plans:

```
/gsd-plan-phase 2 --reviews
```

Priority fixes before execution: resolve the test-harness GUI bootstrap + module-load gap (concerns 1–2), correct the mount/persistence seam in Plan 02-03 (concerns 3–4), define the auto-derive source (concern 5), and tighten the context state machine in Plan 02-04 (concern 7).

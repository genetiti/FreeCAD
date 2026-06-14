# Phase 4: PropertyManager Panel - Pattern Map

**Mapped:** 2026-06-14
**Files analyzed:** 11 (5 new source pairs/edits + 3 test files + 3 reuse/extend)
**Analogs found:** 11 / 11 (every new file has an in-repo `Fw*` or upstream-seam analog)

This phase is **placement + chrome + glue**, not mechanism. Every new file copies the discipline of an existing `Fw*` component (idempotent find-or-reuse mount, scoped-signal consumer, palette-only paint, RAII selection guard, GTest-logic + QTEST-widget split). Planner: each new file's plan action should reference the analog file:line excerpt below for "copy this" and the "Change" note for "do this differently."

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/Gui/FreeWorks/FwLayout.{h,cpp}` — ADD `mountPropertyManager()`/`unmountPropertyManager()` | UI container / dock mount | event-driven (activate/deactivate) | `FwLayout::mountFeatureManager` (FwLayout.cpp:117-146) | exact (same file, same mount discipline) |
| `src/Gui/FreeWorks/FwPropertyManagerHeader.{h,cpp}` — NEW | UI chrome (✓/✗ header band) | request-response (button → Control slot) | `FwRollbackBar.{h,cpp}` + Pattern-2 (Control accept/reject) | role-match (thin Fw widget driving an existing singleton) |
| `src/Gui/FreeWorks/FwReferenceBoxStyler.{h,cpp}` — NEW | styler (pink active-box state) | event-driven (selection/focus → restyle) | `FwFeatureTreeDelegate::paint` (FwFeatureTreeDelegate.cpp:146-164) palette-only | role-match (palette-role paint, no hex) |
| `src/Gui/FreeWorks/FwPropertyKeyFilter.{h,cpp}` — NEW (or container `keyPressEvent`) | event-filter (Tab/Enter/Esc) | request-response (key → accept/reject/focus) | `TaskView::keyPressEvent` (upstream, learn-from) + `FwRibbonContext` scoped-lifetime discipline | partial (new behavior; lifetime pattern from Fw, key-map from upstream) |
| `FLOW-01 sketch-exit handler` — EXTEND `FwRibbonContext.{h,cpp}` OR new `FwLayout`-owned consumer | view-provider glue | event-driven (`signalResetEdit` → select + tab) | `FwRibbonContext::connect` resetEdit lambda (FwRibbonContext.cpp:111-122) | exact (same signal, same type-name-string contract) |
| `src/Gui/FreeWorks/FwSelectionGuard.{h,cpp}` | RAII selection helper | transform (snapshot/restore) | **REUSE as-is** (Phase 3) | reuse |
| `tests/src/Gui/FwPropertyManager.cpp` — NEW GTest | test (logic) | — | `tests/src/Gui/FwRibbon.cpp` (TEST_F + bootstrap) | exact |
| `tests/src/Gui/FwPropertyManagerWidget.cpp` — NEW QTEST | test (widget, offscreen) | — | `tests/src/Gui/FwFeatureTreeWidget.cpp` (QObject + private slots) | exact |
| `tests/src/Gui/FwReferenceBoxStyler.cpp` — NEW GTest | test (state machine) | — | `tests/src/Gui/FwRibbon.cpp` | exact |
| `tests/src/Gui/CMakeLists.txt` — EDIT | build wiring | — | CMakeLists.txt:7-25 (`Gui_tests_run` + `setup_qt_test`) | exact |
| `src/Gui/FreeWorks/CMakeLists.txt` — EDIT (additive `target_sources`) | build wiring | — | existing `FreeWorks_*_SRCS` additive pattern | exact |

---

## Shared Patterns

### Idempotent find-or-reuse mount (move CONTAINER, preserve objectName)
**Source:** `src/Gui/FreeWorks/FwLayout.cpp:117-146` (`mountFeatureManager`)
**Apply to:** `FwLayout::mountPropertyManager()`
The keystone discipline: reach the live registry only through `Gui::DockWindowManager` (never edit `MainWindow.cpp`/`Control.cpp`), be idempotent on re-activation, preserve the saved-layout objectName.
```cpp
// FwLayout.cpp:122-126 — idempotent guard
QWidget* existing = manager->findRegisteredDockWindow(kFeatureManagerDock);
if (qobject_cast<FreeWorksGui::FwFeatureTree*>(existing) != nullptr) {
    return;  // never add a second
}
```
**Change for Phase 4 (RESEARCH Pattern 1, Pitfall 4):** Do NOT register a new widget under `Fw_PropertyManager` and do NOT reparent the `TaskView`. Instead move the EXISTING `"Std_TaskView"` dock CONTAINER left (preserves the `"Tasks"` widget objectName that `Control::taskPanel()` resolves by — Control.cpp:58):
```cpp
QDockWidget* taskDock = Gui::DockWindowManager::instance()->getDockContainer("Std_TaskView");
if (taskDock && mw->dockWidgetArea(taskDock) != Qt::LeftDockWidgetArea) {
    mw->addDockWidget(Qt::LeftDockWidgetArea, taskDock);  // re-docks, keeps identity
    taskDock->show();
}
```
**SPIKE-GATED (A1 / Open Question 1):** Confirm at runtime whether `getDockWindow("Tasks")` resolves the container or the widget objectName BEFORE committing this move. If it would break, fall back to the D-03 thin-adopt path. Verdict → `SPIKE.md`.

### Mount/teardown wiring into the workbench lifecycle
**Source:** `src/Gui/FreeWorks/FwWorkbench.cpp:44-74` (`activated`/`deactivated`) + `FwWorkbench.cpp:105` (`setupDockWindows` calls `FwLayout::install()`)
**Apply to:** add `FwLayout::mountPropertyManager()` after `mountRibbon()` in `activated()`; add `unmountPropertyManager()` (move the Tasks dock back to `Qt::RightDockWidgetArea`) before `unmountRibbon()` in `deactivated()` — mirror `restoreStockChrome` reversibility so stock workbenches keep the right-docked Tasks panel.
```cpp
// FwWorkbench.cpp:61-63 — order: Std first, then mount
StdWorkbench::activated();
FwLayout::mountRibbon();
FwLayout::hideStockChrome();   // ADD: FwLayout::mountPropertyManager();
```
**SPIKE-GATED (A2 / Pitfall 3):** PartDesign `setEdit` calls `assureWorkbench("PartDesignWorkbench")` (ViewProvider.cpp:165) mid-edit — verify the left re-host survives the WB round-trip; the idempotent mount must re-assert cleanly. Verdict → `SPIKE.md`.

### Event-driven signal consumer (never poll `Control().activeDialog()`)
**Source:** `src/Gui/FreeWorks/FwRibbonContext.cpp:80-129` (`connect`/`disconnect`)
**Apply to:** PROP-01 panel reveal (`signalInEdit`) and FLOW-01 handoff (`signalResetEdit`).
- Hold `boost::signals2::scoped_connection` members; reassign in `connect()` (idempotent — never stacks subscriptions); `disconnect()` explicitly in the destructor (FwRibbonContext.cpp:65-71).
- Own exactly ONE consumer for the FreeWorks-mode lifetime via a `unique_ptr` static (FwLayout.cpp:59, `s_ribbonContext`); `reset()` in `unmount*` so connections drop before teardown (FwLayout.cpp:286).
- Guard widget access with `QPointer` (FwRibbonContext.cpp:76-77).
**Pitfall 1:** `Control().activeDialog()` has NO change signal — use `signalInEdit`/`signalResetEdit`, exactly as this consumer already does.

### Palette-only color (no hex, no stylesheet color literal)
**Source:** `src/Gui/FreeWorks/FwFeatureTreeDelegate.cpp:154-162`
**Apply to:** `FwReferenceBoxStyler` (pink active tone) and `FwPropertyManagerHeader` (✓/✗ tint).
```cpp
// FwFeatureTreeDelegate.cpp:157-159 — derive a state color from a palette ROLE
QStyleOptionViewItem greyed(option);
const QColor disabledText = greyed.palette.color(QPalette::Disabled, QPalette::Text);
greyed.palette.setColor(QPalette::Text, disabledText);
```
**Change (UI-SPEC § Color, D-06):** active box = palette-DERIVED pink (functional state, in scope now; exact hue → Phase 7); filled rows use `QPalette::Highlight`/`HighlightedText`; inactive caption uses `QPalette::Disabled, QPalette::Text`. **Never blue** (reserved for prompt icons). Test asserts no `setStyleSheet` color literal; `tools/fw-string-leak-grep.sh` must stay clean.

### RAII selection snapshot/restore
**Source:** `src/Gui/FreeWorks/FwSelectionGuard.h:69-89` — **reuse verbatim**, do not rebuild.
Snapshots selection AND preselection by owned value (handles the `clearPreSelect=true` clobber pitfall, FwSelectionGuard.h:50-56). Use around FLOW-01's `addSelection(finished sketch)` (D-11) and any FreeWorks-driven selection mutation.

---

## Pattern Assignments

### `FwPropertyManagerHeader.{h,cpp}` (UI chrome, request-response)

**Analog:** `FwRollbackBar.{h,cpp}` (thin Fw widget) + RESEARCH Pattern 2.
**Core pattern — map ✓/✗ to the existing Control singleton (no new commit logic):**
```cpp
// Source: src/Gui/Control.h:98-100 (accept/reject slots), Control.cpp:212/227
connect(greenCheck, &QAbstractButton::clicked, [] { Gui::Control().accept(); });  // OK / Enter
connect(redCross,   &QAbstractButton::clicked, [] { Gui::Control().reject(); });  // Cancel / Esc
```
**Copy:** tooltips "Accept (Enter)" / "Cancel (Esc)"; copy from UI-SPEC § Copywriting (OK / Cancel; no "SolidWorks" string). 32px header band (UI-SPEC § Spacing). **Change vs analog:** drives `Gui::Control()`, not a feature command; palette-only tint per Shared Pattern.

### `FwReferenceBoxStyler.{h,cpp}` (styler, event-driven)

**Analog:** `FwFeatureTreeDelegate::paint` (palette-only) + `FwRibbonContext` (scoped-signal lifetime).
**Core:** observe which reference box is active (focus + `Gui::Selection`/selection-gate state) and apply the derived-pink active tone to exactly ONE box; revert on deactivate.
**Pitfall 6 (SPIKE-GATED A3):** the "which box is active" hook is an Open Question — if no clean accessor exists on the hosted panel, infer from focus + selection-gate; a `// SW-FORK HOOK` accessor is the last resort. Keep it a reversible visual restyle only. The box machinery (fill/highlight/expand) is OWNED by the hosted panel (`PartDesignGui::ReferenceSelection : Gui::SelectionFilterGate`, ReferenceSelection.h:42) — do NOT rebuild it (D-05; Phase 5 owns selection).

### `FwPropertyKeyFilter.{h,cpp}` (event-filter)

**Analog:** upstream `TaskView::keyPressEvent` (learn-from; Enter→default, Esc→escape button already mapped, incl. a macOS crash workaround) + `FwRibbonContext` for scoped install/teardown.
**Core:** install a `QObject` event filter on the hosted container; ensure Enter→`accept()`, Esc→`reject()`, Tab→advance to next reference box.
**Pitfall 5:** scope the Tab override to inter-box / top-level fields only; let Qt's `focusNextChild` handle intra-field traversal. Do NOT edit `TaskView::keyPressEvent` (D-02/D-10). Container-level only.

### FLOW-01 sketch-exit handler (view-provider glue) — extend `FwRibbonContext` or a sibling consumer

**Analog:** `FwRibbonContext.cpp:111-122` (the existing `signalResetEdit` lambda).
```cpp
// Source: FwRibbonContext.cpp:111 + RESEARCH Pattern 3 — identify sketch by STRING (Pitfall 2)
app->signalResetEdit.connect([](const Gui::ViewProviderDocumentObject& vp) {
    if (vp.getTypeId().getName() == std::string("SketcherGui::ViewProviderSketch")) {
        App::DocumentObject* sketch = vp.getObject();
        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(sketch->getDocument()->getName(),
                                      sketch->getNameInDocument());
        // ribbon → Features tab via FwRibbonContext's existing restore/decide path
    }
});
```
**Copy:** reuse the `kSketchViewProviderTypeName` constant already defined (FwRibbonContext.cpp:54) — do NOT add a Sketcher include (Pitfall 2). **Change vs analog:** add the `Gui::Selection().addSelection` step; do NOT auto-launch any feature command (D-11). Wrap the selection mutation with `FwSelectionGuard` if it must not clobber existing preselect.
**LIVE RE-VALIDATION (Pitfall 2 / Open Question 3):** the type-name literal is an upstream contract — record in `SPIKE_LIVE_CHECKLIST.md`.

---

## Test Pattern Assignments

### `tests/src/Gui/FwPropertyManager.cpp` + `FwReferenceBoxStyler.cpp` (GTest logic)
**Analog:** `tests/src/Gui/FwRibbon.cpp:64-108` — `TEST_F` fixtures, `tests::ensureGuiTestBootstrap()` before any Control/Selection assertion (FwTestGuiBootstrap.h brings up App + offscreen QApplication + `Gui::Application(false)` + PartDesignGui/SketcherGui imports). Assert: header→accept/reject mapping, FLOW-01 decision (select + no auto-launch), non-modal (no `exec()`), pink state machine (active/inactive/filled), palette-role (not hex).

### `tests/src/Gui/FwPropertyManagerWidget.cpp` (QTEST offscreen)
**Analog:** `tests/src/Gui/FwFeatureTreeWidget.cpp:170-224` — `class testFwPropertyManagerWidget : public QObject` with `private slots` (`initTestCase` calls `ensureGuiTestBootstrap`, `init`/`cleanup`, `test_*`). Use a `Probe*` subclass to expose protected seams if needed (FwFeatureTreeWidget.cpp:108-156). Assert: after `mountPropertyManager()` the Tasks dock is `Qt::LeftDockWidgetArea` and `Control().taskPanel() != nullptr`; rollouts (`TaskBox`/`TaskGroup`) still present + expandable; accept/reject fire via a stub `TaskDialog`; `saveState`/`restoreState` round-trips with objectNames preserved; Tab traversal.

### CMake wiring
**Analog:** `tests/src/Gui/CMakeLists.txt:7-25` — add `FwPropertyManager.cpp`, `FwReferenceBoxStyler.cpp` to the `Gui_tests_run add_executable` list; add `setup_qt_test(FwPropertyManagerWidget)`. FreeWorks sources: append to `FreeWorks_CPP_SRCS`/`FreeWorks_HPP_SRCS` (additive `target_sources(FreeCADGui PRIVATE …)`), per FreeWorks/CMakeLists.txt convention.

---

## No Analog Found

None. Every new file maps to an in-repo `Fw*` component (for fork convention) or a named upstream seam (for the engine being hosted). The only genuinely-new *behavior* is the active-box-pink state machine and the container key filter — both have a structural analog (palette-only paint; scoped-signal lifetime) even though the behavior is new.

---

## Metadata

**Analog search scope:** `src/Gui/FreeWorks/`, `src/Gui/Control.{h,cpp}`, `src/Gui/TaskView/`, `src/Gui/MainWindow.cpp`, `src/Gui/ViewProvider.h`, `src/Mod/PartDesign/Gui/ViewProvider.cpp`, `src/Mod/PartDesign/Gui/ReferenceSelection.h`, `tests/src/Gui/`.
**Files scanned:** ~18 (read targeted ranges; no re-reads).
**Spike-gated assumptions feeding planning:** A1 (dock-name resolution after left move), A2 (WB-switch teardown), A3 (active-box hook) — all resolve in the D-03 `SPIKE.md` before the full build.
**Pattern extraction date:** 2026-06-14

# Phase 4: PropertyManager Panel - Research

**Researched:** 2026-06-14
**Domain:** FreeCAD Qt 6.8 Gui layer — hosting/re-docking the existing `Gui::Control` / `Gui::TaskView::TaskView` task-panel machinery as a SolidWorks-style left PropertyManager (C++20 fork, additive `FreeWorksGui`)
**Confidence:** HIGH (every load-bearing claim grounded in read source in this repo)

## Summary

Phase 4 is **not a build-a-new-panel-system phase** — it is a **re-host + re-style + key/flow-glue** phase. The entire FreeCAD task-panel stack already provides everything PROP-01/PROP-02/TREE-03/FLOW-01 need: collapsible rollout groups (`TaskGroup`/`TaskBox` = QSint `ActionBox`/`ActionGroup`), accept/reject semantics (`TaskView::accept()`/`reject()` wired to a `QDialogButtonBox` in `TaskEditControl`), Enter→accept / Escape→reject key handling (`TaskView::keyPressEvent`, TaskView.cpp:393), transaction-wrapped edits (each `TaskDialog` owns its `openTransaction`/`commitTransaction`), live preview (PartDesign `PreviewExtension` + `PreviewUpdateScheduler`, ViewProvider.cpp:213-230), and selection-reference boxes (`SelectionFilterGate` + `SelectionObserver` owned *inside* each hosted panel — e.g. `PartDesignGui::ReferenceSelection`). **All of this runs unmodified the moment the "Tasks" `TaskView` dock is positioned in the left dock area** — which satisfies SC4 ("existing PartDesign panels work unmodified") and TREE-03 ("double-click → setEdit → Control().showDialog → lands left") *for free* from the hosting decision (D-01/D-09).

The single keystone discovery: `Control::taskPanel()` (Control.cpp:55) resolves the host by looking up the **dock window whose widget objectName is `"Tasks"`** via `DockWindowManager::getDockWindow("Tasks")`. That widget is a real `Gui::TaskView::TaskView` created in `MainWindow::setupTaskView()` (MainWindow.cpp:609-633), wrapped in a `QDockWidget` registered as `"Std_TaskView"`, and **docked right by default**. To make it the left PropertyManager, FreeWorks must move that *existing* dock container to `Qt::LeftDockWidgetArea` — **without reparenting the `TaskView` widget out of its dock and without changing its `"Tasks"` objectName** (three call sites depend on the name/identity: Control.cpp, ViewProvider.cpp:250, OverlayWidgets.cpp:2850). This is achievable purely through `Gui::getMainWindow()->addDockWidget()` / `DockWindowManager`, with **zero edits to `Control.cpp` or `TaskView.cpp` bodies** (D-02). The D-03 spike confirms exactly this.

The genuinely *new* FreeWorks work is narrow and testable: (1) the left re-host of the "Tasks" dock at FreeWorks mount/activate time (mirror `mountFeatureManager`); (2) a SolidWorks-styled green-✓/red-✗ header affordance that drives `Control().accept()`/`reject()`; (3) a palette-derived "pink active" visual treatment for the active reference box (the *box machinery* is already in the hosted panel — FreeWorks only colors the active-state); (4) Tab traversal + ensuring Enter/Esc map to accept/reject at the container level; (5) FLOW-01 sketch-exit auto-select + Features-tab handoff via the existing `signalResetEdit` plumbing already consumed by `FwRibbonContext`.

**Primary recommendation:** Move the existing `"Std_TaskView"` QDockWidget (widget objectName `"Tasks"`) into `Qt::LeftDockWidgetArea` at FreeWorks activation using the find-or-reuse discipline of `FwLayout::mountFeatureManager`, add a thin FreeWorks header/keys/pink-state layer on top, and reuse `signalInEdit`/`signalResetEdit` (the same hooks `FwRibbonContext` already uses) for reveal + FLOW-01 — touching no shared `Control`/`TaskView` body. Run the D-03 spike first to confirm the left-move preserves rollouts + accept/reject + the `getDockWindow("Tasks")` lookups.

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01: Reuse-first — re-host FreeCAD's existing `Gui::Control` / `TaskView` into the left `Fw_PropertyManager` dock and restyle it SW-like.** Do NOT rebuild a task-panel system. `ControlSingleton::showDialog(TaskDialog*)` drops the active task dialog into `Control::taskPanel()` (a `Gui::TaskView::TaskView`) — normally in the right "Tasks" dock. FreeWorks hosts/redirects that `TaskView` in the left dock. Because every PartDesign/Sketcher feature dialog shows through `Gui::Control`, hosting it left makes existing task panels work unmodified (SC4) and makes TREE-03's edit panels land left for free (D-09).
- **D-02: Achieve the SW presentation via FreeWorks-side hosting/configuration, NOT by editing `src/Gui/Control.cpp` or `src/Gui/TaskView/TaskView.cpp` bodies.** Prefer (1) host/observe the existing `Control::taskPanel()` `TaskView` widget under the left dock (re-parent / observe-the-DOM, mirroring the Phase 3 `mountFeatureManager` find-or-reuse swap), or (2) a thin `FwPropertyManager` container that adopts the running `TaskView`/`TaskDialog`. Any unavoidable shared-file touch gets `// SW-FORK HOOK`.
- **D-03 (PLAN-TIME SPIKE):** Before the full build, confirm `Gui::Control`'s `TaskView` can be (a) re-hosted/shown in the LEFT `Fw_PropertyManager` dock instead of the default right Tasks dock, (b) with accept/reject controls and collapsible `TaskBox`/`TaskGroup` rollouts intact, (c) without editing `Control.cpp`/`TaskView.cpp` bodies. PASS → reuse-and-rehost is the committed engine. FAIL → a thin `FwPropertyManager` container that adopts the running `TaskView`/`TaskDialog` widgets (still additive, NOT a from-scratch task system — explicit last resort).
- **D-04: Reuse `TaskView`'s existing structure for header + rollout groups; do not build a custom one.** Map green-✓ → `accept()` and red-✗ → `reject()`, positioned in the panel header. Restyle SW-like via palette only (no hex, no `setStyleSheet` color literals — Phase 7 owns final art). ✓/✗ semantics + header placement in scope this phase; glyph art / exact hue is Phase 7.
- **D-05: Build a thin FreeWorks selection-reference affordance that drives/reads the EXISTING `Gui::Selection`, NOT a parallel selection backend.** SW behavior: click into a reference box to activate it, pick geometry → box fills + picked element highlights, box auto-expands, clear/remove work. Wired to `Gui::Selection` (SelectionObserver) and the existing task-panel reference-selection mechanism — never a new selection model (Phase 5 owns selection behavior; this phase only consumes it).
- **D-06: "Pink-when-active" is a FUNCTIONAL state signal (in scope now), distinct from Phase 7 decorative theming.** Pink = active reference box; blue is reserved for prompt icons. A palette-derived active tone is allowed this phase; exact final hue/art is Phase 7.
- **D-07: Non-modal / semi-modal — geometry stays pickable while the panel is open and the model previews live.** LOCKED by REQUIREMENTS (a fully-modal PropertyManager is explicitly rejected). The panel is event-driven on command/edit start, reusing the same `Gui::Control` / `signalInEdit` plumbing the Phase-2 `FwRibbonContext` already listens to.
- **D-08: The "slide-in" is a light affordance, not a heavyweight animation (MVP framing).** Must-have: panel appears left-docked when a command/edit starts; the literal slide is a refinement. Implement a short width/visibility animation (`QPropertyAnimation`) ONLY if low-risk and cross-platform-clean; otherwise instant reveal is acceptable. Animation polish must never block the functional panel.
- **D-09: Route the EXISTING `ViewProvider::setEdit()` edit path so its task dialog lands in the LEFT `Fw_PropertyManager` dock — no separate mechanism.** Double-clicking a feature in `FwFeatureTree` triggers the standard edit entry (inherited `TreeWidget` double-click → `ViewProviderDocumentObject::doubleClicked` → `setEdit`). Because that dialog shows via `Gui::Control` (D-01), left-hosting automatically lands feature-edit panels in the PropertyManager. Only work: ensure `FwFeatureTree` double-click reaches `setEdit` (reuse inherited behavior) and that `Control`'s panel is the left-hosted one. (TREE-03's F2-rename half shipped in 03-02.)
- **D-10: Apply SW key semantics at the container level — Enter = accept (✓), Escape = cancel (✗), Tab = advance to next field / reference box — overriding FreeCAD task-panel defaults where they differ.** Implement as a FreeWorks key handler on the hosted container; do not edit the shared TaskView. Every edit stays transaction-wrapped by reusing the existing `TaskDialog` `openTransaction`/`commitTransaction` discipline — clean undo/redo, no `.FCStd` pollution.
- **D-11: On sketch exit, auto-select the just-finished sketch as the profile and switch the ribbon to the Features tab — but DO NOT auto-start a feature command; the user clicks Extrude/Revolve with the profile pre-selected (true SW behavior).** Reuse the existing `signalResetEdit` (the same signal `FwRibbonContext` consumes) to (a) set the finished sketch as the current `Gui::Selection`, and (b) let the ribbon context land on the Features tab. No new feature-launch logic.
- **D-12: Plane/face → Sketch entry reuses FreeCAD's existing sketch-creation flow; FreeWorks does not reimplement sketch creation.** It only ensures the entry *feels* SW (plane/face pick → Sketch command → the sketcher edit panel opens in the LEFT PropertyManager via D-01).

### Claude's Discretion

- Exact container seam — re-host the live `Control::taskPanel()` `TaskView` vs. a thin `FwPropertyManager` that adopts it — resolved by the **D-03 spike**.
- Slide animation vs. instant reveal (D-08) — animate only if low-risk and cross-platform-clean.
- Precise active-box pink tone and ✓/✗ glyphs — functional/palette-derived now, final art Phase 7.
- The exact key-event override mechanism for D-10 (event filter vs. container `keyPressEvent`).

### Deferred Ideas (OUT OF SCOPE)

- **SW icon artwork, exact final colors, fonts** → Phase 7. Only functional state colors (active-box pink, ✓/✗ semantics) in scope now.
- **Selection-model / box-select / canvas accelerators / context toolbars** → Phase 5. This phase only consumes selection to fill reference boxes.
- **Multi-body / assembly PropertyManager semantics** → out of scope (single active Body/edit this milestone).
- **Custom per-command PropertyManager layouts beyond what existing TaskDialogs provide** → out of scope; reuse existing panels.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PROP-01 | Left slide-in panel: ✓/✗ header, collapsible rollouts, live fields, non-modal, live preview | Re-host "Tasks" `TaskView` left (Control.cpp:55, MainWindow.cpp:609); rollouts already exist (`TaskGroup`/`TaskBox`, TaskView.h:67/82); accept/reject already wired (TaskView.cpp:644-683); non-modal is the default TaskView behavior (it is a dock, not a `QDialog::exec`); live preview from hosted panels' `PreviewExtension` (ViewProvider.cpp:213-230). New FreeWorks work = left re-host + header affordance + optional `QPropertyAnimation` reveal. |
| PROP-02 | Pink active selection-reference boxes; fill on pick, highlight, auto-expand, clear/remove | The box machinery already lives INSIDE each hosted panel via `Gui::SelectionFilterGate` + `Gui::SelectionObserver` (e.g. `PartDesignGui::ReferenceSelection`, ReferenceSelection.h:42). FreeWorks adds only the palette-derived **pink active-state** restyle at the container level + reads `Gui::Selection` (FwSelectionGuard pattern). Fill/highlight/auto-expand/clear-remove are already provided by the hosted panel. |
| TREE-03 | Double-click feature in tree → edit in PropertyManager (F2 half shipped 03-02) | Confirmed full chain: `doubleClicked → ViewProvider::setEdit(Default) → Gui::Control().showDialog(featureDlg)` (PartDesign ViewProvider.cpp:114-176). Because Control is left-hosted (D-01), the panel lands left automatically. Inherited `FwFeatureTree : Gui::TreeWidget` double-click already routes here. |
| FLOW-01 | Sketch→feature flow: exit sketch → auto-select sketch as profile + Features tab ready | `signalResetEdit` fires on edit exit (Document.cpp:750), already consumed by `FwRibbonContext` (FwRibbonContext.cpp:111). Add: on sketch reset, `Gui::Selection().addSelection(finished sketch)` + land Features tab. No feature auto-launch (true SW). |

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Panel container / dock placement | Gui (FreeWorks `FwLayout`) | Gui (`DockWindowManager`) | Pure window-layout concern; FreeWorks owns the left re-host, DockWindowManager owns the QDockWidget lifecycle. No App-layer involvement. |
| Rollout groups / accept-reject / key handling | Gui (`Gui::TaskView::TaskView`, reused) | Gui (FreeWorks header overlay) | The task-panel widget tier already owns this; FreeWorks adds a thin chrome layer above it. |
| Per-command field layout + live preview + transactions | Gui (hosted `TaskDialog`, e.g. PartDesign) | App (`App::Document` transactions, `PreviewExtension`) | Each hosted panel owns its fields, preview recompute, and transaction discipline — unmodified (SC4). |
| Selection-reference box machinery (fill/highlight/expand) | Gui (hosted panel's `SelectionObserver`/`SelectionFilterGate`) | Gui (`Gui::Selection` singleton) | Already inside the hosted panel; FreeWorks does not own the picking, only the pink active-state restyle. |
| Edit entry from tree double-click | Gui (`ViewProviderDocumentObject::doubleClicked` → `setEdit`) | Gui (`Gui::Document::setEdit` → `signalInEdit`) | Standard FreeCAD edit path; FreeWorks reuses inherited tree behavior. |
| FLOW-01 sketch→feature handoff | Gui (FreeWorks, on `signalResetEdit`) | Gui (`Gui::Selection`, `FwRibbonContext`) | Selection + ribbon-tab are Gui concerns; reuse existing signals and the `FwRibbonContext` tab-switch precedent. |

## Standard Stack

This is a native C++/Qt desktop fork. The "stack" is the set of **in-tree FreeCAD Gui classes reused verbatim** — no external packages are installed this phase. (See Package Legitimacy Audit: N/A.)

### Core (reuse verbatim — host, do not replace)

| Class / API | File:line | Purpose | Why Standard |
|-------------|-----------|---------|--------------|
| `Gui::ControlSingleton` (`Control()`) | src/Gui/Control.h:54, Control.cpp:55 | Routes the active `TaskDialog` into `taskPanel()` (the "Tasks" `TaskView`); exposes `accept()`/`reject()`/`closeDialog()` | Every PartDesign/Sketcher feature dialog shows through it — the single seam (D-01). `taskPanel()` resolves via `getDockWindow("Tasks")`. |
| `Gui::TaskView::TaskView` | src/Gui/TaskView/TaskView.h:163 | The `QStackedWidget` host for task panels; observes `Gui::Selection`; already handles Enter/Esc (`keyPressEvent`, TaskView.cpp:393) | The widget FreeWorks re-docks left. objectName `"Tasks"`, dock container `"Std_TaskView"` (MainWindow.cpp:620-628). |
| `Gui::TaskView::TaskGroup` / `TaskBox` | src/Gui/TaskView/TaskView.h:67/82 | Collapsible rollout groups (QSint `ActionBox`/`ActionGroup`) | PROP-01 rollouts already exist — no custom rollout widget needed (D-04). |
| `Gui::TaskView::TaskDialog` | src/Gui/TaskView/TaskDialog.h | Per-command panel contract; transaction-aware; `getStandardButtons()`/`modifyStandardButtons()` feed the `QDialogButtonBox` | The accept/reject + transaction discipline FreeWorks reuses (D-10). |
| `Gui::TaskView::TaskEditControl` | src/Gui/TaskView/TaskEditControl.h | Holds the `QDialogButtonBox` whose `accepted`/`rejected` drive `TaskView::accept`/`reject` (TaskView.cpp:644-683) | The existing OK/Cancel surface the green-✓/red-✗ header maps onto. |
| `Gui::DockWindowManager` | src/Gui/DockWindowManager.h:84-105 | `registerDockWindow`/`findRegisteredDockWindow`/`getDockWindow`/`getDockContainer`/`addDockWindow` | The find-or-reuse mount API used by `FwLayout` (Phases 1-3). `getDockContainer("Std_TaskView")` returns the movable QDockWidget. |
| `Gui::Application::signalInEdit` / `signalResetEdit` | src/Gui/Application.h:154/156 (emitted Document.cpp:313/750) | Event-driven edit-enter / edit-exit hooks | The reveal trigger (PROP-01) + FLOW-01 handoff; `FwRibbonContext` already consumes them. |
| `Gui::Selection` (`SelectionSingleton`) + `SelectionObserver` + `SelectionFilterGate` | src/Gui/Selection/Selection.h, SelectionFilter.h | Global selection the reference boxes read/drive | D-05: drive/read only, no parallel backend. `PartDesignGui::ReferenceSelection` (ReferenceSelection.h:42) is the in-panel precedent. |

### Supporting (FreeWorks-side, new this phase — additive)

| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `FwLayout::mountPropertyManager()` (new) | Move the existing `"Std_TaskView"` dock to `Qt::LeftDockWidgetArea` (find-or-reuse, idempotent); replace the `Fw_PropertyManager` placeholder semantics | At `FwWorkbench::activated()` (mirror `mountRibbon`/`mountFeatureManager`); reverse in `deactivated()`. |
| `FwPropertyManagerHeader` (new, thin) | SolidWorks green-✓ / red-✗ header band that calls `Gui::Control().accept()` / `reject()`; palette-only color | The PROP-01 ✓/✗ header (D-04). Lives above/around the hosted `TaskView`. |
| `FwPropertyKeyFilter` (new, thin `QObject` event filter) OR container `keyPressEvent` | D-10 Tab traversal + ensure Enter→accept / Esc→reject at container level | Installed on the hosted `TaskView` container; do NOT edit `TaskView::keyPressEvent`. |
| `FwReferenceBoxStyler` (new, thin) | Apply palette-derived **pink** active tone to the currently-active reference box; read active-box state from `Gui::Selection`/hosted panel | PROP-02 functional pink-state (D-06). The box itself is owned by the hosted panel. |
| Reuse `FwSelectionGuard` (Phase 3) | Snapshot/restore `Gui::Selection` + preselection (RAII) | FLOW-01 auto-profile-select (D-11) and any FreeWorks-driven selection mutation. |
| Reuse `FwRibbonContext` (Phase 2) | Tab handoff to the Features tab on sketch exit | FLOW-01 (D-11) — extend or pair with the existing `signalResetEdit` handler. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Moving the existing `"Std_TaskView"` dock container left | Reparenting the bare `TaskView` widget into a new `Fw_PropertyManager` QDockWidget | Reparenting risks breaking `getDockWindow("Tasks")` (Control.cpp:58 looks up by widget objectName via the dock list) and the OverlayWidgets `findChild<TaskView*>` (OverlayWidgets.cpp:2850). Moving the *container* preserves widget identity + objectName + all lookups — strictly safer. This is the D-03 spike's primary hypothesis. |
| Reuse `TaskView::accept()`/`reject()` for ✓/✗ | A bespoke FreeWorks accept/reject backend | A custom backend would duplicate the transaction-commit path each `TaskDialog` already runs — the explicit anti-goal (D-01, last resort only). |
| Restyle the active reference box pink at container level | Editing each hosted panel to add a pink box | Editing hosted panels violates SC4 ("work unmodified") and additive-merge discipline. Container-level restyle keeps panels untouched. |

**Installation:** None. All sources compile into `FreeCADGui` via `target_sources(FreeCADGui PRIVATE …)` (FreeWorks/CMakeLists.txt:56). New files added additively to `FreeWorks_CPP_SRCS`/`FreeWorks_HPP_SRCS`; new tests added to `tests/src/Gui/CMakeLists.txt`.

**Version verification:** N/A — no external packages. Build toolchain is the existing pinned pixi/conda + Qt 6.8 + OCCT 7.8 environment (CLAUDE.md). The only "version" risk is the upstream type-name string contract (see Pitfalls / Open Questions).

## Package Legitimacy Audit

**Not applicable.** This phase installs **zero external packages**. All components are in-tree FreeCAD Gui classes reused verbatim and FreeWorks-side additive C++ files compiled into the existing `FreeCADGui` library. No npm/PyPI/crates dependency is added.

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
USER ACTION                          SIGNAL / CALL PATH                         RESULT (LEFT PropertyManager)
───────────                          ──────────────────                         ─────────────────────────────

[Start feature cmd] ───────────────► Command fires → ViewProvider::setEdit ──┐
                                       (PartDesign VP.cpp:114)                │
[Double-click tree feature] ───────► doubleClicked → setEdit(Default) ───────┤
                                       (ViewProviderDocumentObject)          │
                                                                             ▼
                                     Gui::Control().showDialog(TaskDialog) ──► TaskView ("Tasks" dock)
                                       (Control.cpp:149)                       │  ← already moved LEFT by
                                                                              │     FwLayout::mountPropertyManager
   Gui::Document::setEdit ──► signalInEdit ─────────────────────────────────►├─► FreeWorks reveal/animate (D-07/D-08)
        (Document.cpp:313)                                                    │
                                                                             ▼
                                     ┌──────────────────────────────────────────────────┐
                                     │  Fw_PropertyManager dock (left)                    │
                                     │  ┌────────────────────────────────────────────┐  │
                                     │  │ FwPropertyManagerHeader  [ ✓ green ][ ✗ red ]│──┼─► Control().accept()/reject()
                                     │  ├────────────────────────────────────────────┤  │   (D-04)
                                     │  │ HOSTED TaskDialog (unmodified, SC4):        │  │
                                     │  │   ▸ TaskBox/TaskGroup rollouts (collapsible)│  │
                                     │  │   ▸ reference box ──[PINK when active]──────┼──┼─► FwReferenceBoxStyler restyle
                                     │  │       (SelectionFilterGate + Observer)      │  │   reads Gui::Selection (D-05/06)
                                     │  │   ▸ live fields → PreviewExtension recompute│  │   live preview (non-modal)
                                     │  └────────────────────────────────────────────┘  │
                                     └──────────────────────────────────────────────────┘
                                                                             │
[Pick geometry in 3D view] ────────► Gui::Selection (non-modal, still pickable) ─► hosted box fills + highlights + expands

[Enter] ─► FwKeyFilter/TaskView keyPressEvent ─► accept()    [Esc] ─► reject()    [Tab] ─► next field/box (D-10)

[Exit sketch] ─► signalResetEdit ──► FwRibbonContext + FLOW handler ─► Gui::Selection.addSelection(sketch)
   (Document.cpp:750)                  (FwRibbonContext.cpp:111)        + ribbon → Features tab (D-11, no auto-launch)
```

### Recommended Project Structure

```
src/Gui/FreeWorks/
├── FwLayout.{h,cpp}                 # ADD mountPropertyManager()/unmountPropertyManager() (mirror mountFeatureManager)
├── FwPropertyManagerHeader.{h,cpp}  # NEW: green-✓/red-✗ header → Control().accept()/reject() (palette-only)
├── FwPropertyKeyFilter.{h,cpp}      # NEW: D-10 Tab traversal + Enter/Esc container-level guarantee
├── FwReferenceBoxStyler.{h,cpp}     # NEW: pink active-box restyle, reads Gui::Selection (D-05/D-06)
├── FwSelectionGuard.{h,cpp}         # REUSE (Phase 3): selection snapshot/restore for FLOW-01
├── FwRibbonContext.{h,cpp}          # REUSE/EXTEND (Phase 2): Features-tab handoff on sketch exit (FLOW-01)
└── SPIKE.md / SPIKE_LIVE_CHECKLIST.md  # D-03 verdict + live-only FEEL items

tests/src/Gui/
├── FwPropertyManager.cpp            # NEW GTest (Gui_tests_run): pure logic — header→accept mapping, key map, flow decision
├── FwPropertyManagerWidget.cpp      # NEW QTEST (setup_qt_test, offscreen): left re-host, rollouts intact, accept/reject fire
└── FwReferenceBoxStyler.cpp         # NEW: pink-state state machine (active/inactive/filled), palette-role assertion
```

### Pattern 1: Find-or-reuse left re-host (mirror `mountFeatureManager`)
**What:** Idempotently move the existing `"Std_TaskView"` QDockWidget into the left area at FreeWorks activation; reverse on deactivation. Preserve the `"Tasks"` widget objectName and identity.
**When to use:** `FwWorkbench::activated()` (after `StdWorkbench::activated()`), paired teardown in `deactivated()`.
**Example:**
```cpp
// Source: pattern from src/Gui/FreeWorks/FwLayout.cpp:117-146 (mountFeatureManager) +
//         src/Gui/DockWindowManager.cpp:337 (getDockContainer) + MainWindow.cpp:620-628
void FwLayout::mountPropertyManager()
{
    Gui::MainWindow* mw = Gui::getMainWindow();
    auto* mgr = Gui::DockWindowManager::instance();
    if (!mw || !mgr) { return; }

    // The "Tasks" TaskView lives in the dock container registered as "Std_TaskView"
    // (MainWindow.cpp:628). Move the CONTAINER left — do NOT reparent the TaskView widget
    // (Control.cpp:58 / OverlayWidgets.cpp:2850 depend on its objectName + identity).
    QDockWidget* taskDock = mgr->getDockContainer("Std_TaskView");
    if (!taskDock) { return; }

    // Idempotent: only move if not already in the left area.
    if (mw->dockWidgetArea(taskDock) != Qt::LeftDockWidgetArea) {
        mw->addDockWidget(Qt::LeftDockWidgetArea, taskDock);  // re-docks, preserves widget
        taskDock->show();
    }
    // OPTIONAL: tabify/stack under Fw_PropertyManager or place below Fw_FeatureManager.
}
```

### Pattern 2: Header affordance drives existing accept/reject
**What:** SolidWorks ✓/✗ header buttons call the existing Control singleton; no new commit logic.
**Example:**
```cpp
// Source: src/Gui/Control.h:98-100 (accept/reject slots) + TaskView.cpp:644-683 (existing button wiring)
connect(acceptButton, &QToolButton::clicked, [] { Gui::Control().accept(); });   // green ✓
connect(rejectButton, &QToolButton::clicked, [] { Gui::Control().reject(); });   // red ✗
// Color via palette role ONLY (no hex): mirror FwFeatureTreeDelegate.cpp:157-159 precedent.
```

### Pattern 3: FLOW-01 handoff on sketch exit (reuse `signalResetEdit`)
**What:** On sketch edit reset, select the finished sketch and land the Features tab — no feature auto-launch.
**Example:**
```cpp
// Source: src/Gui/FreeWorks/FwRibbonContext.cpp:111-122 (existing signalResetEdit consumer) +
//         Selection.h addSelection; identify sketch by type-name STRING (no Sketcher link — Pitfall 2)
app->signalResetEdit.connect([](const Gui::ViewProviderDocumentObject& vp) {
    if (vp.getTypeId().getName() == std::string("SketcherGui::ViewProviderSketch")) {
        App::DocumentObject* sketch = vp.getObject();        // the just-finished sketch
        Gui::Selection().clearSelection();
        Gui::Selection().addSelection(sketch->getDocument()->getName(), sketch->getNameInDocument());
        // ribbon → Features tab handled by FwRibbonContext's existing restore path
    }
});
```

### Anti-Patterns to Avoid
- **Editing `Control.cpp` or `TaskView.cpp` bodies** to add left-placement, a pink box, or Tab handling. Violates D-02 and upstream-merge discipline. Use container-level FreeWorks code + `// SW-FORK HOOK` only if a shared touch is truly unavoidable.
- **Reparenting the `TaskView` widget out of "Std_TaskView" into a fresh `Fw_PropertyManager` dock.** Breaks `getDockWindow("Tasks")` (Control.cpp) and `findChild<TaskView*>` (OverlayWidgets). Move the *container* instead.
- **Building a parallel selection backend for reference boxes.** D-05/Phase-5 boundary — the hosted panel already owns picking via `SelectionFilterGate`/`SelectionObserver`. FreeWorks only restyles the active state and reads `Gui::Selection`.
- **Auto-launching a feature command on sketch exit.** D-11 forbids it — SW leaves the sketch selected and lets the user choose. Only select + tab-switch.
- **Polling `Control().activeDialog()` for reveal.** It has no change signal (Pitfall 1, established in Phase 2). Use `signalInEdit`/`signalResetEdit` instead.
- **A hard-modal panel (`QDialog::exec`).** Explicitly rejected (REQUIREMENTS Out-of-Scope; D-07). The `TaskView` dock is already non-modal — keep it that way.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Collapsible rollout groups | A custom expander widget | `Gui::TaskView::TaskBox`/`TaskGroup` (QSint) | Already collapsible, already styled, already what every hosted panel uses (D-04). |
| Accept/Cancel + transaction commit | A FreeWorks commit pipeline | `Gui::Control().accept()/reject()` → `TaskDialog::accept/reject` | Each panel already wraps its own `openTransaction`/`commitTransaction` — clean undo (D-10). |
| Enter→accept / Esc→reject | A new key handler from scratch | Existing `TaskView::keyPressEvent` (TaskView.cpp:393-457) + a thin filter only for Tab/edge cases | FreeCAD already maps Enter/Esc to the default/escape buttons, incl. a macOS crash workaround (TaskView.cpp:403). |
| Reference-box picking (fill/highlight/expand) | A parallel picker | The hosted panel's `SelectionObserver` + `SelectionFilterGate` (`ReferenceSelection.h:42`) | Picking, highlight, and auto-expand are already implemented per-panel — FreeWorks colors the active state only. |
| Selection snapshot/restore | A new RAII guard | Reuse `FreeWorksGui::FwSelectionGuard` (Phase 3) | Already handles the preselect-clobber pitfall (clearPreSelect defaults true). |
| Live preview during edit | A FreeWorks preview/recompute loop | Hosted panel's `Part::PreviewExtension` + `PreviewUpdateScheduler` (ViewProvider.cpp:213-230) | Preview + scheduling already exists and is debounced; running the document mutation in FreeWorks would risk the "modify-during-recompute" anti-pattern. |
| Edit entry from tree | A new edit dispatcher | Inherited `Gui::TreeWidget` double-click → `setEdit` → `Control().showDialog` | The full chain already works (PartDesign VP.cpp:114-176); `FwFeatureTree` inherits it. |

**Key insight:** In this domain the "feature" is *placement, chrome, and glue*, not mechanism. Every hard part (transactions, preview, picking, rollouts, key handling) is already solved in the hosted machinery. The fork's value is making it *look and feel* SolidWorks while the engine stays untouched — and the moment the dock is left-hosted, ~80% of the success criteria are satisfied with no new mechanism.

## Runtime State Inventory

> This phase is **additive Gui chrome**, not a rename/refactor/migration. No stored data, no string rename. The one persisted-state consideration is dock layout.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — no datastore keys, collection names, or persisted IDs touched. | none |
| Live service config | None — no external services. | none |
| OS-registered state | None. | none |
| Secrets/env vars | None. | none |
| Build artifacts | None new beyond the additive FreeWorks `.cpp`/`.h` compiled into `FreeCADGui`. | none |
| **Persisted dock layout (Qt `saveState`/`restoreState`)** | Moving the `"Std_TaskView"` dock left changes the `QMainWindow` saved layout. The dock objectName `"Std_TaskView"` and widget objectName `"Tasks"` MUST be preserved so `saveState`/`restoreState` round-trips (same discipline as Phase 2 ribbon + Phase 3 tree). | Preserve objectNames; verify the moved dock round-trips a save/restore (headless QTEST + live restart checklist item). Reverse the move in `deactivated()` so stock workbenches keep the right-docked Tasks panel (mirror `restoreStockChrome`). |

## Common Pitfalls

### Pitfall 1: `Control().activeDialog()` has no change signal — do not poll it
**What goes wrong:** Reading `Control().activeDialog()` in a timer/loop to detect "a panel opened" is laggy and racy.
**Why it happens:** It is a plain getter (Control.h:69); no signal is emitted on dialog change.
**How to avoid:** Subscribe to `Gui::Application::signalInEdit` / `signalResetEdit` (Document.cpp:313/750) — the event-driven realization, already used by `FwRibbonContext` (FwRibbonContext.cpp:99-121). Documented precedent: STATE.md `[02-04]`.
**Warning signs:** Timer-based dock reveal; flicker; "panel appears a frame late."

### Pitfall 2: Sketch identification must be a type-name STRING, never a Sketcher include
**What goes wrong:** Including a Sketcher/PartDesign header to `dynamic_cast` the view provider creates a compile/link dependency that breaks additive-merge discipline.
**Why it happens:** The obvious way to detect "this is a sketch" is a typed cast.
**How to avoid:** Compare `vp.getTypeId().getName() == "SketcherGui::ViewProviderSketch"` — a STRING, zero link (FwRibbonContext.cpp:54). Same for any feature-type check.
**Warning signs:** New `#include <Mod/Sketcher/...>` or `target_link_libraries(... SketcherGui)` in FreeWorks. **This literal is an external contract requiring live re-validation** (an upstream rename silently breaks it — see Open Questions).

### Pitfall 3: `PartDesign::ViewProvider::setEdit` switches the active workbench
**What goes wrong:** `setEdit(Default)` calls `Gui::Command::assureWorkbench("PartDesignWorkbench")` (ViewProvider.cpp:165) and restores the prior WB in `unsetEdit` (VP.cpp:194-197). If a feature is edited from FreeWorks mode, FreeCAD may switch the active workbench out of `FwWorkbench` during the edit, then back on exit — potentially tearing down/re-mounting the ribbon + docks mid-edit.
**Why it happens:** PartDesign assumes its own workbench owns the edit context.
**How to avoid:** Verify (D-03 spike / live checklist) what happens to the FreeWorks ribbon + left dock when `assureWorkbench` fires during a double-click edit. If FreeWorks chrome is torn down, the mount must be idempotent and survive (mounts already are — `mountRibbon`/`mountFeatureManager` are find-or-reuse). Confirm the left re-host survives a WB round-trip; if not, re-assert it on `signalInEdit`. **Flag for the spike — this is the highest-risk integration unknown.**
**Warning signs:** Ribbon disappears when double-clicking a Pad; left panel jumps back right mid-edit.

### Pitfall 4: Reparenting the TaskView widget breaks name-based lookups
**What goes wrong:** Pulling the `TaskView` out of "Std_TaskView" into a new dock changes the dock list so `getDockWindow("Tasks")` (Control.cpp:58, via the widget objectName) or `findChild<TaskView*>` (OverlayWidgets.cpp:2850) may fail.
**Why it happens:** `getDockWindow` matches the *dock container* objectName against `"Tasks"`? — No: it returns `(*it)->widget()` where the **dock container** objectName is matched (DockWindowManager.cpp:325). The container is `"Std_TaskView"`, the widget is `"Tasks"`. `Control::taskPanel()` calls `getDockWindow("Tasks")` — meaning it relies on a dock whose objectName is `"Tasks"`. **Re-verify this exact matching in the spike** (container vs widget objectName) before choosing the move strategy.
**How to avoid:** Move the existing dock container left (Pattern 1) rather than reparenting; preserve every objectName. Add a headless test asserting `Control().taskPanel() != nullptr` after the left move.
**Warning signs:** `Control().showDialog` warns "should return the pointer to combo view" / returns early; panels stop appearing.

### Pitfall 5: Container-level Tab handling can fight the hosted panel's own focus chain
**What goes wrong:** A FreeWorks Tab override may skip or double-advance fields the hosted panel manages.
**Why it happens:** The hosted `TaskDialog` builds its own tab order; an aggressive event filter that consumes Tab everywhere overrides it.
**How to avoid:** Scope the Tab override to "advance between reference boxes / top-level fields" and let Qt's default `focusNextChild` handle intra-field traversal; only intercept where SW differs. Test the focus chain headlessly with a synthetic panel.
**Warning signs:** Tab gets "stuck"; focus leaves the panel into the 3D view unexpectedly.

### Pitfall 6: Pink active-box state must track the hosted panel's *active* reference box, not the whole panel
**What goes wrong:** Coloring the entire panel or the wrong box pink when multiple reference boxes exist.
**Why it happens:** FreeWorks does not own the boxes; it must observe which one is currently armed.
**How to avoid:** Read the active-box signal from the hosted panel where exposed; otherwise track "which reference widget has focus / is in selection-gate mode" via the focus + `Gui::Selection` state. Keep it a *visual restyle* only (palette role), reversible, and Phase-7-overridable. **The exact hook for "which box is active" is an Open Question — resolve in the spike.**
**Warning signs:** Two boxes pink at once; pink persists after a box fills/deactivates.

## Code Examples

### Discover and move the Tasks dock left (verified seam)
```cpp
// Source: DockWindowManager.cpp:337 (getDockContainer) + MainWindow.cpp:620-628 (objectNames)
auto* mgr = Gui::DockWindowManager::instance();
QDockWidget* tasks = mgr->getDockContainer("Std_TaskView");   // container objectName
// tasks->widget() is the Gui::TaskView::TaskView whose objectName is "Tasks"
Gui::getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, tasks);  // preserves identity
```

### Assert the host is reachable after the move (headless test shape)
```cpp
// Source: pattern from tests/src/Gui/FwTestGuiBootstrap.h + Control.cpp:55
tests::ensureGuiTestBootstrap();
FreeWorksGui::FwLayout::mountPropertyManager();
EXPECT_NE(Gui::Control().taskPanel(), nullptr);                 // still found via getDockWindow("Tasks")
EXPECT_EQ(Gui::getMainWindow()->dockWidgetArea(
              Gui::DockWindowManager::instance()->getDockContainer("Std_TaskView")),
          Qt::LeftDockWidgetArea);
```

### Map ✓/✗ header to existing accept/reject
```cpp
// Source: Control.h:98-100
connect(greenCheck, &QAbstractButton::clicked, [] { Gui::Control().accept(); });
connect(redCross,   &QAbstractButton::clicked, [] { Gui::Control().reject(); });
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Task panels in a right "Tasks" dock / or inside ComboView "Model" | Standalone `Std_TaskView` dock holding a `TaskView` (`"Tasks"`), resolved by `Control::taskPanel()` via `getDockWindow("Tasks")` | Current `main` (1.2.0-dev) | The TaskView is a movable standalone dock — re-hosting left is a window-management move, not a surgery. |
| Per-phase bespoke widgets | Additive `FreeWorksGui` re-host of stock subsystems (ribbon, tree, now panel) | Phases 1-3 (this fork) | The phase-4 mount mirrors `mountRibbon`/`mountFeatureManager` exactly. |

**Deprecated/outdated:**
- Qt5 support is deprecated as of 2026-08 (CLAUDE.md) — target Qt 6.8 APIs only (`QDockWidget`, `QPropertyAnimation`, palette roles).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Moving the `"Std_TaskView"` dock container left preserves `Control::taskPanel()` resolution (`getDockWindow("Tasks")` still finds it). The exact container-vs-widget objectName matching in `getDockWindow` must be re-verified live. | Pitfall 4 / Pattern 1 | If `getDockWindow("Tasks")` matches the *container* name (not the widget), the lookup expects a dock objectName `"Tasks"` — but the container is `"Std_TaskView"`. **This is the single most load-bearing assumption — the D-03 spike must confirm it before committing the move strategy.** If wrong, fall back to the D-03 thin-container adoption path. |
| A2 | `PartDesign::ViewProvider::setEdit`'s `assureWorkbench("PartDesignWorkbench")` does not permanently tear down the FreeWorks left re-host / ribbon during a tree double-click edit, OR the idempotent mounts re-assert cleanly. | Pitfall 3 | If the WB switch tears down FreeWorks chrome mid-edit, the panel/ribbon flicker or vanish — breaks SC3/SC4 feel. Spike must observe a real double-click edit. |
| A3 | The hosted panel exposes (or can be observed for) which reference box is currently active, enough to restyle exactly one box pink. | Pitfall 6 / D-05 | If no clean hook exists, FreeWorks must infer active-box from focus + selection-gate state — more fragile; may need a `// SW-FORK HOOK` accessor. |
| A4 | `signalInEdit` fires for sketch/feature command starts (not only tree double-click) such that it is a reliable reveal trigger for PROP-01. | Pattern / FLOW | If a command starts a TaskDialog without entering VP edit mode, the reveal must also hook `Control().showDialog`-adjacent state; verify both feature-command and sketch-command entry in the spike. |
| A5 | A `QPropertyAnimation` width/visibility slide on the left QDockWidget is cross-platform-clean (esp. macOS native dock behavior). | D-08 | If janky on macOS, fall back to instant reveal (D-08 already permits this) — non-blocking. |
| A6 | Tab traversal can be added at the container level without breaking each hosted panel's internal focus chain. | Pitfall 5 / D-10 | If it fights the hosted chain, scope Tab to inter-box only; live checklist FEEL item. |

## Open Questions

1. **Does `getDockWindow("Tasks")` resolve by container objectName or widget objectName after a left move?** (A1)
   - What we know: `getDockWindow` iterates `_dockedWindows` (QDockWidgets) and matches `(*it)->objectName()` against the argument, returning `(*it)->widget()` (DockWindowManager.cpp:323-327). The container is `"Std_TaskView"`; the *widget* is `"Tasks"`. Yet `Control::taskPanel()` calls `getDockWindow("Tasks")`.
   - What's unclear: This implies a dock container whose objectName is `"Tasks"` must exist — but `setupTaskView` registers it as `"Std_TaskView"`. There may be a registration path that names the container `"Tasks"`, or `qobject_cast<TaskView*>` succeeds because the lookup falls through to a different mechanism. **Must be resolved by reading the live dock list / spike**, since the whole move strategy hinges on it.
   - Recommendation: First spike task = print `Control().taskPanel()` and the dock container/widget objectNames at runtime; confirm which name the move must preserve. If the move would break it, use the D-03 thin-`FwPropertyManager`-adopt fallback.

2. **What happens to FreeWorks chrome when `assureWorkbench("PartDesignWorkbench")` fires during a tree double-click edit?** (A2 / Pitfall 3)
   - What we know: `setEdit` switches WB and restores it on `unsetEdit`.
   - What's unclear: Whether `FwWorkbench::deactivated()`/`activated()` run (tearing down + re-mounting ribbon/docks) during the edit.
   - Recommendation: Spike a real double-click Pad edit in FreeWorks mode; observe ribbon + left panel. Record in SPIKE_LIVE_CHECKLIST.md.

3. **Is the sketch-type-name literal `"SketcherGui::ViewProviderSketch"` still current on this `main`?** (Pitfall 2)
   - What we know: Confirmed present at ViewProviderSketch.cpp:562 (per FwRibbonContext comment) and already shipped in Phase 2.
   - What's unclear: Upstream rename risk on a moving target.
   - Recommendation: Reuse the existing constant; add it to the live re-validation checklist (already a Phase-2 obligation).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Qt 6.8 (`QDockWidget`, `QPropertyAnimation`, `QPalette`) | All of Phase 4 | Assumed ✓ (project baseline, CLAUDE.md) | 6.8.x | — |
| `Gui::Control` / `Gui::TaskView` (in-tree) | Host engine | ✓ (read in repo) | main 1.2.0-dev | — |
| PartDesign/Sketcher Gui modules (for live verify only) | SC4 unmodified-panel proof; sketch type-name | ✓ (bootstrap imports them, FwTestGuiBootstrap.h:99-100) | in-tree | — |
| Built GUI binary / display | Live FEEL items (slide, pink pick, WB-switch behavior, SC daily-user) | ✗ (no build tree in env — consistent with Phases 1-3) | — | Document verdicts on documentation-approval basis; defer live items to SPIKE_LIVE_CHECKLIST.md |

**Missing dependencies with no fallback:** none (the missing build/display is handled by the established SPIKE_LIVE_CHECKLIST deferral pattern).
**Missing dependencies with fallback:** Live GUI verification → headless GTest/QTEST (offscreen) for logic + a live checklist for FEEL, per Phase 1-3 precedent.

## Validation Architecture

> `workflow.nyquist_validation: true` and `tdd_mode: true` — this section is required.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Google Test (`Gui_tests_run`, logic) + Qt Test (`setup_qt_test`, offscreen widgets) — GTest 1.x, QTEST_MAIN |
| Config file | `tests/src/Gui/CMakeLists.txt` (add new `.cpp` to `Gui_tests_run` and `setup_qt_test(...)`) |
| Quick run command | `ctest -R "FwPropertyManager" --output-on-failure` |
| Full suite command | `ctest -R "Fw|Gui_tests_run" --output-on-failure` |

Bootstrap: `tests::ensureGuiTestBootstrap()` (FwTestGuiBootstrap.h) brings up App + offscreen `QApplication` + `Gui::Application(false)` + imports PartDesignGui/SketcherGui — required before any Control/TaskView/Selection assertion.

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PROP-01 | After `mountPropertyManager()`, the Tasks dock is in the LEFT area and `Control().taskPanel()` is non-null | widget (QTEST offscreen) | `ctest -R FwPropertyManagerWidget` | ❌ Wave 0 |
| PROP-01 | Header green-✓ click → `Control().accept()` path invoked; red-✗ → `reject()` (verify via a stub TaskDialog) | widget | `ctest -R FwPropertyManagerWidget` | ❌ Wave 0 |
| PROP-01 | Rollout `TaskBox`/`TaskGroup` remain collapsible after re-host (assert widget type present + `isExpandable`) | widget | `ctest -R FwPropertyManagerWidget` | ❌ Wave 0 |
| PROP-01 | Panel is non-modal: re-host does not call `exec()`; `Gui::Selection` still mutable while panel shown | logic | `ctest -R FwPropertyManager` | ❌ Wave 0 |
| PROP-02 | Reference-box styler marks exactly one box active (pink palette role) and reverts on deactivate — pure state machine | logic | `ctest -R FwReferenceBoxStyler` | ❌ Wave 0 |
| PROP-02 | Active tone resolves through a `QPalette` role, NOT a hex literal (assert no `setStyleSheet` color literal; leak-grep clean) | logic + grep | `tools/fw-string-leak-grep.sh` + `ctest -R FwReferenceBoxStyler` | ❌ Wave 0 |
| TREE-03 | `FwFeatureTree` double-click routes to `setEdit` (inherited) → `Control().showDialog` lands in the left-hosted panel | widget/integration | `ctest -R FwFeatureTreeWidget` (extend) | ⚠️ partial (tree exists) |
| FLOW-01 | On `signalResetEdit` for a sketch VP, the finished sketch becomes the current `Gui::Selection` and no feature command is launched | logic | `ctest -R FwPropertyManager` | ❌ Wave 0 |
| FLOW-01 | Sketch-exit lands the ribbon on the Features tab (reuse `FwRibbonContext` decision-core test pattern) | logic | `ctest -R FwRibbon` (extend) | ⚠️ partial |
| D-10 | Enter→accept / Esc→reject reach the hosted button box; Tab advances between reference boxes (synthetic panel) | widget | `ctest -R FwPropertyManagerWidget` | ❌ Wave 0 |
| Layout | Moved Tasks dock round-trips `saveState`/`restoreState` with objectNames preserved | widget | `ctest -R FwPropertyManagerWidget` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest -R "FwPropertyManager" --output-on-failure` + `tools/fw-string-leak-grep.sh`
- **Per wave merge:** `ctest -R "Fw|Gui_tests_run" --output-on-failure`
- **Phase gate:** Full FreeWorks suite green + leak-grep clean before `/gsd-verify-work`; live FEEL items recorded in SPIKE_LIVE_CHECKLIST.md (not gating code completion, per Phase 1-3 precedent).

### Wave 0 Gaps
- [ ] `tests/src/Gui/FwPropertyManager.cpp` — GTest logic (header→accept/reject mapping, FLOW-01 decision, non-modal assertion). Covers PROP-01, FLOW-01, D-10 logic.
- [ ] `tests/src/Gui/FwPropertyManagerWidget.cpp` — QTEST offscreen (left re-host, dock area, rollouts intact, accept/reject fire, saveState round-trip, Tab traversal). Covers PROP-01, TREE-03, D-10, layout.
- [ ] `tests/src/Gui/FwReferenceBoxStyler.cpp` — pink active-state state machine + palette-role assertion. Covers PROP-02.
- [ ] Wire all three into `tests/src/Gui/CMakeLists.txt` (`Gui_tests_run` + `setup_qt_test`).
- [ ] D-03 SPIKE harness (throwaway) + `SPIKE.md` verdict before the full build (resolves A1/A2/A3 Open Questions). Mirror `02-commandmanager-ribbon/SPIKE.md`.
- Framework install: none — GTest + Qt Test already configured.

## Security Domain

> `security_enforcement` not set to false → section included. This is a desktop CAD UI fork with **no auth, no network, no untrusted input, no crypto** in Phase 4 scope.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No auth surface in a desktop GUI panel. |
| V3 Session Management | no | No sessions. |
| V4 Access Control | no | No multi-user/permission model. |
| V5 Input Validation | partial | Field validation is owned by the hosted `TaskDialog`/property setters (App-layer `setValue` validates/clamps, per ARCHITECTURE.md). FreeWorks adds no new input parsing. |
| V6 Cryptography | no | No crypto. |

### Known Threat Patterns for {Qt desktop / FreeCAD Gui fork}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Document mutation during recompute (corruption) | Tampering | Do NOT mutate `App::Document` from FreeWorks during recompute; route all edits through the hosted `TaskDialog`'s transaction discipline (ARCHITECTURE.md anti-pattern; D-10). |
| Coin3D / Qt object leak over long session | Denial of Service (resource) | Leak-grep (`tools/fw-string-leak-grep.sh`) + `unique_ptr`-until-adopt mount discipline (FwLayout precedent); reversible teardown in `deactivated()`. |
| Dangling pointer after dock/widget teardown | Tampering/crash | `QPointer` for held widgets, scoped signal connections released on teardown (FwRibbonContext precedent); `FwSelectionGuard` owns by-value copies (no borrowed `const char*`). |
| Trademark/asset leak ("SolidWorks" string, proprietary art) | (compliance) | No "SolidWorks" identifier in source; palette-only color, recreated art only; leak-grep guard (CLAUDE.md legal constraint). |

## Sources

### Primary (HIGH confidence — read in this repo this session)
- `src/Gui/Control.h` / `src/Gui/Control.cpp` — `taskPanel()` via `getDockWindow("Tasks")` (cpp:55-61); `showDialog` (cpp:149-195); `accept`/`reject`/`closeDialog` slots (h:98-100).
- `src/Gui/TaskView/TaskView.h` / `TaskView.cpp` — `TaskGroup`/`TaskBox` rollouts (h:67/82); `accept`/`reject` (h:212-213); `keyPressEvent` Enter/Esc (cpp:393-457); button-box wiring (cpp:644-686).
- `src/Gui/MainWindow.cpp` — `setupTaskView()` creates `TaskView`, objectName `"Tasks"`, registered `"Std_TaskView"`, docked right (lines 609-633); `updateTaskView` (748-773).
- `src/Gui/DockWindowManager.h` / `.cpp` — `getDockWindow`/`getDockContainer`/`addDockWindow`/find-or-reuse API (h:84-105; cpp:251-347).
- `src/Gui/ViewProvider.h` — `setEdit`/`unsetEdit`/`EditMode`/`setEditViewer` (640-680).
- `src/Mod/PartDesign/Gui/ViewProvider.cpp` — `setEdit → Control().showDialog` chain (114-176); `unsetEdit` (190-211); `assureWorkbench` WB switch (165); live preview `updateData`/`PreviewExtension`/scheduler (213-230).
- `src/Mod/PartDesign/Gui/ReferenceSelection.h` — in-panel `SelectionFilterGate` reference-box pattern (42-129).
- `src/Gui/Application.h` — `signalInEdit`/`signalResetEdit` (154/156); emitted `src/Gui/Document.cpp:313/750`.
- `src/Gui/FreeWorks/FwLayout.cpp` — `mountFeatureManager`/`mountRibbon` find-or-reuse mount discipline (117-300).
- `src/Gui/FreeWorks/FwWorkbench.cpp` — `activated`/`deactivated` mount/teardown ordering; `setupDockWindows` left-dock placement (95-125).
- `src/Gui/FreeWorks/FwRibbonContext.cpp` — `signalInEdit`/`signalResetEdit` consumer; sketch type-name string contract (54, 99-168).
- `src/Gui/FreeWorks/FwSelectionGuard.h` — reusable selection/preselection RAII snapshot (preselect-clobber pitfall).
- `tests/src/Gui/CMakeLists.txt` + `tests/src/Gui/FwTestGuiBootstrap.h` — GTest + `setup_qt_test` offscreen patterns; GUI bootstrap.

### Secondary (MEDIUM confidence)
- `.planning/phases/04-propertymanager-panel/04-CONTEXT.md` + `04-UI-SPEC.md` — locked decisions, copy/color/spacing contract.
- `.planning/STATE.md` `[02-04]`/`[03-02]`/`[03-03]` — established mount-swap, signal, and selection-guard precedents.

### Tertiary (LOW confidence)
- None — no WebSearch was needed; the domain is fully grounded in repo source.

## Metadata

**Confidence breakdown:**
- Standard stack (reused FreeCAD classes): HIGH — every class/API read in-repo with file:line.
- Architecture (left re-host + signal glue): HIGH for the seam; MEDIUM for the *exact* dock-move resolution (Open Question 1 / A1 — spike-gated).
- Pitfalls: HIGH — derived from read source + documented Phase 1-3 precedents.
- The two highest-risk unknowns (A1 dock-name resolution, A2 WB-switch teardown) are explicitly spike-gated, matching the Phase 2/3 spike-first discipline.

**Research date:** 2026-06-14
**Valid until:** ~2026-07-14 for the FreeWorks/decision content; the upstream `Control`/`TaskView`/`setEdit` seam and the sketch type-name string track a moving `main` (1.2.0-dev) and should be re-validated at plan time and live (sketch type-name + dock-name resolution).

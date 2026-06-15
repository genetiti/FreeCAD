---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
stopped_at: Phase 04 Plan 02 complete (PROP-01 core slice — left-dock + ✓/✗ header + deferred-cancellable teardown)
last_updated: "2026-06-15T02:32:12.000Z"
last_activity: 2026-06-15 -- Phase 04 Plan 02 complete (PROP-01 core vertical slice)
progress:
  total_phases: 7
  completed_phases: 3
  total_plans: 15
  completed_plans: 13
  percent: 49
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-06)

**Core value:** A SolidWorks user can open FreeCAD and be immediately productive — it looks, navigates, and behaves like SolidWorks — with no FreeCAD tutorial required.
**Current focus:** Phase 04 — propertymanager-panel

## Current Position

Phase: 04 (propertymanager-panel) — EXECUTING
Plan: 3 of 4 (next)
Status: Executing Phase 04 (Plans 01-02 complete)
Last activity: 2026-06-15 -- Phase 04 Plan 02 complete (PROP-01 core vertical slice)

Progress: [█████░░░░░] 3 of 7 phases complete; Phase 4 Plan 02/04 done — PROP-01 core slice committed (~49%)

## Performance Metrics

**Velocity:**

- Total plans completed: 4
- Average duration: — min
- Total execution time: 0.0 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 02 | 4 | - | - |

**Recent Trend:**

- Last 5 plans: —
- Trend: —

*Updated after each plan completion*
| Phase 01 P01 | 18 | 3 tasks | 13 files |
| Phase 01 P02 | 2 | 3 tasks | 5 files |
| Phase 01 P03 | 8 | 3 tasks | 5 files |
| Phase 01 P04 | 6 | 4 tasks | 6 files |
| Phase 02 P01 | — | 3 tasks | 9 files |
| Phase 02 P02 | — | 2 tasks | 7 files |
| Phase 02 P03 | 6 | 2 tasks | 7 files |
| Phase 02 P04 | 5 | 2 tasks | 7 files |
| Phase 03 P01 | 511 | 3 tasks | 9 files |
| Phase 03 P02 | 396 | 2 tasks | 8 files |
| Phase 03 P03 | ~720* | 3 tasks | 10 files |
| Phase 04 P01 | — | 3 tasks | 7 files |
| Phase 04 P02 | ~12 | 2 tasks | 9 files |

*03-03 duration estimated from commit timestamps (feat commits 11:53–11:58 CDT) — not instrumented (gsd-tools off PATH; closed out during resume).

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Roadmap]: Phase 1 owns the additive `src/Gui/SolidWorks/` module + `SwWorkbench` seam, upstream-merge discipline (`// SW-FORK HOOK`), asset-provenance CI guard, and headless `.FCStd`-compat gate — non-negotiable foundation per PITFALLS.md.
- [Roadmap]: Ribbon (Phase 2) precedes tree/panels — highest net-new risk; native-vs-SARibbon decision must resolve before downstream tab-switching depends on it.
- [Roadmap]: PropertyManager (Phase 4) is the keystone — TREE-03, FLOW-01, PROP-02 terminate in it; built by hosting existing `Control`/`TaskView`, not replacing it.
- [Roadmap]: Gestures (CANVAS-06) + Instant3D (CANVAS-07) isolated to Phase 6 — highest UX risk (context-menu conflict, general handle system); deferred behind the validated core loop.
- [Phase ?]: [01-01]: FreeWorks Gui module compiles into FreeCADGui via target_sources; single marked add_subdirectory(FreeWorks) # SW-FORK HOOK is the only shared-file edit
- [Phase ?]: [01-01]: Permanent Fw_FeatureManager/Fw_PropertyManager/Fw_TaskPane dock names established; observe-the-DOM (no App includes, no MainWindow.cpp edits)
- [Phase 01]: [01-02]: NAV-01 implemented as a preference default (Gui::SolidWorksNavigationStyle when unset, no-clobber); macOS default = modifier-emulated MMB with GestureNavigationStyle one-click alternative
- [Phase 01]: [01-02]: Task 3 (Mac-hardware nav feel-test) approved by documentation; live real-hardware validation deferred to MACOS_NAV_PROFILE.md spike checklist (no Mac build tree available)
- [Phase ?]: [01-03]: SHELL-02 covered via headless GTest (FwWorkbench reg + Fw_* docks + NavigationStyle default), workflow_call .FCStd-compat gate (HEADLESS_OK, no App-layer GUI leak), and tri-OS launch checklist
- [Phase ?]: [01-03]: Task 3 blocking human-verify approved; live tri-OS GUI launch + CI-green confirmation deferred to CI matrix + TRIOS_LAUNCH_CHECKLIST.md sign-off per plan verification contract (no build tree/hardware/CI in env)
- [Phase ?]: [01-04]: Task 4 blocking human-verify approved; provenance guard, SolidWorks-leak grep, and pinned-commit upstream-sync drill (two // SW-FORK HOOK touches) independently re-verified and wired into CI via sub_fwForkGuards.yml
- [Phase 02]: [02-02]: Curated ribbon is a compile-time C++ table (FwRibbonRow {tab,panel,commandId} + std::span accessor) — 56 source-verified core-loop IDs placed as-is, no FreeCAD-only extras (D-08 strict); kKnownGaps intentionally empty so the per-row test stays a true typo guard
- [Phase 02]: [02-02]: Auto-derive (D-07) consumes the LIVE value-type Workbench::getToolbarItems() list (NOT the transient setupToolBars() tree activate() deletes); literal "Separator" is the separator sentinel (REVIEW concern 5)
- [Phase 02]: [02-02]: Flyout verified by inspecting the real QToolButton (MenuButtonPopup + menu>1 action), not getGroupCommands() metadata (REVIEW concern 8); unique Fw_RibbonPanel_<Tab>_<Panel> objectNames for D-14 persistence; build API buildFromCuratedMap/buildAutoDerived/setCurrentTab consumed by Plans 03/04
- [Phase 02]: [02-03]: Ribbon mounted by WRAPPING FwRibbon in a real Gui::ToolBar (objectName Fw_RibbonToolBar) added via addToolBar(Qt::TopToolBarArea, wrapper) — a genuine QMainWindow::saveState participant; NOT toolBarAreaWidget/area-widget child (REVIEW concerns 3 & 4); mount is idempotent (find-or-reuse)
- [Phase 02]: [02-03]: Stock chrome hide/restore is reversible + FreeWorks-scoped + SNAPSHOT-driven (hidden toolbar names + menu-bar visibility + macOS native-menu setting; ForceHidden/RestoreDefault round-trip), restore replays the snapshot, never an unconditional reveal (Pitfall 3 + macOS native-menu MEDIUM)
- [Phase 02]: [02-03]: D-14 persistence is TWO-LAYER — toolbar presence/area/order via QMainWindow saveState (real QToolBar); selected tab persisted SEPARATELY via ParameterGrp key User parameter:BaseApp/Preferences/FreeWorks/Ribbon (currentTab int) because saveState does not cover a QTabWidget's selected tab (REVIEW concern 4)
- [Phase 02]: [02-03]: D-12 escape hatch = pinned "More commands…" tab-bar corner widget (QToolButton + lazy QMenu from getAllCommands() grouped by module, each the command's existing QAction) + the baseline that QAction shortcuts survive menuBar()->hide(); no command stranded
- [Phase 02]: [02-03]: Live ctest + true-restart persistence + live menu-bar/macOS native-menu round-trip deferred (no build tree/GUI in env); headless QTEST_MAIN assertions authored compile-intended (mount area, idempotent remount, chrome round-trip, overflow reachable, saveState area round-trip, tab-index restore); leak-grep clean
- [Phase 02]: [02-04]: RIBBON-02 wired to Gui::Application signalInEdit/signalResetEdit (event-driven) — the documented reinterpretation of D-09's "Gui::Control active-dialog/edit state"; Control's accessor has no change signal so polling it would be laggy/racy (Pitfall 1)
- [Phase 02]: [02-04]: Sketch identified ONLY by the type-name literal "SketcherGui::ViewProviderSketch" — zero compile/link dependency on the Sketcher module (Pitfall 2); the literal is an external contract requiring live re-validation (REVIEW LOW)
- [Phase 02]: [02-04]: FwRibbonContext pure core returns a TabAction enum (SwitchToSketch/NoOp/RestorePrevious), never a raw index; explicit contextActive_ state machine — nested sketch enter is NoOp+no-re-stash (concern 7), stray reset is NoOp; ribbon held via QPointer; scoped fastsignals connections released on teardown; FwLayout owns one context per mounted ribbon, reset BEFORE the ribbon is removed (threat T-02-09)
- [Phase 02]: [02-04]: Live edit-lifecycle round-trip (enter sketch→Sketch tab→exit→restore — open item-5 in SPIKE_LIVE_CHECKLIST.md) + live type-name re-validation + ctest deferred (no build tree/GUI in env); pure-logic + bound-ribbon tests authored compile-intended; leak-grep clean
- [Phase ?]: Spike A verdict: reuse committed — thin FwFeatureTree : Gui::TreeWidget is the committed FeatureManager engine for Plans 03-02/03-03 (scoping via recursive setHidden over the DocumentItem->Body topology, proven by a REAL two-Body QTEST)
- [Phase ?]: Active Body identified by 'PartDesign::Body' type-name literal only; no PartDesign include/link; link-free Group read via getPropertyByName('Group')
- [Phase ?]: Phase 3 spike gate cleared by documentation approval (no build tree); live demonstrations deferred to FwFeatureTree_SPIKE_LIVE_CHECKLIST.md
- [Phase 03]: [03-02]: FwFeatureTreeDelegate remaps origin planes XY/XZ/YZ -> Front/Top/Right DISPLAY-ONLY via initStyleOption (object Label never written); role resolved through the stock item-object path (itemFromIndex -> DocumentObjectItem::object() -> getPropertyByName('Role')), no Datums/PartDesign link
- [Phase 03]: [03-02]: Below-tip greying is a Gui-only item data role (kBelowTipRole = Qt::UserRole+4201) painted with QPalette::Disabled Text — never an App property; Plan 03-03 sets the flag, this plan provides the palette-driven paint hook
- [Phase 03]: [03-02]: FwLayout mounts FwFeatureTree under Fw_FeatureManager via find-or-reuse (unregister placeholder, re-register tree under the SAME objectName) so saveState round-trips; mirrors mountRibbon discipline, DockWindowManager only
- [Phase 03]: [03-02]: DnD validity affordance calls Gui::TreeWidget::dragMoveEvent FIRST then only decorates event->isAccepted()==false with Qt::ForbiddenCursor + no insertion line (no transaction on BLOCK); never re-implements/re-calls the drop gate (TREE-04, D-10/D-11/D-12)
- [Phase 03]: [03-02]: A1 plane correspondence (Front=XY/Top=XZ/Right=YZ) carried forward for the daily-SW-user parity check; live remap/scoping/F2/DnD demos deferred to FwFeatureTree_SPIKE_LIVE_CHECKLIST.md (no build tree)
- [Phase 03]: [03-03]: Rollback bar fires the REAL Body.Tip via the existing PartDesign_MoveTip command-ID under an RAII FwSelectionGuard — the command opens its OWN transaction so NO outer FreeWorks openCommand is added (one Ctrl+Z restores); no PartDesign include/link, no C++ Tip.setValue (D-04/A3/Pitfall 1, reviewer concern 5)
- [Phase 03]: [03-03]: FwSelectionGuard restores BOTH the global selection AND the preselection — both clearSelection and addSelection default clearPreSelect=true (Selection.h:360/385), so the replay passes clearPreSelect=false and the preselection is re-asserted explicitly via setPreselect (live) / rmvPreselect (none); getCompleteSelection() + getPreselection() both round-trip (reviewer HIGH-B)
- [Phase 03]: [03-03]: Bar→solid-feature snap is a PURE LINK-FREE resolver (read Group via getPropertyByName('Group') as App::PropertyLinkList; classify solidness by getTypeId().getName() type-name STRING, mirroring isSolidFeature semantics) — never the C++-only getPrevSolidFeature/isSolidFeature (reviewer concern 7); insert-at-bar reuses the Python-only Body.insertObject + explicit post-insert Tip policy (solid→becomes tip, non-solid→Tip unchanged)
- [Phase 03]: [03-03]: Below-tip greying is the Gui-only kBelowTipRole driven live by Body.Tip (painted by the 03-02 delegate hook) — NEVER a Visibility write; the bar holds NO new persisted state, Body.Tip is the single source of truth (D-05/D-06, Pitfall 3). Live FEEL items (3D suppress-below, drag/grab, fire-no-link + selection/preselection restore on hardware, SC5, A1) routed to SPIKE_LIVE_CHECKLIST.md
- [Phase 04]: [04-01]: D-03 verdict APPROVED — `reuse-and-rehost committed`: re-host the existing Tasks TaskView in the LEFT PropertyManager slot is the committed engine for 04-02/04-03/04-04; NO from-scratch task system, NO Control.cpp/TaskView.cpp body edits
- [Phase 04]: [04-01]: A1 two-branch left-placement — addDockWindow CANNOT move an already-docked panel (DockWindowManager.cpp:256-258, R2-F1); resolve host from Control().taskPanel() walk-up, branch on dockWidgetArea: re-dock-existing-left via getMainWindow()->addDockWidget(Left, dock) | create-left via addDockWindow when never-docked; getDockWindow('Tasks') is parent-independent; managed identity 'Tasks' (R2-F3)
- [Phase 04]: [04-01]: A2 dock-stays-left — the 'Tasks' dock is NOT moved right synchronously on deactivated(); the activeDialog() guard AND a pre-checked edit-in-progress flag are BOTH too late under the deactivated-before-activated-before-signalInEdit ordering (R2-F2/R4-BLOCKER)
- [Phase 04]: [04-01]: A4 overlays-survive — deferred-cancellable teardown: deactivated() SCHEDULES a QTimer::singleShot(0) teardown cancelled by signalInEdit / re-activation within the same synchronous activateWorkbench turn, so chrome + BOTH FLOW-01 halves (selection AND Features-tab-ready) survive the edit-time WB switch; Phase 2-3 ribbon shares the same synchronous-teardown pattern (cross-phase observation, NOT patched here)
- [Phase 04]: [04-01]: A3 active-box — focus-inference-first by DEFAULT (no shared-file edit) validated against the real TaskPatternParameters 2-field panel; gated // SW-FORK HOOK accessor on TaskPatternParameters.{h,cpp} only if focus inference proven insufficient (R6-MAJOR2); pink via QPalette::Midlight role populated by a now-functional FwTheme::apply() (R2-F6); spike gate cleared by documentation approval (Phase 1-3 precedent), live observations deferred to SPIKE_LIVE_CHECKLIST.md (§ Phase 4)
- [Phase 04]: [04-02]: PROP-01 core slice — FwLayout::mountPropertyManager() left-docks the managed 'Tasks' PropertyManager by the two-branch mechanism (re-dock-existing-left via getMainWindow()->addDockWidget(Left,dock) | create-left via addDockWindow when never-docked, R2-F1); resolved from Control().taskPanel() walk-up; 'Tasks' objectName preserved (R2-F3); no Control/TaskView/MainWindow body edit
- [Phase 04]: [04-02]: R3-MAJOR3 disposition (i) — STOP CONTRIBUTING Fw_PropertyManager (dropped from install() + setupDockWindows()); removeStalePropertyManagerDock() (live-dock removeDockWindow path) as defense for restored layouts — NEVER unregisterDockWindow+deleteLater
- [Phase 04]: [04-02]: A4 deferred-cancellable teardown realized via FwPropertyReveal — unmountPropertyManager() SCHEDULES a QTimer::singleShot(0) teardown guarded by a monotonic GENERATION TOKEN (singleShot returns void, so cancel()/re-schedule bump the token to no-op a queued lambda — race-free without owning the QTimer); signalInEdit cancels + re-asserts left placement; consumer owned SEPARATELY from s_ribbonContext so it survives the transient deactivated(); true-exit teardown calls disconnect() NOT reset() (runs from inside the consumer's own queued lambda)
- [Phase 04]: [04-02]: FwPropertyManagerHeader — thin 32px ✓/✗ band → Gui::Control().accept()/reject() (no new commit logic); attached as the dock's setTitleBarWidget (container-level, inner TaskView never reparented), released only by the deferred teardown so it survives the edit-time WB switch; palette-role-derived green(120deg)/red(0deg) tints off QPalette::Highlight (no hex / no setStyleSheet color literal, D-06); tooltips 'Accept (Enter)'/'Cancel (Esc)'; leak-grep clean. Live placement/WB-switch FEEL deferred to SPIKE_LIVE_CHECKLIST.md § Phase 4

### Pending Todos

[From .planning/todos/pending/ — ideas captured during sessions]

None yet.

### Blockers/Concerns

[Issues that affect future work]

- Research flags for plan-time spikes: native-vs-SARibbon ribbon spike (Phase 2), `Body.Tip` suppress-below code-read (Phase 3), `SoFCUnifiedSelection` box-select event routing (Phase 5). See `.planning/research/questions.md`.
- A daily-SolidWorks-user parity-testing track must be established in Phase 1 and run every phase — "zero relearning" can only be validated by real SW users (PITFALLS.md Pitfall 7).

## Deferred Items

Items acknowledged and carried forward from previous milestone close:

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| *(none)* | | | |

## Session Continuity

Last session: 2026-06-15
Stopped at: Phase 04 Plan 02 complete (PROP-01 core slice — left-dock + ✓/✗ header + deferred-cancellable teardown); next Plan 04-03
Resume file: .planning/phases/04-propertymanager-panel/04-03-PLAN.md

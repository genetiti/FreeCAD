---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
stopped_at: Phase 3 UI-SPEC approved — ready for /gsd-plan-phase 3
last_updated: "2026-06-14T05:19:51.035Z"
last_activity: "2026-06-14 -- Phase 03 prereqs done (CONTEXT + RESEARCH + VALIDATION + UI-SPEC); ready to plan"
progress:
  total_phases: 7
  completed_phases: 2
  total_plans: 8
  completed_plans: 8
  percent: 29
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-06)

**Core value:** A SolidWorks user can open FreeCAD and be immediately productive — it looks, navigates, and behaves like SolidWorks — with no FreeCAD tutorial required.
**Current focus:** Phase 03 — featuremanager-design-tree

## Current Position

Phase: 3 of 7 (featuremanager-design-tree) — READY TO PLAN
Plan: Not started — prereqs complete (03-CONTEXT, 03-RESEARCH, 03-VALIDATION, 03-UI-SPEC all done)
Status: Phase 03 planning prerequisites done — CONTEXT (D-01..D-13 locked), RESEARCH (HIGH confidence; Body.Tip rollback engine confirmed recompute-aware), VALIDATION (Nyquist test map), UI-SPEC (6/6 dimensions PASS). Next: /gsd-plan-phase 3 runs the planner + verify loop.
Last activity: 2026-06-14 -- Phase 03 UI-SPEC approved

Progress: [███░░░░░░░] 2 of 7 phases complete (29%)

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

Last session: 2026-06-14T05:19:51.027Z
Stopped at: Phase 3 UI-SPEC approved
Resume file: .planning/phases/03-featuremanager-design-tree/03-UI-SPEC.md

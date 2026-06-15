---
phase: 04-propertymanager-panel
plan: 01
subsystem: ui
tags: [qt6, taskview, dockwindow, propertymanager, spike, gtest, qtest, freeworks]

# Dependency graph
requires:
  - phase: 02-commandmanager-ribbon
    provides: "FwRibbonContext signalInEdit/signalResetEdit edit-lifecycle wiring + kSketchViewProviderTypeName literal; s_ribbonContext synchronous-teardown-on-deactivated pattern (the cross-phase observation)"
  - phase: 03-featuremanager-design-tree
    provides: "FwLayout mount discipline (find-or-reuse under a fixed objectName, DockWindowManager-only); FwFeatureTreeDelegate palette-derivation precedent; the Wave-0 GTest/QTEST split + ensureGuiTestBootstrap() harness"
provides:
  - "D-03 verdict: reuse-and-rehost committed — re-host the existing Tasks TaskView in the LEFT PropertyManager slot is the committed engine for 04-02/04-03/04-04"
  - "A1 two-branch left-placement mechanism (re-dock-existing-left | create-left) resolving R2-F1 (addDockWindow cannot move an already-docked panel)"
  - "A2 dock-stays-left-on-deactivated() policy (no synchronous right-move; activeDialog() guard AND a pre-checked flag both insufficient — R4-BLOCKER)"
  - "A4 overlays-survive: deferred-cancellable teardown (QTimer::singleShot(0) cancelled by signalInEdit / re-activation) — the survivable-lifecycle policy for chrome + BOTH FLOW-01 halves"
  - "A3 active-box: focus-inference-first by default (no shared-file edit) | gated // SW-FORK HOOK escalation on TaskPatternParameters.{h,cpp}; pink via QPalette::Midlight role populated by a functional FwTheme::apply()"
  - "Wave 0 test scaffold: FwPropertyManager.cpp + FwReferenceBoxStyler.cpp (Gui_tests_run) + FwPropertyManagerWidget.cpp (FwPropertyManagerWidget_Tests_run)"
affects: [04-02, 04-03, 04-04, propertymanager, taskview, freeworks-lifecycle]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Two-branch dock left-placement: resolve host from Control().taskPanel(), walk up to parent QDockWidget, branch on getMainWindow()->dockWidgetArea(dock) — re-dock existing via addDockWidget(Left, dock) vs create-left via addDockWindow when never-docked"
    - "Deferred-cancellable teardown: deactivated() SCHEDULES a QTimer::singleShot(0) teardown cancelled by signalInEdit / re-activation within the same synchronous activateWorkbench turn — survives the edit-time WB switch"
    - "Functional FwTheme palette role: install a FreeWorks-owned pink VALUE into QPalette::Midlight on the box widget's LOCAL palette; styler reads widget->palette().color(role) — never a hex / setStyleSheet literal (D-06)"

key-files:
  created:
    - "tests/src/Gui/FwPropertyManager.cpp"
    - "tests/src/Gui/FwReferenceBoxStyler.cpp"
    - "tests/src/Gui/FwPropertyManagerWidget.cpp"
    - ".planning/phases/04-propertymanager-panel/SPIKE.md"
  modified:
    - "tests/src/Gui/CMakeLists.txt"
    - "src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md"
    - ".planning/phases/04-propertymanager-panel/04-VALIDATION.md"

key-decisions:
  - "D-03 verdict: reuse-and-rehost committed — re-host the existing Tasks TaskView in the LEFT slot; NO from-scratch task system, NO Control.cpp/TaskView.cpp body edits"
  - "A1: addDockWindow CANNOT move an already-docked panel (DockWindowManager.cpp:256-258, R2-F1); left placement is a two-branch mechanism (re-dock-existing-left via getMainWindow()->addDockWidget(Left, dock) | create-left via addDockWindow when never-docked); getDockWindow('Tasks') is parent-independent; managed identity is 'Tasks' (R2-F3)"
  - "A2: the 'Tasks' dock STAYS LEFT on deactivated() (no synchronous right-move); the activeDialog() guard AND a pre-checked edit-in-progress flag are BOTH too late under the deactivated-before-activated-before-signalInEdit ordering (R2-F2/R4-BLOCKER)"
  - "A4: overlays-survive via deferred-cancellable teardown (QTimer::singleShot(0) cancelled by signalInEdit / re-activation) — chrome + BOTH FLOW-01 halves (selection AND Features-tab-ready) ride this single teardown; Phase 2-3 ribbon shares the same synchronous-teardown pattern (cross-phase observation, NOT patched here)"
  - "A3: active-box hook is focus-inference-first by DEFAULT (no shared-file edit) validated against the real TaskPatternParameters 2-field panel; gated // SW-FORK HOOK accessor on TaskPatternParameters.{h,cpp} is the conditional escalation only if focus inference proves insufficient (R2-F4/R6-MAJOR2); pink via QPalette::Midlight role populated by a now-functional FwTheme::apply() (R2-F6)"

patterns-established:
  - "Spike-gate-by-documentation-approval: PASS marks granted on [CODE]+[RESEARCH]; live observations deferred to SPIKE_LIVE_CHECKLIST.md (Phase 1-3 precedent, no build tree / GUI / CI in env)"
  - "Wave-0 RED scaffold: GTest decision-core + palette-role-read contract + QTEST construction harness wired additively, asserting only the reuse primitives that need no FreeWorks production code (production-symbol asserts left as the Wave-0 gap 04-02/04-03 close)"

requirements-completed: [PROP-01, PROP-02, TREE-03, FLOW-01]

# Metrics
duration: ~continuation
completed: 2026-06-15
---

# Phase 4 Plan 1: Wave 0 Scaffold + D-03 Container Spike Summary

**D-03 spike resolves `reuse-and-rehost committed` — re-host the existing Tasks TaskView in the LEFT PropertyManager slot — with decisive A1/A2/A3/A4 verdict tokens and a Wave-0 GTest/QTEST scaffold, approved on the documentation basis (Phase 1-3 precedent).**

## Performance

- **Duration:** continuation (Tasks 1-2 committed in a prior run; Task 3 gate finalized here)
- **Completed:** 2026-06-15
- **Tasks:** 3 (2 implementing + 1 approval gate)
- **Files modified:** 7 (4 created, 3 modified)

## Accomplishments
- Resolved the keystone D-03 foundation decision DECISIVELY: `reuse-and-rehost committed` — the existing `Gui::Control`/`Gui::TaskView::TaskView` host is placed in the LEFT slot, no new task-panel backend, no `Control.cpp`/`TaskView.cpp` body edits.
- Resolved A1 (two-branch left-placement mechanism — R2-F1: `addDockWindow` cannot move an already-docked panel), A2 (dock-stays-left on `deactivated()` — R2-F2/R4-BLOCKER), A4 (the R3-ROOT/R4-BLOCKER survivability gate — deferred-cancellable teardown surviving the edit-time WB switch for chrome + BOTH FLOW-01 halves), and A3 (focus-inference-first active-box hook + gated `// SW-FORK HOOK` escalation + functional `FwTheme::apply()` pink via `QPalette::Midlight`).
- Stood up the Wave 0 test scaffold: `FwPropertyManager.cpp` (FLOW-01 decision-core + header→accept/reject mapping + non-modal contract), `FwReferenceBoxStyler.cpp` (pink-active-box state machine + palette-role-read contract), `FwPropertyManagerWidget.cpp` (offscreen QTEST construction harness), wired additively into `tests/src/Gui/CMakeLists.txt` (`FwReferenceBoxStyler` inside `Gui_tests_run` — NO separate `*_Tests_run` target, R2-F5; `FwPropertyManagerWidget_Tests_run` created via `setup_qt_test`).
- Appended the Phase-4 live FEEL obligations to `SPIKE_LIVE_CHECKLIST.md` (incl. the A4 overlays-survive observation — BOTH FLOW-01 halves — and the synchronous-within-one-turn confirmation the `singleShot` cancellation relies on).

## Task Commits

1. **Task 1: Wave 0 test scaffold** - `fde908a662` (test) — GTest logic (FLOW-01 decision, header→accept/reject) + pink-box state machine + offscreen QTEST harness + CMake wiring; `04-VALIDATION.md` flags flipped (`wave_0_complete`/`nyquist_compliant`).
2. **Task 2: D-03 container spike** - `ba03aedf01` (docs) — `SPIKE.md` verdict (`reuse-and-rehost committed`) with A1/A2/A3/A4 decisive tokens + Phase-4 live obligations appended to `SPIKE_LIVE_CHECKLIST.md`.
3. **Task 3: Verdict approval gate** - finalized in this continuation (`checkpoint:human-verify`, gate="blocking") — APPROVED via `approved: reuse-and-rehost`; approval recorded in the SPIKE.md Sign-off block.

**Plan metadata:** tracking commit (this SUMMARY + STATE/ROADMAP/REQUIREMENTS).

## Files Created/Modified
- `tests/src/Gui/FwPropertyManager.cpp` - GTest (Gui_tests_run): FLOW-01 sketch-type-name decision-core, header→`Control().accept()`/`reject()` mapping, non-modal assertion.
- `tests/src/Gui/FwReferenceBoxStyler.cpp` - GTest (Gui_tests_run, no separate target — R2-F5): pink active-box state machine + palette-role-read-off-the-box-widget contract (R2-F6).
- `tests/src/Gui/FwPropertyManagerWidget.cpp` - QTEST_MAIN (offscreen, `FwPropertyManagerWidget_Tests_run`): real-QApplication bring-up + resolvable registered Tasks TaskView.
- `tests/src/Gui/CMakeLists.txt` - Additive wiring (two filenames into `Gui_tests_run`; one `setup_qt_test(FwPropertyManagerWidget)`).
- `.planning/phases/04-propertymanager-panel/SPIKE.md` - D-03 verdict document + Sign-off with the recorded approval.
- `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` - Appended `## Phase 4` live FEEL obligations.
- `.planning/phases/04-propertymanager-panel/04-VALIDATION.md` - `wave_0_complete: true` / `nyquist_compliant: true`.

## Decisions Made
- See `key-decisions` frontmatter. The committed engine for downstream plans: re-host the Tasks TaskView LEFT (two-branch placement), keep the dock left on `deactivated()`, use a deferred-cancellable `QTimer::singleShot(0)` teardown so chrome + both FLOW-01 halves survive the edit-time WB switch, infer the active reference box from focus by default (gated `// SW-FORK HOOK` escalation only if proven insufficient), and source the pink active-box tone from a `QPalette::Midlight` role populated by a now-functional `FwTheme::apply()`.

## Deviations from Plan

None - plan executed exactly as written. Task 3 is the approval gate; it was approved on the documentation basis (`approved: reuse-and-rehost`), consistent with the Phase 1-3 precedent (no build tree / GUI / CI in this environment).

## Issues Encountered
None. Tasks 1 & 2 were verified present (commits `fde908a662`, `ba03aedf01`) and not re-executed; the continuation only finalized the approved gate and the tracking artifacts.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- **Plan 04-02 (gated, now unblocked):** implement `FwLayout::mountPropertyManager()`/`unmountPropertyManager()` (two-branch left placement, idempotent, placeholder-consuming, `Control().taskPanel()` walk-up), `FwPropertyManagerHeader` (green-✓/red-✗ → `accept()`/`reject()`), and the `signalInEdit` reveal/re-assert + teardown-cancel consumer owning the pending `QTimer` handle.
- **Plan 04-03:** `FwReferenceBoxStyler.{h,cpp}` (read `QPalette::Midlight`, focus-inference-first; gated hook only if escalated) + `FwPropertyKeyFilter.{h,cpp}` (Enter/Esc/Tab) — released only by the A4 deferred-cancellable teardown.
- **Plan 04-04:** the FLOW-01 sketch-exit handler on the SURVIVABLE `s_propertyReveal` consumer (BOTH halves — selection + Features-tab-ready) + the TREE-03 double-click→left-panel integration confirmation.
- **Open obligations (live, non-gating):** the live placement round-trip, the WB-switch dock observation, the A4 overlays-survive observation (both FLOW-01 halves), and the synchronous-within-one-turn confirmation remain in `SPIKE_LIVE_CHECKLIST.md` (`## Phase 4`).
- **Cross-phase observation (flagged, NOT patched):** the Phase 2-3 ribbon `s_ribbonContext` uses the same synchronous-teardown-on-deactivated pattern and likely vanishes during edits too — a follow-up, never patched here.

## Self-Check: PASSED

- Commits verified present: `fde908a662` (test), `ba03aedf01` (docs) — both found via `git log --grep="04-01"` with files on disk.
- Created files verified present: `tests/src/Gui/FwPropertyManager.cpp`, `tests/src/Gui/FwReferenceBoxStyler.cpp`, `tests/src/Gui/FwPropertyManagerWidget.cpp`, `.planning/phases/04-propertymanager-panel/SPIKE.md`.
- `SPIKE.md` carries the literal verdict token `reuse-and-rehost committed` and the A4 `overlays-survive: deferred-cancellable teardown` token; Sign-off records the `approved: reuse-and-rehost` gate clearance.
- CMake wiring confirmed: `FwPropertyManager.cpp` + `FwReferenceBoxStyler.cpp` in `Gui_tests_run`; `setup_qt_test(FwPropertyManagerWidget)` present; NO `FwReferenceBoxStyler_Tests_run` target (R2-F5).
- `tools/fw-string-leak-grep.sh` clean over new source/tests.

---
*Phase: 04-propertymanager-panel*
*Completed: 2026-06-15*

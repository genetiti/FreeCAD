---
phase: 04-propertymanager-panel
plan: 02
subsystem: ui
tags: [qt6, taskview, dockwindow, propertymanager, control, fastsignals, qtest, freeworks, deferred-teardown]

# Dependency graph
requires:
  - phase: 04-propertymanager-panel
    plan: 01
    provides: "D-03 reuse-and-rehost engine + A1 two-branch left-placement + A2 dock-stays-left + A4 deferred-cancellable teardown; Wave-0 FwPropertyManagerWidget QTEST scaffold"
  - phase: 02-commandmanager-ribbon
    provides: "FwRibbonContext fastsignals scoped_connection edit-signal discipline; s_ribbonContext synchronous-teardown-on-deactivated cross-phase observation"
  - phase: 03-featuremanager-design-tree
    provides: "FwLayout mount discipline (find-or-reuse, DockWindowManager-only); FwFeatureTreeDelegate palette-derivation precedent; QTEST split + ensureGuiTestBootstrap harness"
provides:
  - "FwLayout::mountPropertyManager()/unmountPropertyManager(): two-branch left placement of the managed 'Tasks' PropertyManager dock (re-dock-existing-left | create-left, R2-F1) + A4 deferred-cancellable teardown"
  - "FwPropertyReveal: survivable reveal/teardown consumer (owned separately from s_ribbonContext) — signalInEdit cancels the pending QTimer::singleShot(0) teardown + re-asserts left placement; generation-token cancellation"
  - "FwPropertyManagerHeader: thin 32px green-check/red-cross band driving Gui::Control().accept()/reject() (no new commit logic); palette-role-derived tints"
  - "R3-MAJOR3 disposition (i): Fw_PropertyManager is no longer contributed (install()/setupDockWindows()); removeStalePropertyManagerDock() defends restored layouts via the live-dock removeDockWindow path"
affects: [04-03, 04-04, propertymanager, taskview, freeworks-lifecycle]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Two-branch dock left-placement: resolve host from Control().taskPanel() walk-up to parent QDockWidget, branch on getMainWindow()->dockWidgetArea(dock) — re-dock existing via addDockWidget(Left, dock) vs create-left via addDockWindow when never-docked (R2-F1)"
    - "Deferred-cancellable teardown via generation token: unmount SCHEDULES QTimer::singleShot(0) guarded by a monotonic token; cancel()/re-schedule bump the token so a queued lambda becomes a no-op — race-free cancellation without owning/stopping the QTimer object"
    - "Survivable signal consumer owned SEPARATELY from s_ribbonContext (which is reset on the transient deactivated()) so chrome + placement survive the edit-time WB switch (A4)"
    - "Container-level chrome attach: header as QDockWidget::setTitleBarWidget() — never reparents the inner TaskView (D-01/D-02)"
    - "Palette-role-derived functional accent: QColor::fromHsv(functionalHue, S/V from QPalette::Highlight) — no hex / no setStyleSheet color literal (D-06)"

key-files:
  created:
    - "src/Gui/FreeWorks/FwPropertyManagerHeader.h"
    - "src/Gui/FreeWorks/FwPropertyManagerHeader.cpp"
    - "src/Gui/FreeWorks/FwPropertyReveal.h"
    - "src/Gui/FreeWorks/FwPropertyReveal.cpp"
  modified:
    - "src/Gui/FreeWorks/FwLayout.h"
    - "src/Gui/FreeWorks/FwLayout.cpp"
    - "src/Gui/FreeWorks/FwWorkbench.cpp"
    - "src/Gui/FreeWorks/CMakeLists.txt"
    - "tests/src/Gui/FwPropertyManagerWidget.cpp"

key-decisions:
  - "R3-MAJOR3 disposition (i) — STOP CONTRIBUTING Fw_PropertyManager: removed the placeholder registration from FwLayout::install() AND the addDockWidget('Fw_PropertyManager', ...) from FwWorkbench::setupDockWindows(), so DockWindowManager::setup() never creates a second live left dock. removeStalePropertyManagerDock() (live-dock removeDockWindow path) is kept as defense-in-depth for a restored layout — NEVER unregisterDockWindow+deleteLater (R3-MAJOR3)."
  - "Cancellation is a generation-token guard, not QTimer ownership: QTimer::singleShot returns void, so FwPropertyReveal captures a monotonic token in the queued lambda; cancel()/schedule() bump pendingGeneration_ so a stale queued teardown is a no-op. Race-free without an owned restartable QTimer."
  - "True-exit teardown calls s_propertyReveal->disconnect() (drop subscriptions) NOT s_propertyReveal.reset(): the teardown runs FROM INSIDE the consumer's own queued lambda, so resetting the unique_ptr would destroy the object mid-stack. The consumer object persists for FreeWorks-mode lifetime and is re-connected by a later mount."
  - "Header attached as the dock's setTitleBarWidget() (container-level band), idempotent — never reparents the inner 'Tasks' TaskView; released only by the deferred teardown so it survives the edit-time WB switch (R3-ROOT)."
  - "Header tint is palette-role-derived (QColor::fromHsv with green=120deg/red=0deg hue + S/V from QPalette::Highlight) — no hex, no setStyleSheet color literal (D-06); tooltips 'Accept (Enter)'/'Cancel (Esc)'; no trademark string (leak-grep clean)."

patterns-established:
  - "Generation-token cancellable deferred teardown (FwPropertyReveal): the reusable mechanism Plans 04-03/04-04 install their chrome (key filter, reference-box styler) and the FLOW-01 sketch-exit handler onto."
  - "Compile-intended TDD under a no-build-tree env: RED test references the not-yet-existing production symbol (compile-fail RED), GREEN implements it; live placement/WB-switch FEEL deferred to SPIKE_LIVE_CHECKLIST.md § Phase 4 (Phase 1-3 precedent)."

requirements-completed: [PROP-01]

# Metrics
duration: ~12min
completed: 2026-06-15
---

# Phase 4 Plan 2: PROP-01 Core Vertical Slice Summary

**Left-docks the existing "Tasks" TaskView as the PropertyManager via a two-branch placement (R2-F1), adds a green-✓/red-✗ header driving the existing Control().accept()/reject(), and makes the chrome SURVIVE the edit-time workbench switch through a generation-token deferred-cancellable teardown cancelled by signalInEdit.**

## Performance

- **Duration:** ~12 min
- **Started:** 2026-06-15T02:20:47Z
- **Completed:** 2026-06-15T02:32:12Z
- **Tasks:** 2 (both TDD: RED → GREEN)
- **Files modified:** 9 (4 created, 5 modified)

## Accomplishments
- `FwLayout::mountPropertyManager()` places the managed "Tasks" PropertyManager dock LEFT by the CORRECT mechanism (R2-F1): resolve the host from `Gui::Control().taskPanel()`, walk UP to its parent `QDockWidget`, branch on `getMainWindow()->dockWidgetArea(dock)` — re-dock an existing non-left container left via `getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, dock)` (came-from-right-dock), or create-left via `addDockWindow("Tasks", taskView, Left)` only when never-docked. Idempotent (already-left is a no-op); the "Tasks" container + inner-widget objectNames are preserved (R2-F3). Never a hardcoded container-name guess.
- `Fw_PropertyManager` is removed CORRECTLY for its post-setup state (R3-MAJOR3): disposition (i) — stop contributing it (dropped from `install()` and `setupDockWindows()`), plus `removeStalePropertyManagerDock()` (live-dock `removeDockWindow` path) as defense for a restored layout. NOT the pre-setup `unregisterDockWindow+deleteLater`.
- A4 deferred-cancellable teardown: `unmountPropertyManager()` SCHEDULES a cancellable `QTimer::singleShot(0)` teardown through the new `FwPropertyReveal` consumer rather than tearing down inline. `signalInEdit` (fired after `startEditing` but within the same synchronous `activateWorkbench` turn as the transient `deactivated()`) CANCELS the pending teardown and re-asserts the idempotent left placement — so the "Tasks" dock is never moved back right (R2-F2) and the chrome is never stripped mid-edit (R3-ROOT/R3-BLOCKER2). The consumer is owned SEPARATELY from `s_ribbonContext` (which is reset on the same transient `deactivated()`) so it survives.
- `FwPropertyManagerHeader`: a thin 32px green-✓/red-✗ band whose ✓ calls `Gui::Control().accept()` and ✗ calls `Gui::Control().reject()` — the SAME accept/reject the hosted `TaskEditControl` `QDialogButtonBox` drives — with NO new commit logic. Palette-role-derived tints (no hex / no `setStyleSheet` color literal), tooltips "Accept (Enter)"/"Cancel (Esc)", no trademark string. Attached as the dock's title-bar band (container-level), never reparenting the inner TaskView, released only by the deferred teardown.
- Extended the offscreen QTEST: never-docked create-left AND came-from-right-dock re-dock-left, no `Fw_PropertyManager` dock remains, saveState round-trip on the "Tasks" identity, the REAL transient order (unmount schedules → signalInEdit/re-mount cancels → drain → dock+header survive), the genuine-exit case (unmount → drain → teardown ran), header controls/tooltips/32px, and accept/reject invocation.

## Task Commits

Each task was committed atomically (TDD RED → GREEN):

1. **Task 1: mountPropertyManager()/unmountPropertyManager() + lifecycle** — `48a827e129` (test, RED) → `fdf762f5da` (feat, GREEN)
2. **Task 2: FwPropertyManagerHeader + header-attachment + QTEST** — `87791893a7` (test, RED) → `4f45c753a9` (feat, GREEN)

**Plan metadata:** tracking commit (this SUMMARY + STATE/ROADMAP/REQUIREMENTS).

_All four task commits verified present; no REFACTOR commits were needed._

## Files Created/Modified
- `src/Gui/FreeWorks/FwPropertyManagerHeader.{h,cpp}` (new) — thin 32px ✓/✗ band → `Gui::Control().accept()`/`reject()`; palette-role-derived tints; tooltips; no trademark string.
- `src/Gui/FreeWorks/FwPropertyReveal.{h,cpp}` (new) — survivable reveal/deferred-cancellable-teardown consumer: two `fastsignals::scoped_connection`s (signalInEdit cancel+re-assert, signalResetEdit reserved for 04-04) + generation-token `schedule()`/`cancel()`.
- `src/Gui/FreeWorks/FwLayout.{h,cpp}` — `mountPropertyManager()`/`unmountPropertyManager()`, `placeTasksDockLeft()`/`removeStalePropertyManagerDock()`/`attachPropertyManagerHeader()` helpers, `s_propertyReveal` static (separate from `s_ribbonContext`); `install()` stops registering `Fw_PropertyManager`.
- `src/Gui/FreeWorks/FwWorkbench.cpp` — `activated()` calls `mountPropertyManager()` after `mountRibbon()`; `deactivated()` calls `unmountPropertyManager()` (schedules, never inline); `setupDockWindows()` no longer contributes `Fw_PropertyManager`.
- `src/Gui/FreeWorks/CMakeLists.txt` — additive: `FwPropertyReveal.{cpp,h}` + `FwPropertyManagerHeader.{cpp,h}` into `FreeWorks_CPP_SRCS`/`FreeWorks_HPP_SRCS`.
- `tests/src/Gui/FwPropertyManagerWidget.cpp` — extended with the placement/teardown/header assertions.

## Decisions Made
See `key-decisions` frontmatter. Headlines: R3-MAJOR3 disposition (i) (stop contributing `Fw_PropertyManager`, with live-dock removal as defense); cancellation via a monotonic generation token rather than QTimer ownership (singleShot returns void); the true-exit teardown calls `disconnect()` not `reset()` because it runs from inside the consumer's own queued lambda; header attached as `setTitleBarWidget()` (container-level, no TaskView reparent); palette-role-derived green/red tints.

## Deviations from Plan

None — plan executed exactly as written. Two small, in-scope authoring choices the plan explicitly delegated:
- The plan offered R3-MAJOR3 disposition (i) OR (ii); I chose (i) (stop contributing) as the cleaner primary, keeping the (ii) live-dock `removeDockWindow` path as defense-in-depth for restored layouts. Both are sanctioned by the plan.
- The plan left the cancellable-teardown realization open ("an owned restartable single-shot QTimer OR a pending-generation counter / QPointer-guarded lambda"); I used the generation-counter realization, which the plan names.

## Issues Encountered
- The leak-grep flagged my own Doxygen comment containing the bare word "SolidWorks" (project convention forbids the bare token even in comments — use "reference-CAD"/"trademark"). Reworded to "trademark identifier" → leak-grep clean. Same class of fix for a `getDockContainer("Std_TaskView")` literal appearing in a comment that tripped the `<automated>` negative-grep — reworded the comment.
- No build tree / GUI / CI in this environment (Phase 1-3 precedent): the QTEST and production are authored compile-intended; the live left-placement, came-from-right-dock re-dock, and the assureWorkbench WB-switch survival (panel stays left WITH chrome, edit lands left) are deferred to `SPIKE_LIVE_CHECKLIST.md` § Phase 4. The RED→GREEN commit sequence is still enforced.

## TDD Gate Compliance
Both behavior-adding tasks satisfied the MVP+TDD blocking gate: a `test(04-02-NN):` RED commit precedes each `feat(04-02-NN):` GREEN commit (`48a827e129`→`fdf762f5da`, `87791893a7`→`4f45c753a9`). Tests could not be executed live (no build tree); the failing-test-first SEQUENCE was honored per the documented Phase 1-3 precedent.

## User Setup Required
None — no external service configuration required.

## Next Phase Readiness
- **Plan 04-03:** `FwReferenceBoxStyler.{h,cpp}` (pink active-box via `QPalette::Midlight`) + `FwPropertyKeyFilter.{h,cpp}` (Tab + Enter/Esc) — install on the left-docked "Tasks" host this plan establishes; release them ONLY on the `FwPropertyReveal` deferred true-exit teardown (NOT the transient `deactivated()`).
- **Plan 04-04:** the FLOW-01 sketch-exit handler hangs off `FwPropertyReveal`'s `signalResetEdit` connection (reserved/empty in 04-02) — NOT `s_ribbonContext`; TREE-03 double-click→left-panel confirmation rides this plan's left-docked Control panel + survivable lifecycle.
- **Open obligations (live, non-gating):** live left-placement round-trip, the WB-switch dock+chrome survival observation, and the A4 overlays-survive observation remain in `SPIKE_LIVE_CHECKLIST.md` § Phase 4.

## Threat Flags
None — no new security-relevant surface beyond the plan's `<threat_model>` (desktop GUI chrome; the header calls only existing Control slots; no network/auth/PII/parsing).

## Self-Check: PASSED
- Created files verified present on disk: `FwPropertyManagerHeader.{h,cpp}`, `FwPropertyReveal.{h,cpp}`.
- Commits verified present in git log: `48a827e129` (test), `fdf762f5da` (feat), `87791893a7` (test), `4f45c753a9` (feat).
- No `Control.cpp` / `TaskView.cpp` / `MainWindow.cpp` body edit across the plan (verified via `git diff --name-only`).
- `tools/fw-string-leak-grep.sh` clean; no hex / `setStyleSheet` color literal in the header; all new lines ≤100 cols.
- `MOUNT-WIRED-OK` and `HEADER-OK` printed by the per-task `<automated>` grep verification blocks.

---
*Phase: 04-propertymanager-panel*
*Completed: 2026-06-15*

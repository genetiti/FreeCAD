---
phase: 04
slug: propertymanager-panel
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-06-14
---

# Phase 04 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `04-RESEARCH.md` § Validation Architecture (`nyquist_validation: true`, `tdd_mode: true`).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Google Test (`Gui_tests_run`, logic) + Qt Test (`setup_qt_test`, offscreen widgets) — GTest 1.x, QTEST_MAIN |
| **Config file** | `tests/src/Gui/CMakeLists.txt` (add new `.cpp` to `Gui_tests_run` and `setup_qt_test(...)`) |
| **Quick run command** | `ctest -R "FwPropertyManager" --output-on-failure` |
| **Full suite command** | `ctest -R "Fw|Gui_tests_run" --output-on-failure` |
| **Estimated runtime** | ~30–90 seconds (offscreen Qt widget bring-up dominates) |

Bootstrap: `tests::ensureGuiTestBootstrap()` (FwTestGuiBootstrap.h) brings up App + offscreen `QApplication` + `Gui::Application(false)` + imports PartDesignGui/SketcherGui — required before any Control/TaskView/Selection assertion.

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "FwPropertyManager" --output-on-failure` + `tools/fw-string-leak-grep.sh`
- **After every plan wave:** Run `ctest -R "Fw|Gui_tests_run" --output-on-failure`
- **Before `/gsd-verify-work`:** Full FreeWorks suite green + leak-grep clean; live FEEL items recorded in `SPIKE_LIVE_CHECKLIST.md` (not gating code completion, per Phase 1–3 precedent)
- **Max feedback latency:** ~90 seconds (full suite)

---

## Per-Task Verification Map

> Task IDs are assigned during planning; rows below map each phase requirement / decision to its automated proof. The planner refines `Task ID` / `Plan` / `Wave` columns into PLAN.md `<automated>` verify blocks.

| Requirement | Behavior | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|-------------|----------|------------|-----------------|-----------|-------------------|-------------|--------|
| PROP-01 | After `mountPropertyManager()`, Tasks dock is in LEFT area and `Control().taskPanel()` is non-null | — | N/A | widget (QTEST offscreen) | `ctest -R FwPropertyManagerWidget` | ❌ W0 | ⬜ pending |
| PROP-01 | Header green-✓ → `Control().accept()` invoked; red-✗ → `reject()` (stub TaskDialog) | — | N/A | widget | `ctest -R FwPropertyManagerWidget` | ❌ W0 | ⬜ pending |
| PROP-01 | Rollout `TaskBox`/`TaskGroup` remain collapsible after re-host (widget type present + `isExpandable`) | — | N/A | widget | `ctest -R FwPropertyManagerWidget` | ❌ W0 | ⬜ pending |
| PROP-01 | Panel non-modal: re-host never calls `exec()`; `Gui::Selection` mutable while panel shown | — | N/A | logic | `ctest -R FwPropertyManager` | ❌ W0 | ⬜ pending |
| PROP-02 | Reference-box styler marks exactly one box active (pink palette role), reverts on deactivate — pure state machine | — | N/A | logic | `ctest -R FwReferenceBoxStyler` | ❌ W0 | ⬜ pending |
| PROP-02 | Active tone resolves via a `QPalette` role, NOT a hex literal (no `setStyleSheet` color literal; leak-grep clean) | — | N/A | logic + grep | `tools/fw-string-leak-grep.sh` + `ctest -R FwReferenceBoxStyler` | ❌ W0 | ⬜ pending |
| TREE-03 | `FwFeatureTree` double-click routes to `setEdit` → `Control().showDialog` lands in left-hosted panel | — | N/A | widget/integration | `ctest -R FwFeatureTreeWidget` (extend) | ⚠️ partial | ⬜ pending |
| FLOW-01 | On `signalResetEdit` for a sketch VP, finished sketch becomes current `Gui::Selection`; no feature command auto-launched | — | N/A | logic | `ctest -R FwPropertyManager` | ❌ W0 | ⬜ pending |
| FLOW-01 | Sketch-exit lands the ribbon on the Features tab (reuse `FwRibbonContext` decision-core pattern) | — | N/A | logic | `ctest -R FwRibbon` (extend) | ⚠️ partial | ⬜ pending |
| D-10 | Enter→accept / Esc→reject reach hosted button box; Tab advances between reference boxes (synthetic panel) | — | N/A | widget | `ctest -R FwPropertyManagerWidget` | ❌ W0 | ⬜ pending |
| Layout | Moved Tasks dock round-trips `saveState`/`restoreState` with objectNames preserved | — | N/A | widget | `ctest -R FwPropertyManagerWidget` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/src/Gui/FwPropertyManager.cpp` — GTest logic (header→accept/reject mapping, FLOW-01 decision, non-modal assertion). Covers PROP-01, FLOW-01, D-10 logic.
- [ ] `tests/src/Gui/FwPropertyManagerWidget.cpp` — QTEST offscreen (left re-host, dock area, rollouts intact, accept/reject fire, `saveState` round-trip, Tab traversal). Covers PROP-01, TREE-03, D-10, layout.
- [ ] `tests/src/Gui/FwReferenceBoxStyler.cpp` — pink active-state state machine + palette-role assertion. Covers PROP-02.
- [ ] Wire all three into `tests/src/Gui/CMakeLists.txt` (`Gui_tests_run` + `setup_qt_test`).
- [ ] D-03 SPIKE harness (throwaway) + `SPIKE.md` verdict before the full build (resolves A1/A2/A3 Open Questions). Mirror `02-commandmanager-ribbon/SPIKE.md`.
- Framework install: none — GTest + Qt Test already configured.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Panel visually slides in on the LEFT with correct chrome (green-✓/red-✗, pink active box) | PROP-01, PROP-02 | Pixel/animation FEEL — offscreen widget tests assert structure/area, not perceived slide-in | Run a Pad/Sketch command; confirm panel appears left, header colors correct, active reference box renders pink. Record in `SPIKE_LIVE_CHECKLIST.md`. |
| Double-click tree feature opens edit in the LEFT panel live | TREE-03 | End-to-end VP `setEdit` round-trip with real document; widget test covers routing, not live visual landing | Double-click an existing Pad in the tree; confirm its panel opens left and edits commit cleanly. |
| Sketch→feature loop FEEL (pick plane → Sketch → draw → exit → profile auto-selected → Features tab ready) | FLOW-01 | Full interactive modeling loop across sketcher + ribbon | Walk the loop manually; confirm finished sketch is pre-selected and Features tab is active. |
| A2: `assureWorkbench("PartDesignWorkbench")` does not tear down FreeWorks chrome mid-edit | TREE-03 | Requires live workbench switch observation | D-03 SPIKE verdict; record in `SPIKE.md`. |

*Live FEEL items are recorded, not code-completion gating, per Phase 1–3 precedent.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING (❌ W0) references
- [ ] No watch-mode flags
- [ ] Feedback latency < 90s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

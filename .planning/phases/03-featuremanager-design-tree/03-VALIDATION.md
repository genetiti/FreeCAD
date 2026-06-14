---
phase: 3
slug: featuremanager-design-tree
status: draft
nyquist_compliant: true
wave_0_complete: false  # tests authored, not run locally (no build tree); pending CI
created: 2026-06-14
---

# Phase 3 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Source: `03-RESEARCH.md` § Validation Architecture. No configured build tree in this
> environment (Phase 1/2 precedent) — tests are authored compile-intended / CI-green;
> live-only items go to a SPIKE_LIVE_CHECKLIST per the verification-deferral pattern.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GoogleTest (`Gui_tests_run`) for pure-logic + QTest (`QTEST_MAIN`, offscreen) for widget construction |
| **Config file** | `tests/src/Gui/CMakeLists.txt` (add `FwFeatureTree.cpp` to `Gui_tests_run`; `setup_qt_test(FwFeatureTreeWidget)`) — [VERIFIED CMakeLists.txt:7,23] |
| **Quick run command** | `ctest -R "Gui_tests_run"` |
| **Full suite command** | `ctest -R "Gui_tests_run\|FwFeatureTreeWidget_Tests_run"` |
| **Estimated runtime** | ~CI-bound (no local build tree; rides the CI matrix) |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "Gui_tests_run"` (CI / when a build tree exists)
- **After every plan wave:** Run `ctest -R "Gui_tests_run\|FwFeatureTreeWidget_Tests_run"` + `tools/fw-string-leak-grep.sh`
- **Before `/gsd-verify-work`:** Full suite green + leak-grep clean + SPIKE_LIVE_CHECKLIST obligations recorded
- **Max feedback latency:** CI-bound (headless asserts authored compile-intended)

---

## Per-Task Verification Map

| Requirement | Behavior | Test Type | Automated Command | File Exists | Status |
|-------------|----------|-----------|-------------------|-------------|--------|
| TREE-01 | Tree mounts in `Fw_FeatureManager`; structure = Origin-first + creation order + nested sketches | QTest (offscreen) build + structure assert | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ W0 | ⬜ pending |
| TREE-01 | Plane label remap XY→"Front Plane" etc. (delegate display text) | QTest (offscreen) | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ W0 | ⬜ pending |
| TREE-01 | Active-Body scoping resolves `"PartDesign::Body"` type-name (no module link) | GTest (logic) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-02 | Set-tip-to-earlier-feature → `Tip.isTouched()` → `mustExecute()==1` | GTest (logic) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-02 | Insert-at-bar via `Body::insertObject(target, after)` places feature in Group | GTest (logic) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-02 | "Roll to End" sets Tip to last solid feature; reversible | GTest (logic) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-02 | 3D suppress-below + drag *feel* (looks/feels like SW) | **manual / live-only** | SPIKE_LIVE_CHECKLIST + parity-user track | n/a (deferred) | ⬜ deferred |
| TREE-04 | Valid reorder commits via Group `PropertyLinkList` in one transaction; clean undo | GTest (logic) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-04 | Invalid reorder (child before parent) BLOCKED by `canDragObjectToTarget`/`canDropObjectEx` (no commit) | GTest (logic, assert gate false) | `ctest -R Gui_tests_run` | ❌ W0 | ⬜ pending |
| TREE-04 | Insertion-line affordance + forbidden cursor on invalid drop | **manual / live-only** | SPIKE_LIVE_CHECKLIST | n/a (deferred) | ⬜ deferred |
| TREE-03 (F2 half) | F2 → editItem → transactioned `Label.setValue`; action installed on FwFeatureTree | QTest (offscreen) assert action + shortcut | `ctest -R FwFeatureTreeWidget_Tests_run` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky · (TREE-03 double-click→PropertyManager is Phase 4; only the F2 rename half is in Phase 3 scope.)*

---

## Wave 0 Requirements

- [ ] `tests/src/Gui/FwFeatureTree.cpp` — GTest: active-Body type-name scoping, Tip-move/`mustExecute` logic, `insertObject` placement, reorder+undo, DnD gate BLOCK (TREE-01 scoping, TREE-02, TREE-04 logic)
- [ ] `tests/src/Gui/FwFeatureTreeWidget.cpp` — QTEST_MAIN offscreen: tree construction, structure (Origin-first / creation-order / nested sketches), delegate label remap, F2 action present (TREE-01 structure, F2)
- [ ] `tests/src/Gui/FwTestGuiBootstrap.h` — reuse existing bootstrap (creates `Gui::Application` then loads PartDesignGui/SketcherGui so Body/feature types + commands resolve) — [VERIFIED present from Phase 2]
- [ ] CMake wiring: add both files to `tests/src/Gui/CMakeLists.txt` (`Gui_tests_run` list + `setup_qt_test(FwFeatureTreeWidget)`)

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Rollback bar drag *feels* like SW; 3D suppress-below renders | TREE-02 / SC5 | Live GUI + 3D viewport + real Body; pure-logic tests cover Tip/recompute only | SPIKE_LIVE_CHECKLIST: drag bar up → features below greyed + 3D rolls back; drag down → restores |
| Insertion-line affordance + forbidden-cursor on invalid drop | TREE-04 | Live pointer + drag event loop | SPIKE_LIVE_CHECKLIST: drag a feature before its parent → insertion line + no-drop cursor, drop refused |
| Daily-SW-user acceptance (rollback suppress-below + insert-at-bar) | SC5 | Subjective parity judgement | parity-user track (PITFALLS Pitfall 7) |

---

## Validation Sign-Off

- [ ] All tasks have automated verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Live-only items recorded in SPIKE_LIVE_CHECKLIST before phase gate
- [ ] `nyquist_compliant: true` set in frontmatter (at plan time, once tasks map cleanly)

**Approval:** pending

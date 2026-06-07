---
phase: 2
slug: commandmanager-ribbon
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-07
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GoogleTest (GTest) + ctest — mirrors Phase 1's headless `Gui_tests_run` |
| **Config file** | `tests/src/Gui/CMakeLists.txt` (FwRibbon test wired into the `Gui_tests_run` target) |
| **Quick run command** | `ctest -R Gui_tests_run --output-on-failure` |
| **Full suite command** | `ctest --output-on-failure` |
| **Estimated runtime** | ~60 seconds (Gui_tests_run subset); full suite varies by build |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R Gui_tests_run --output-on-failure`
- **After every plan wave:** Run `ctest --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-T1 (Wave 0 GTest scaffold) | 02-01 | 1 | RIBBON-01 | T-02-01/02 | Additive test-list edit; no "SolidWorks" string | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ❌ W0 (this task creates it) | ⬜ pending |
| 02-01-T2 (minimal FwRibbon: command→button + tab build) | 02-01 | 1 | RIBBON-01 | T-02-03/04 | No App includes; getCommandByName null-checked (D-08) | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ❌ W0 | ⬜ pending |
| 02-01-T3 (spike gate: D-03 parity verdict) | 02-01 | 1 | RIBBON-01 | T-02-SC | SARibbon vendor-vet gate only if spike FAILS (blocking) | checkpoint:human-verify | `<human-check>` SPIKE.md 6× PASS/FAIL + Verdict; `grep -c "PASS\|FAIL" SPIKE.md >= 6` | n/a (planning doc) | ⬜ pending |
| 02-02-T1 (curated FwRibbonMap of pinned IDs) | 02-02 | 2 | RIBBON-01 | T-02-01/04 | Pinned developer-constant IDs; per-row resolution test | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ (built in 02-01-T1) | ⬜ pending |
| 02-02-T2 (full curated/auto-derive build + flyouts) | 02-02 | 2 | RIBBON-01 | T-02-03 | No Mod/Sketcher include; no setStyleSheet/hex | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ | ⬜ pending |
| 02-03-T1 (mount in TopToolBarArea + reversible chrome hide/restore) | 02-03 | 3 | RIBBON-01 | T-02-02/03/06 | Public Gui singletons only; no MainWindow.cpp edit; reversible | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ | ⬜ pending |
| 02-03-T2 (D-12 overflow escape hatch + D-14 persistence round-trip) | 02-03 | 3 | RIBBON-01 | T-02-05/07 | Unique Fw_* objectNames; Qt-native saveState/restoreState | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ | ⬜ pending |
| 02-04-T1 (FwRibbonContext: edit-signal switch logic) | 02-04 | 4 | RIBBON-02 | T-02-08/10 | Type-name string, no Sketcher include; no activeDialog polling | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ | ⬜ pending |
| 02-04-T2 (bind FwRibbonContext to mounted ribbon via FwLayout) | 02-04 | 4 | RIBBON-02 | T-02-09 | Scoped fastsignals connections; one context per ribbon | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

> All code-producing tasks carry an `<automated>` verify (`ctest -R Gui_tests_run`); the lone checkpoint
> (02-01-T3) is the spike gate whose acceptance is asserted by `grep -c "PASS\|FAIL" SPIKE.md >= 6` plus the
> explicit native-vs-SARibbon Verdict line. GUI-only fidelity (visual SW-likeness, flyout hover, live
> menu-bar round-trip, true-restart persistence) falls to the Manual-Only table below.

---

## Wave 0 Requirements

- [ ] `tests/src/Gui/FwRibbon.cpp` — headless GTest scaffold (created by Plan 02-01 Task 1) covering RIBBON-01 (curated-ID resolution + headless tab build) and growing through later plans to RIBBON-02 (context switch), SC5/D-14 (persistence round-trip), and D-11 (chrome hide/restore reachable assertions)
- [ ] Wire `tests/src/Gui/FwRibbon.cpp` into the existing `Gui_tests_run` target in `tests/src/Gui/CMakeLists.txt` (mirror Phase 1's `FwWorkbench.cpp` wiring; GTest framework already present, no new link)
- [ ] Tri-OS manual checklist doc (mirror Phase 1 `TRIOS_LAUNCH_CHECKLIST.md`) for the GUI-only fidelity items in the Manual-Only table

> `wave_0_complete` flips to `true` once Plan 02-01 Task 1 lands `tests/src/Gui/FwRibbon.cpp` wired into `Gui_tests_run`.

*If none: "Existing infrastructure covers all phase requirements."*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ribbon reads as "this is SolidWorks" — large labeled icons, tab strip, SW-like layout | RIBBON-01 / SC1 | Visual fidelity is subjective; no headless assertion captures "looks SW-like" | Launch FreeWorks mode tri-OS; confirm tabbed ribbon in top area with large icon-over-label buttons replacing menus+toolbars |
| Flyout split-button reveals grouped commands on arrow click | RIBBON-01 / SC3 | Native `QToolButton(MenuButtonPopup)` popup interaction needs a live event loop/pointer | Click a split-button (e.g. Fillet ▸) and confirm Chamfer/related commands appear and fire |
| Menu bar + toolbars hidden in FreeWorks mode and cleanly restored on deactivation | SC1 / D-11 | Round-trip of `ForceHidden` chrome state across a live workbench toggle needs interactive verification | Toggle in/out of FreeWorks mode; confirm stock chrome hides then reappears intact (the headless 02-03-T1 toggle test covers the reachable assertions; this confirms the live menu bar) |
| Layout persists across an actual app restart | RIBBON-01 / SC5 / D-14 | True restart persistence exercises real config write/read, not an in-process round-trip (02-03-T2 covers the in-process blob) | Set a tab, restart app, confirm tab/panel state restores |
| Live sketch entry switches the active ribbon tab to Sketch and restores on exit | RIBBON-02 / SC4 | The full edit lifecycle + 3D view + real sketch VP needs the running app (02-04 covers the pure switch logic headless) | Enter a sketch in FreeWorks mode; confirm the Sketch tab activates, then leave and confirm the prior tab restores |

*If none: "All phase behaviors have automated verification."*

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies (8 code tasks → `ctest -R Gui_tests_run`; 1 checkpoint → spike `<human-check>` with `grep -c "PASS\|FAIL" SPIKE.md >= 6`)
- [x] Sampling continuity: no 3 consecutive tasks without automated verify (every code task samples `Gui_tests_run`; the single checkpoint is bracketed by automated tasks)
- [x] Wave 0 covers all MISSING references (`tests/src/Gui/FwRibbon.cpp` + its `tests/src/Gui/CMakeLists.txt` wiring, created by 02-01-T1)
- [x] No watch-mode flags (ctest one-shot only)
- [x] Feedback latency < 60s (Gui_tests_run subset)
- [x] `nyquist_compliant: true` set in frontmatter
- [ ] `wave_0_complete: true` — flips once 02-01-T1 lands the scaffold (currently false; scaffold not yet created)

**Approval:** pending (sign-off boxes satisfied except wave_0_complete, which is gated on 02-01-T1 execution)

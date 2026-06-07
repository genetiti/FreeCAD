---
phase: 2
slug: commandmanager-ribbon
status: draft
nyquist_compliant: false
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
| **Config file** | `src/Gui/TestGui/CMakeLists.txt` (FreeWorks test target wiring) |
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
| 02-XX-XX | TBD | TBD | RIBBON-01 | — | N/A (additive Gui widget, no new attack surface) | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ❌ W0 | ⬜ pending |
| 02-XX-XX | TBD | TBD | RIBBON-02 | — | N/A | unit (headless GTest) | `ctest -R Gui_tests_run --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

> Per-task rows are finalized by the planner once PLAN.md task IDs exist. The headless-testable
> surface (per RESEARCH.md): ribbon builds from a known command tree, context switch toggles the
> active tab, layout persistence round-trips. GUI-only fidelity (visual SW-likeness, flyout
> hover) falls to the Manual-Only table below.

---

## Wave 0 Requirements

- [ ] `src/Gui/TestGui/FwRibbonTest.cpp` — headless GTest stubs for RIBBON-01 (ribbon builds from a known `ToolBarItem` tree) and RIBBON-02 (context switch toggles active tab)
- [ ] Persistence round-trip test scaffold (save/restore tab + panel state via `QMainWindow::saveState/restoreState`)
- [ ] Wire FreeWorks ribbon test sources into the existing `Gui_tests_run` target (mirror Phase 1 pattern; GTest framework already present)

*If none: "Existing infrastructure covers all phase requirements."*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ribbon reads as "this is SolidWorks" — large labeled icons, tab strip, SW-like layout | RIBBON-01 / SC1 | Visual fidelity is subjective; no headless assertion captures "looks SW-like" | Launch FreeWorks mode tri-OS; confirm tabbed ribbon in top area with large icon-over-label buttons replacing menus+toolbars |
| Flyout split-button reveals grouped commands on arrow click | RIBBON-01 / SC3 | Native `QToolButton(MenuButtonPopup)` popup interaction needs a live event loop/pointer | Click a split-button (e.g. Fillet ▸) and confirm Chamfer/related commands appear and fire |
| Menu bar + toolbars hidden and cleanly restored on FreeWorks deactivation | SC1 / D-11 | Round-trip of `ForceHidden` chrome state across workbench toggle needs interactive verification | Toggle in/out of FreeWorks mode; confirm stock chrome hides then reappears intact (also covered by a headless workbench-toggle test where feasible) |
| Layout persists across an actual app restart | RIBBON-01 / SC5 | True restart persistence exercises real config write/read, not an in-process round-trip | Set a tab, restart app, confirm tab/panel state restores |

*If none: "All phase behaviors have automated verification."*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

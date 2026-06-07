---
phase: 1
slug: solidworks-mode-foundation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-06
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | GoogleTest (C++ `Gui_tests_run` target) + ctest; shell/CI scripts for gates |
| **Config file** | `src/Gui/CMakeLists.txt` (test target registration) |
| **Quick run command** | `ctest -R Gui_tests_run --output-on-failure` |
| **Full suite command** | `ctest --output-on-failure` |
| **Estimated runtime** | ~depends on build (incremental); CI gates run in seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R Gui_tests_run --output-on-failure`
- **After every plan wave:** Run `ctest --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** ~120 seconds (incremental build + targeted test)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| TBD | TBD | TBD | SHELL-01 / SHELL-02 / NAV-01 | — | N/A | unit / CI gate / headless | TBD by planner | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*Planner fills concrete rows; derived from RESEARCH.md ## Validation Architecture.*

---

## Wave 0 Requirements

- [ ] Asset-provenance CI guard script + `ASSET_PROVENANCE.md` scaffold
- [ ] Headless/`--console` `.FCStd`-compat gate (no GUI-state leak into App layer)
- [ ] "SolidWorks"-string leak grep over new FreeWorks code
- [ ] Scripted upstream-sync drill against a pinned upstream commit
- [ ] GTest stub(s) for `FwWorkbench` registration / dock-shell mount

*Finalized by planner against the success criteria.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| SW-style shell builds and launches on Windows, macOS, Linux | SHELL-02 | Cross-platform GUI launch not fully automatable in CI | Launch FreeCAD, activate FreeWorks workbench, confirm dock shell mounts on each OS |
| macOS no-middle-button / trackpad nav profile | NAV-01 | Requires real Mac trackpad hardware | On macOS: rotate/pan via modifier-emulated MMB or gesture; confirm SW-parity feel |

*Cross-platform launch + Mac nav are manual; CI guards/headless gate are automated.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending

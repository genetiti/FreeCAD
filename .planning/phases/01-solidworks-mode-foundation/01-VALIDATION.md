---
phase: 1
slug: solidworks-mode-foundation
status: verified
nyquist_compliant: true
wave_0_complete: true
created: 2026-06-06
audited: 2026-06-07
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
| Workbench + Fw_* dock registration | 01-01 | W1 | SHELL-01 | T-01-01/02 | App/Gui separation | unit (GTest) | `ctest -R Gui_tests_run` → `registersAndExposesFwDockNames` | ✅ `tests/src/Gui/FwWorkbench.cpp:80` | ✅ green |
| SW navigation default (unset → SolidWorksNavigationStyle) | 01-02 | W1 | NAV-01 | T-01-04 | no shared-state clobber | unit (GTest) | `ctest -R Gui_tests_run` → `navigationStyleDefaultsToSolidWorksWhenUnset` | ✅ `tests/src/Gui/FwWorkbench.cpp:111` | ✅ green |
| Navigation no-clobber edge case | 01-02 | W1 | NAV-01 | T-01-04 | preserve user choice | unit (GTest) | `ctest -R Gui_tests_run` → `navigationStyleDoesNotClobberUserChoice` | ✅ `tests/src/Gui/FwWorkbench.cpp:122` | ✅ green |
| Headless App/Gui-leak + `.FCStd` recompute gate | 01-03 | W2 | SHELL-01 / SHELL-02 | T-01-07/09 | no GUI state in App layer | CI gate (headless) | `sub_fwHeadlessCompat.yml` (CI_primary:77) → `HEADLESS_OK` | ✅ `.github/workflows/sub_fwHeadlessCompat.yml` | ✅ green |
| Cross-platform build matrix | 01-03 | W2 | SHELL-02 | T-01-08 | tri-OS compile parity | CI gate (build) | `sub_buildUbuntu` / `sub_buildPixi` / `sub_buildWindows` | ✅ CI matrix | ✅ green |
| Asset-provenance guard + self-test | 01-04 | W2 | SHELL-01 | T-01-10 | no unlisted/derived asset | CI gate (script) | `sub_fwForkGuards.yml` (CI_primary:84) → `fw-provenance-guard.sh` | ✅ `tools/fw-provenance-guard.sh` | ✅ green |
| "SolidWorks"-string leak grep | 01-04 | W2 | SHELL-01 | T-01-11 | no trademark leak (D-03) | CI gate (script) | `sub_fwForkGuards.yml` → `fw-string-leak-grep.sh` | ✅ `tools/fw-string-leak-grep.sh` | ✅ green |
| Upstream-sync merge-discipline drill | 01-04 | W2 | SHELL-01 | T-01-12 | marked shared-file edits | CI gate (script) | `sub_fwForkGuards.yml` → `fw-sync-upstream.sh` | ✅ `tools/fw-sync-upstream.sh` | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*All automatable Phase 1 requirements covered. Cross-OS GUI launch (SHELL-02) and macOS trackpad feel (NAV-01) are inherently un-automatable in CI → see Manual-Only (both passed in 01-UAT.md).*

---

## Wave 0 Requirements

- [x] Asset-provenance CI guard script + `ASSET_PROVENANCE.md` scaffold — `tools/fw-provenance-guard.sh` + ledger, CI-wired
- [x] Headless/`--console` `.FCStd`-compat gate (no GUI-state leak into App layer) — `sub_fwHeadlessCompat.yml`
- [x] "SolidWorks"-string leak grep over new FreeWorks code — `tools/fw-string-leak-grep.sh`
- [x] Scripted upstream-sync drill against a pinned upstream commit — `tools/fw-sync-upstream.sh`
- [x] GTest stub(s) for `FwWorkbench` registration / dock-shell mount — `tests/src/Gui/FwWorkbench.cpp`

*All Wave 0 validation infrastructure delivered and CI-wired.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions | UAT Result |
|----------|-------------|------------|-------------------|------------|
| SW-style shell builds and launches on Windows, macOS, Linux | SHELL-02 | Cross-platform GUI launch not fully automatable in CI | Launch FreeCAD, activate FreeWorks workbench, confirm dock shell mounts on each OS (`TRIOS_LAUNCH_CHECKLIST.md`) | ✅ pass (01-UAT.md Test 1) |
| macOS no-middle-button / trackpad nav profile | NAV-01 | Requires real Mac trackpad hardware | On macOS: rotate/pan via modifier-emulated MMB or gesture; confirm SW-parity feel (`MACOS_NAV_PROFILE.md` §4) | ✅ pass (01-UAT.md Test 2) |

*Cross-platform launch + Mac nav are manual (inherent); both confirmed via UAT. CI guards/headless gate/GTests are automated.*

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 120s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** verified 2026-06-07

---

## Validation Audit 2026-06-07

| Metric | Count |
|--------|-------|
| Requirements (SHELL-01, SHELL-02, NAV-01) | 3 |
| Automatable gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Automated test/CI rows | 8 |
| Documented manual-only (both UAT-passed) | 2 |

State A audit: reconciled the placeholder per-task map against shipped artifacts. Every automatable Phase 1 requirement has a green GTest or CI gate; the two inherently-manual legs (cross-OS GUI launch, macOS trackpad feel) are documented and confirmed via `01-UAT.md`. No auditor spawn required — no automatable gaps existed.

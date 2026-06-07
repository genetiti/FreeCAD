---
phase: 01-solidworks-mode-foundation
verified: 2026-06-07T00:00:00Z
status: human_needed
score: 8/9 must-haves verified
overrides_applied: 0
human_verification:
  - test: "Tri-OS GUI launch and dock shell mount confirmation"
    expected: "On Windows, macOS, and Linux: build FreeCAD with FreeWorks, launch the GUI binary, activate the FreeWorks workbench, confirm Fw_FeatureManager and Fw_PropertyManager appear docked on the left, Fw_TaskPane is reserved on the right, the top toolbar area is free, no crash on activation or workbench switch, and the reference-CAD navigation default is active"
    why_human: "SHELL-02 requires GUI launch on real hardware across all three OSes. No build tree, no tri-OS hardware, and no live CI run are available in this environment. The CI matrix (sub_buildUbuntu, sub_buildWindows via CI_approved, sub_buildPixi via CI_approved) proves compile; the sub_fwHeadlessCompat gate proves App/Gui separation; but dock-shell geometry, navigation feel, and crash absence on activation require a human confirming against TRIOS_LAUNCH_CHECKLIST.md. This item was explicitly delegated to a TRIOS_LAUNCH_CHECKLIST.md sign-off in Plans 03 and 04."
  - test: "macOS real-hardware navigation feel (trackpad / 2-button mouse)"
    expected: "With the modifier-emulated-MMB chord (Option + left-drag for rotate; Option+Command for pan; Shift+Option for dolly; Control+Option for roll; two-finger scroll for zoom-to-cursor), the SolidWorks navigation semantics are reached on a real Mac. GestureNavigationStyle is selectable as a one-click native-trackpad alternative and works as documented."
    why_human: "Documented in MACOS_NAV_PROFILE.md and flagged as a deferred Mac-hardware spike at Plan 02 Task 3. No Mac build tree or real trackpad hardware available. The modifier-emulated-MMB chord must be confirmed ergonomically and for gesture conflicts on real Apple hardware."
---

# Phase 01: SolidWorks Mode Foundation Verification Report

**Phase Goal:** Additive module shell + upstream-merge & asset-provenance discipline + CI gates + SolidWorks navigation as the default — the app launches into a coherent SolidWorks-mode skeleton (left-docked FeatureManager/PropertyManager placeholders, right TaskPane, top ribbon area reserved) that navigates like SolidWorks, with zero MainWindow.cpp body edits and the fork kept mergeable/asset-clean/legally safe.
**Verified:** 2026-06-07
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | User can select "FreeWorks" in the FreeCAD workbench selector | VERIFIED | `src/Gui/FreeWorks/InitGui.py` registers `FreeWorksWorkbench` with `MenuText = "FreeWorks"` via `Gui.addWorkbench()`; `GetClassName()` returns `"FreeWorksGui::FwWorkbench"`. Installed to `Mod/FreeWorks/` via CMake `fc_target_copy_resource`/`install`. |
| 2 | Activating FreeWorks mounts a coherent dock shell with Fw_* placeholder docks | VERIFIED | `FwWorkbench::setupDockWindows()` calls `FwLayout::install()` before returning, ensuring placeholder widgets are registered in `DockWindowManager` before `setup()` consumes the dock-items list (CR-01 fix confirmed at commit `5fed81a291`). `FwLayout::install()` registers three unique-named, titled placeholder `QLabel` widgets via `DockWindowManager::registerDockWindow()`. |
| 3 | Fw_FeatureManager and Fw_PropertyManager dock left; Fw_TaskPane reserved right; top toolbar area free | VERIFIED | `FwWorkbench::setupDockWindows()` adds `Fw_FeatureManager` + `Fw_PropertyManager` as `LeftDockWidgetArea / Visible` and `Fw_TaskPane` as `RightDockWidgetArea / VisibleTabbed`. No top-area dock registered. No Std_* dock block modified. Visual confirmation requires human launch (human_verification item 1). |
| 4 | Zero edits to MainWindow.cpp bodies; one marked CMake edit only | VERIFIED | `git log -- src/Gui/MainWindow.cpp` shows no FreeWorks commits. `grep -c 'add_subdirectory(FreeWorks)' src/Gui/CMakeLists.txt` returns exactly 1, on line 17: `add_subdirectory(FreeWorks)  # SW-FORK HOOK`. Two SW-FORK HOOK markers in total exist in `src/` — 1 on a shared file (`src/Gui/CMakeLists.txt`) and 2 inside the additive module (`FwNavigationDefault.cpp`). The offline sync drill confirms the single shared-file edit is marked and exits 0. |
| 5 | SolidWorks navigation is the FreeWorks default when the user has not chosen a nav style | VERIFIED | `FwNavigationDefault::applyDefault()` reads `User parameter:BaseApp/Preferences/View NavigationStyle`; writes `"Gui::SolidWorksNavigationStyle"` (marked `// SW-FORK HOOK`) only when the key is absent. Called from `FwWorkbench::activated()`. No-clobber logic verified in `FwNavigationDefault.cpp` lines 45–55. Test at `tests/src/Gui/FwWorkbench.cpp` asserts this. |
| 6 | An existing user-chosen NavigationStyle is never clobbered | VERIFIED | `FwNavigationDefault.cpp:45` guards: `if (current.empty())` — writes only when key is unset. GTest `navigationStyleDoesNotClobberUserChoice` asserts this edge case. |
| 7 | macOS has an explicit no-middle-button / trackpad profile documented and selectable | VERIFIED (source) / UNCERTAIN (behavior) | `MACOS_NAV_PROFILE.md` exists with the Option+left-drag chord, GestureNavigationStyle one-click alternative, full rotate/pan/zoom/roll/dolly table, and a Mac-hardware spike checklist. `FwNavigationDefault.cpp` sets `FwMacNavProfile = "ModifierEmulatedMMB"` as an intent marker (comment states plainly this only records intent; real binding is a later phase — WR-03 fix confirmed). Actual ergonomic feel requires human hardware validation (human_verification item 2). |
| 8 | CI asset-provenance guard rejects any binary image lacking a source entry in ASSET_PROVENANCE.md | VERIFIED | `tools/fw-provenance-guard.sh` exits 0 on the current tree (1 image listed). Self-test with `/tmp/fwprov-test/unlisted.png` exits non-zero — confirmed live. Anchored table-cell match (`^\| \`...\` \|`) confirmed at WR-05 fix. Wired into `sub_fwForkGuards.yml` → `CI_primary.yml`. |
| 9 | Scripted upstream-sync drill runs and completes with all shared-file touches greppable via SW-FORK HOOK | VERIFIED | `FW_SYNC_OFFLINE=1 bash tools/fw-sync-upstream.sh` exits 0. Pin self-check passes (`768e237091` is a pristine upstream base, ancestor of HEAD, no fork artifacts — CR-04 fix confirmed). One shared-file edit (`src/Gui/CMakeLists.txt`) is correctly detected as marked. |

**Score:** 8/9 truths verified (truth 7 is source-verified; real-hardware feel deferred to human checklist)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/Gui/FreeWorks/FwWorkbench.h` | `class FwWorkbench` in `namespace FreeWorksGui`, subclasses `Gui::StdWorkbench` | VERIFIED | Line 49: `class FwWorkbench: public Gui::StdWorkbench`. Namespace present line 31. `TYPESYSTEM_HEADER_WITH_OVERRIDE()` present. |
| `src/Gui/FreeWorks/FwWorkbench.cpp` | `setupDockWindows()` registering Fw_* dock names + `FwLayout::install()` call | VERIFIED | Line 88: `FwLayout::install()` called from `setupDockWindows()`. Lines 97-105: all three dock names added. `TYPESYSTEM_SOURCE` at line 38. |
| `src/Gui/FreeWorks/FwLayout.cpp` | `FwLayout::install()` registering labeled placeholder widgets via `registerDockWindow` | VERIFIED | Lines 94-105: all three `ensureDock()` calls. Unique `objectName` per dock (CR-02 fix). No App-layer include. |
| `src/Gui/FreeWorks/CMakeLists.txt` | FreeWorks module sources compiled into FreeCADGui | VERIFIED | `target_sources(FreeCADGui PRIVATE ${FreeWorks_SRCS})` at line 42. All source files listed. InitGui.py installed to `Mod/FreeWorks`. |
| `src/Gui/CMakeLists.txt` | `add_subdirectory(FreeWorks)` marked `# SW-FORK HOOK` | VERIFIED | Line 17: `add_subdirectory(FreeWorks)  # SW-FORK HOOK`. Exactly one occurrence. |
| `src/Gui/FreeWorks/FwNavigationDefault.h` | `applyDefault()` declaration | VERIFIED | Line 58: `static void applyDefault();` in `namespace FreeWorksGui`. |
| `src/Gui/FreeWorks/FwNavigationDefault.cpp` | Reads NavigationStyle; writes `Gui::SolidWorksNavigationStyle` when unset | VERIFIED | Lines 45-55: read with empty default, conditional write with `// SW-FORK HOOK`. |
| `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` | macOS profile with GestureNavigationStyle alternative and spike checklist | VERIFIED | File exists. Contains modifier chord table, GestureNavigationStyle alternative, mapping table, spike checklist in section 4. |
| `tests/src/Gui/FwWorkbench.cpp` | GTest: registration + Fw_* docks + NavigationStyle default | VERIFIED | Three tests present: `registersAndExposesFwDockNames`, `navigationStyleDefaultsToSolidWorksWhenUnset`, `navigationStyleDoesNotClobberUserChoice`. `FwWorkbenchAccessor` subclass for CR-03 fix confirmed. |
| `tests/src/Gui/CMakeLists.txt` | `FwWorkbench.cpp` in `Gui_tests_run` source list | VERIFIED | Line 17: `FwWorkbench.cpp` present in the source list. |
| `.github/workflows/sub_fwHeadlessCompat.yml` | `workflow_call` gate proving no App-layer GUI leak | VERIFIED | `on: workflow_call:` present. HEADLESS_OK grep present. App-layer Document.xml leak detection present. |
| `src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md` | Manual tri-OS launch verification checklist | VERIFIED | File exists with per-OS build, launch, dock-shell, nav-default, and no-crash checklist items. |
| `.github/workflows/sub_fwForkGuards.yml` | `workflow_call` gate running provenance guard + leak grep | VERIFIED | `on: workflow_call:` present. Runs all three guards including the offline sync-drill marker assertion. |
| `ASSET_PROVENANCE.md` | Provenance ledger with one row per binary image | VERIFIED | Table row for `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg` marked `recreated-original`. |
| `tools/fw-provenance-guard.sh` | Exits non-zero on unlisted binary image | VERIFIED | Exits 0 on current tree; exits 1 on self-test with unlisted image (confirmed live). |
| `tools/fw-string-leak-grep.sh` | Fails on any "SolidWorks" except allow-listed `Gui::SolidWorksNavigationStyle` | VERIFIED | Exits 0 on current tree (confirmed live). Only `Gui::SolidWorksNavigationStyle` present. |
| `tools/fw-sync-upstream.sh` | Pinned-commit upstream-sync drill asserting SW-FORK HOOK greppability | VERIFIED | Pin self-check guard present (step 0). Pin updated to `768e237091` (CR-04 fix). Exits 0 offline (confirmed live). |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/Gui/FreeWorks/InitGui.py` | `FreeWorksGui::FwWorkbench` | `GetClassName()` return | VERIFIED | `GetClassName()` returns `"FreeWorksGui::FwWorkbench"` (line 47). `Gui.addWorkbench(FreeWorksWorkbench())` at line 50. |
| `src/Gui/FreeWorks/FwWorkbench.cpp` | `FwLayout::install` | `setupDockWindows()` override | VERIFIED | Line 88: `FwLayout::install()` called at top of `setupDockWindows()` before dock items are returned (CR-01 fix). |
| `src/Gui/CMakeLists.txt` | `src/Gui/FreeWorks/CMakeLists.txt` | `add_subdirectory(FreeWorks)` | VERIFIED | Line 17: exactly one occurrence, marked `# SW-FORK HOOK`. |
| `src/Gui/FreeWorks/FwWorkbench.cpp` | `FwNavigationDefault::applyDefault` | `activated()` override | VERIFIED | Line 50: `FwNavigationDefault::applyDefault()` called from `activated()`. |
| `src/Gui/FreeWorks/FwNavigationDefault.cpp` | `User parameter:BaseApp/Preferences/View NavigationStyle` | `GetParameterGroupByPath` + `SetASCII when unset` | VERIFIED | Lines 39-55: `GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")`, guarded `SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle")`. |
| `.github/workflows/CI_primary.yml` | `.github/workflows/sub_fwHeadlessCompat.yml` | `uses:` | VERIFIED | Line 77: `uses: ./.github/workflows/sub_fwHeadlessCompat.yml` in `FwHeadlessCompat` job. |
| `.github/workflows/CI_primary.yml` | `.github/workflows/sub_fwForkGuards.yml` | `uses:` | VERIFIED | Line 84: `uses: ./.github/workflows/sub_fwForkGuards.yml` in `FwForkGuards` job. |
| `tools/fw-provenance-guard.sh` | `ASSET_PROVENANCE.md` | `grep -qE` table cell match | VERIFIED | Line 74: anchored regex match `^\| \`${rel_esc}\` \|` (WR-05 fix). |
| `tests/src/Gui/FwWorkbench.cpp` | `FreeWorksGui::FwWorkbench` | GTest assertions via `FwWorkbenchAccessor` | VERIFIED | `FwWorkbenchAccessor` exposes `setupDockWindows()` via `using` declaration (CR-03 fix). Tests assert all three Fw_* dock names. |

### Data-Flow Trace (Level 4)

Not applicable. Phase 1 artifacts are infrastructure/skeleton (workbench shell, preference defaults, CI scripts, provenance ledger) — no dynamic data rendering. The placeholder dock widgets display static label text by design; real data (FeatureManager tree, PropertyManager fields) is Phase 3/4 scope.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Provenance guard accepts current tree | `bash tools/fw-provenance-guard.sh` | exit 0; "scanned 1 image(s); 0 unlisted" | PASS |
| Provenance guard rejects unlisted image | `bash tools/fw-provenance-guard.sh /tmp/fwprov-test` (with unlisted.png) | exit 1; "FAIL unlisted" | PASS |
| String leak grep passes on current tree | `bash tools/fw-string-leak-grep.sh` | exit 0; "no disallowed 'SolidWorks' tokens" | PASS |
| Upstream-sync drill offline marker assertion | `FW_SYNC_OFFLINE=1 bash tools/fw-sync-upstream.sh` | exit 0; "PASS — all shared-file touches are 'SW-FORK HOOK'-greppable" | PASS |
| GUI launch and dock shell mount | Requires tri-OS build + hardware | Cannot run without build tree | SKIP (human_verification item 1) |
| macOS trackpad navigation feel | Requires real Mac hardware | Cannot run without Mac hardware | SKIP (human_verification item 2) |

### Probe Execution

No `probe-*.sh` scripts declared for this phase. Phase 3 PLAN (CI gate) uses standard GitHub Actions workflows, not standalone probe scripts.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| SHELL-01 | 01-PLAN.md | User can launch FreeCAD into a "SolidWorks mode" coherent layout | SATISFIED | `FwWorkbench` registered in workbench selector; `FwLayout::install()` mounts three Fw_* placeholder docks in the correct SW geometry on activation. Zero MainWindow.cpp edits. |
| SHELL-02 | 02-PLAN.md, 03-PLAN.md, 04-PLAN.md | SolidWorks-style interface builds and runs on Windows, macOS, and Linux | PARTIALLY SATISFIED — human needed | FreeWorks sources compile into `FreeCADGui` via `target_sources()`. Ubuntu build is in `CI_primary.yml`; Windows/macOS builds are in `CI_approved.yml` (label-gated, upstream's standard model — not a regression). `sub_fwHeadlessCompat.yml` + `sub_fwForkGuards.yml` wired into `CI_primary.yml`. `TRIOS_LAUNCH_CHECKLIST.md` authored. Tri-OS live GUI launch sign-off outstanding (human_verification item 1). |
| NAV-01 | 02-PLAN.md | SolidWorks mouse navigation is the default (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB) | SATISFIED | `FwNavigationDefault::applyDefault()` writes `Gui::SolidWorksNavigationStyle` when unset. No-clobber discipline in place. GTest covers this. macOS profile documented. Real-hardware confirmation deferred (human_verification item 2). |

All three requirements declared for Phase 1 are accounted for. No orphaned requirements.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/Gui/FreeWorks/FwTheme.cpp` | 31-39 | `FwTheme::apply()` is a planned no-op stub | Info | Intentional Walking Skeleton stub for Phase 7. The seam is now live (called from `activated()` per WR-02 fix). No blocker. |
| `src/Gui/FreeWorks/FwLayout.cpp` | 94-105 | Placeholder label text "FeatureManager (Phase 3)" etc. | Info | Intentional Walking Skeleton placeholders. Each references the resolving phase. No blocker. |
| `src/Gui/FreeWorks/FwNavigationDefault.cpp` | 70-76 | `FwMacNavProfile` key written but not consumed in this phase | Info | WR-03 fix confirmed: comment now states plainly this only records intent; real binding is a later phase. No blocker. |

No `TBD`, `FIXME`, or `XXX` debt markers found in any phase-modified file.

**Critical issues from 01-REVIEW.md — all 4 verified as fixed:**

| Review Finding | Fix Commit | Verification |
|----------------|-----------|--------------|
| CR-01: Dock shell never appears (install after setup) | `5fed81a291` | `FwLayout::install()` now called from `setupDockWindows()` (line 88), before framework consumes dock items |
| CR-02: Shared "FwPlaceholder" objectName breaks Qt layout save/restore | `5fed81a291` | `makePlaceholder()` takes unique `objectName` per dock; each placeholder's `objectName` equals its dock name |
| CR-03: Test calls protected `setupDockWindows()` — compile error | `22c0e3b21e` | `FwWorkbenchAccessor` subclass with `using` declaration at test line 38-41 |
| CR-04: Sync guard pinned to fork planning commit (not pristine upstream) | `efd04aa25b` | `DEFAULT_PIN = "768e237091..."` (pristine upstream base); step-0 self-check added |

**Warning fixes from 01-REVIEW.md — all 6 verified as fixed:**

| Finding | Fix Commit | Verification |
|---------|-----------|--------------|
| WR-01: `install()` ignored its `QMainWindow*` parameter | `5fed81a291` | Parameter removed; signature is now `static void install()` |
| WR-02: `FwTheme::apply()` declared but never called | `b124200fbb` | Called from `FwWorkbench::activated()` at line 54 |
| WR-03: macOS nav comment overstated behavior | `45cecf78b3` | Comment now states "only RECORDS the chosen substitute-profile intent" (line 63) |
| WR-04: `<QMainWindow>/<QLabel>` only included under `!_PreComp_` | `5fed81a291` | `<QLabel>` included unconditionally; `<QMainWindow>` no longer needed |
| WR-05: Provenance guard used unanchored substring match | `d922e2be81` | `grep -qE "^\| \`${rel_esc}\` \|"` (anchored table-cell match) |
| WR-06: Stale "produced in Plan 04" comments | `7d090ad706` | Comments updated; unused `import FreeCAD as App` removed (IN-01 fix) |

### Human Verification Required

#### 1. Tri-OS GUI Launch and Dock Shell Mount (TRIOS_LAUNCH_CHECKLIST.md)

**Test:** On each of Windows, macOS, and Linux: build FreeCAD with FreeWorks, launch the GUI binary, activate the "FreeWorks" workbench from the workbench selector. Confirm: `Fw_FeatureManager` docked left, `Fw_PropertyManager` docked left, `Fw_TaskPane` reserved right, top toolbar area free, no crash on activation or workbench switch (in/out), reference-CAD navigation default active. Walk `src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md` and sign off each OS.
**Expected:** Dock shell mounts cleanly with SW left/right geometry on all three OSes. No crash. FreeWorks appears in the workbench selector list. Navigation default (rotate=MMB, pan=Ctrl+MMB) is active immediately after activation.
**Why human:** No build tree, no tri-OS hardware, no live CI available in this environment. Compilation is wired into the per-OS CI matrix; dock geometry and crash absence require a live GUI. Explicitly delegated to TRIOS_LAUNCH_CHECKLIST.md in Plans 03 and 04.

#### 2. macOS Real-Hardware Navigation Feel

**Test:** On real macOS hardware (trackpad and/or 2-button mouse without physical MMB): build FreeCAD with FreeWorks, launch, activate FreeWorks, open a 3D document. Using the Option+left-drag chord (emulated MMB), confirm rotate, pan (Option+Command+drag), zoom-to-cursor (scroll), roll (Control+Option+drag), dolly (Shift+Option+drag). Switch to `Gui::GestureNavigationStyle` and confirm native trackpad gestures work. Walk the spike checklist in `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` §4. Record any gesture conflicts or modifier-ergonomics issues.
**Expected:** All five navigation actions reach with SolidWorks semantic parity using the modifier chords. No macOS-reserved modifier conflicts. GestureNavigationStyle selectable as one-click alternative.
**Why human:** No real Mac hardware or build tree available. The `FwMacNavProfile` key documents intent only in this phase — actual MMB emulation binding is a later-phase deliverable. However, the chosen modifier chord (Option+left-drag) must be validated ergonomically on real hardware to confirm the spike checklist design before Phase 6 commits to it.

### Gaps Summary

No gaps blocking goal achievement. All 4 critical bugs from the code review are confirmed fixed in source. All 6 warnings are fixed. The 3 tools run cleanly live. All artifacts exist and are substantive.

The single `human_needed` status is driven by the two items above, which were explicitly planned as deferred human verifications in Plans 02/03. They are not regressions or missing work — the source-level artifacts that drive them (TRIOS_LAUNCH_CHECKLIST.md, MACOS_NAV_PROFILE.md, tri-OS CI matrix wiring, FwNavigationDefault) all exist and are correct.

---

_Verified: 2026-06-07_
_Verifier: Claude (gsd-verifier)_

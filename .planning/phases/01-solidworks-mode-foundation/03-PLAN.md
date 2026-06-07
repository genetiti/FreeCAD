---
phase: 01-solidworks-mode-foundation
plan: 03
type: execute
wave: 2
depends_on: [01-01]
files_modified:
  - tests/src/Gui/FwWorkbench.cpp
  - tests/src/Gui/CMakeLists.txt
  - .github/workflows/sub_fwHeadlessCompat.yml
  - .github/workflows/CI_primary.yml
  - src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md
autonomous: false
requirements: [SHELL-02]
must_haves:
  truths:
    - "The FreeWorks subdirectory compiles in the existing per-OS CI matrix (Windows, Ubuntu, Pixi/macOS)"
    - "A headless/--console .FCStd-compat gate proves no GUI state leaks into the App layer"
    - "A tri-OS manual launch checklist confirms the shell mounts on all three operating systems"
    - "A GTest asserts FwWorkbench registers, setupDockWindows() returns Fw_* names, and NavigationStyle defaults to Gui::SolidWorksNavigationStyle"
  artifacts:
    - path: "tests/src/Gui/FwWorkbench.cpp"
      provides: "Headless GTest: workbench registration + Fw_* docks + NavigationStyle default"
      contains: "FwWorkbench"
    - path: "tests/src/Gui/CMakeLists.txt"
      provides: "FwWorkbench.cpp added to Gui_tests_run source list"
      contains: "FwWorkbench.cpp"
    - path: ".github/workflows/sub_fwHeadlessCompat.yml"
      provides: "Headless .FCStd-compat CI gate (workflow_call), asserts no App-layer GUI leak"
      contains: "workflow_call"
    - path: "src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md"
      provides: "Manual Windows/macOS/Linux launch verification checklist"
  key_links:
    - from: ".github/workflows/CI_primary.yml"
      to: ".github/workflows/sub_fwHeadlessCompat.yml"
      via: "uses: ./.github/workflows/sub_fwHeadlessCompat.yml"
      pattern: "sub_fwHeadlessCompat"
    - from: "tests/src/Gui/FwWorkbench.cpp"
      to: "FreeWorksGui::FwWorkbench"
      via: "GTest registration + dock-name assertions"
      pattern: "Fw_FeatureManager"
---

<objective>
Prove SHELL-02: the FreeWorks subdirectory compiles in the existing per-OS CI matrix and the SolidWorks-style shell builds/runs on Windows, macOS, and Linux, AND prove the App/Gui separation invariant via a headless/`--console` `.FCStd`-compat gate that asserts no GUI state leaks into the App layer. A GTest covers FwWorkbench registration, the Fw_* dock names, and the NavigationStyle default; a tri-OS manual checklist covers the parts CI cannot.

Purpose: Cross-platform build + run guarantee (SHELL-02) and the headless detector for the "no App-layer GUI leak" invariant.
Output: `tests/src/Gui/FwWorkbench.cpp` GTest (registered in `Gui_tests_run`), the `sub_fwHeadlessCompat.yml` CI gate slotted into `CI_primary.yml`, and `TRIOS_LAUNCH_CHECKLIST.md`.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/01-solidworks-mode-foundation/01-SKELETON.md
@.planning/phases/01-solidworks-mode-foundation/01-PATTERNS.md
@.planning/phases/01-solidworks-mode-foundation/01-CONTEXT.md
@.planning/phases/01-solidworks-mode-foundation/01-VALIDATION.md
@.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md
</context>

<artifacts_produced>
## Artifacts this phase produces (Plan 03)

| Symbol / Path | Kind |
|---|---|
| `FwWorkbenchTest` (GTest fixture) | C++ test in `Gui_tests_run` |
| `tests/src/Gui/FwWorkbench.cpp` | test file (added to `add_executable(Gui_tests_run ...)`) |
| `.github/workflows/sub_fwHeadlessCompat.yml` | CI gate (workflow_call) |
| `src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md` | manual Win/mac/Linux launch checklist |
| `HEADLESS_OK` assertion token | headless-gate success sentinel |
</artifacts_produced>

<tasks>

<task type="auto" tdd="true">
  <name>Task 1: GTest for FwWorkbench registration + Fw_* docks + NavigationStyle default</name>
  <read_first>
    - 01-PATTERNS.md section "tests/src/Gui/FwWorkbench.cpp (GTest)" — analog `tests/src/Gui/SelectionTest.cpp:1-55`; fixture `class FwWorkbenchTest : public ::testing::Test` with `static void SetUpTestSuite() { tests::initApplication(); }` (`SelectionTest.cpp:42-45`); headless via `App::DocumentInitFlags{ .createView = false }` (`SelectionTest.cpp:51-55`).
    - 01-PATTERNS.md section "tests/src/Gui/CMakeLists.txt (EDIT)" — add `FwWorkbench.cpp` to `add_executable(Gui_tests_run ...)` source list (`tests/src/Gui/CMakeLists.txt:7-16`); target already links FreeCADGui (`:23`).
    - 01-VALIDATION.md "Wave 0 Requirements" (GTest stub for FwWorkbench registration / dock-shell mount) and "Per-Task Verification Map".
  </read_first>
  <behavior>
    - Test 1 (SC1): `FreeWorksGui::FwWorkbench` is type-registered and instantiable; `setupDockWindows()` returns docks whose names include `Fw_FeatureManager`, `Fw_PropertyManager`, `Fw_TaskPane`.
    - Test 2 (SC3): after `FwNavigationDefault::applyDefault()` runs with the key unset, reading `NavigationStyle` from `User parameter:BaseApp/Preferences/View` yields `"Gui::SolidWorksNavigationStyle"`.
    - Edge: when `NavigationStyle` is pre-set to a non-empty value, `applyDefault()` does NOT overwrite it (note A2).
  </behavior>
  <action>
    Create `tests/src/Gui/FwWorkbench.cpp` (SPDX line 1) with fixture `FwWorkbenchTest` copying the headless setup from `SelectionTest.cpp`. Write the three assertions in the behavior block: workbench type registration + Fw_* dock names from `setupDockWindows()`; NavigationStyle default resolves to `Gui::SolidWorksNavigationStyle`; and the no-clobber edge case. Keep the test headless (no view creation). Add `FwWorkbench.cpp` to the `add_executable(Gui_tests_run ...)` source list in `tests/src/Gui/CMakeLists.txt`. Run RED first (assertions fail before Plan 01/02 symbols are wired), then GREEN once the FreeWorks symbols link.
  </action>
  <verify>
    <automated>grep -q "FwWorkbench" tests/src/Gui/FwWorkbench.cpp &amp;&amp; grep -q "Fw_FeatureManager" tests/src/Gui/FwWorkbench.cpp &amp;&amp; grep -q "Gui::SolidWorksNavigationStyle" tests/src/Gui/FwWorkbench.cpp &amp;&amp; grep -q "FwWorkbench.cpp" tests/src/Gui/CMakeLists.txt</automated>
  </verify>
  <acceptance_criteria>
    - `tests/src/Gui/FwWorkbench.cpp` asserts FwWorkbench registration, the three Fw_* dock names, and the NavigationStyle default (plus the no-clobber edge).
    - `FwWorkbench.cpp` is in the `Gui_tests_run` source list.
    - `ctest -R Gui_tests_run --output-on-failure` passes green once Plans 01/02 are built.
  </acceptance_criteria>
  <done>The headless GTest proves FwWorkbench registers, exposes the Fw_* docks, and defaults NavigationStyle to Gui::SolidWorksNavigationStyle without clobbering a user choice; runs in Gui_tests_run.</done>
</task>

<task type="auto">
  <name>Task 2: Headless .FCStd-compat CI gate (no GUI-state leak into App layer)</name>
  <read_first>
    - 01-PATTERNS.md section ".github/workflows/ provenance + headless gates" — analog `.github/workflows/sub_lint.yml:30-40` `workflow_call` reusable-job shape; headless gate via `FreeCADCmd -c "...openDocument...recompute..."` asserting `HEADLESS_OK` and no Fw/Gui keys; `MainCmd.cpp` confirmed present.
    - 01-VALIDATION.md "Wave 0 Requirements" (headless/--console .FCStd-compat gate) and "Manual-Only Verifications".
    - 01-SKELETON.md "Deployment/full-stack run" row + Anti-Pattern 2 (no App-layer leak); RESEARCH.md "## Validation Architecture" ONLY if a specific headless command string is needed.
  </read_first>
  <action>
    Create `.github/workflows/sub_fwHeadlessCompat.yml` (SPDX header line 1) as a `workflow_call` reusable job mirroring the `sub_lint.yml` shape. The job: builds (or reuses an artifact of) FreeCAD with FreeWorks, then runs `FreeCADCmd`/`--console` to open a `.FCStd` document and recompute it headlessly, asserting (a) the run prints a `HEADLESS_OK` sentinel and exits 0, and (b) the document contains no FreeWorks/Gui keys in the App-layer persisted state (grep the saved `.FCStd`/Document.xml for `Fw_`/`FreeWorksGui`/Gui-only keys → must be absent), proving no GUI state leaked into the App layer and the file opens in unmodified upstream. Slot the gate into `.github/workflows/CI_primary.yml` via `uses: ./.github/workflows/sub_fwHeadlessCompat.yml` alongside the existing sub_* jobs.
  </action>
  <verify>
    <automated>test -f .github/workflows/sub_fwHeadlessCompat.yml &amp;&amp; grep -q "workflow_call" .github/workflows/sub_fwHeadlessCompat.yml &amp;&amp; grep -q "HEADLESS_OK" .github/workflows/sub_fwHeadlessCompat.yml &amp;&amp; grep -q "sub_fwHeadlessCompat" .github/workflows/CI_primary.yml</automated>
  </verify>
  <acceptance_criteria>
    - `sub_fwHeadlessCompat.yml` is a `workflow_call` job that exits 0 with `HEADLESS_OK` on a clean recompute.
    - The gate asserts the saved `.FCStd` App-layer state contains no `Fw_`/`FreeWorksGui`/Gui-only keys (fails if any GUI state leaked).
    - The gate is referenced from `CI_primary.yml`.
  </acceptance_criteria>
  <done>A headless --console gate recomputes a FreeWorks-saved document, prints HEADLESS_OK, exits 0, and asserts the App layer carries no FreeWorks/Gui state — wired into CI_primary.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 3: Tri-OS manual launch verification (Windows, macOS, Linux)</name>
  <what-built>The FreeWorks subdirectory compiles in the existing per-OS CI matrix (sub_buildWindows/Ubuntu/Pixi). Cross-platform GUI launch + shell-mount feel are not fully automatable in CI, so they are verified manually on all three OSes against TRIOS_LAUNCH_CHECKLIST.md.</what-built>
  <how-to-verify>
    1. Author `src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md` (SPDX line 1) listing, per OS (Windows / macOS / Linux): build command, launch, activate FreeWorks, confirm the Fw_* dock shell mounts in SW geometry, top toolbar area reserved, no crash, navigation default active.
    2. On each of Windows, macOS, and Linux: build FreeCAD with FreeWorks linked, launch, select "FreeWorks" in the workbench selector, confirm the `Fw_FeatureManager`/`Fw_PropertyManager` (left) + reserved `Fw_TaskPane` (right) shell mounts cleanly and the SolidWorks navigation default is active.
    3. Confirm the per-OS CI matrix (sub_buildWindows.yml / sub_buildUbuntu.yml / sub_buildPixi.yml) is green for the branch.
  </how-to-verify>
  <resume-signal>Type "approved" if the shell mounts cleanly and CI is green on all three OSes; otherwise list the failing OS + symptom.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Gui layer -> App-layer persisted state | GUI state must never serialize into `.FCStd` / App documents |
| CI matrix -> three host OSes | New subdirectory must compile identically across Win/mac/Linux |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-01-07 | Information Disclosure | App/Gui separation | mitigate | Headless gate asserts no `Fw_`/`FreeWorksGui`/Gui keys in saved App state; HIGH-value invariant, blocks CI on leak |
| T-01-08 | Denial of Service | cross-platform build | mitigate | Existing per-OS CI matrix compiles the new subdirectory; tri-OS manual launch confirms runtime |
| T-01-09 | Tampering | `.FCStd` upstream compat | mitigate | Gate opens the FreeWorks-saved file in unmodified upstream path and recomputes to `HEADLESS_OK` |
</threat_model>

<verification>
- `ctest -R Gui_tests_run --output-on-failure` green (FwWorkbench registration + Fw_* docks + NavigationStyle default).
- `sub_fwHeadlessCompat.yml` exits 0 with `HEADLESS_OK` and asserts no App-layer GUI leak.
- Per-OS CI matrix green on Windows/Ubuntu/Pixi; tri-OS manual checklist signed off.
</verification>

<success_criteria>
- SHELL-02: SolidWorks-style shell builds and runs on Windows, macOS, Linux (CI green + manual tri-OS).
- Headless `.FCStd`-compat gate proves no GUI state leaks into the App layer.
</success_criteria>

<output>
Create `.planning/phases/01-solidworks-mode-foundation/01-03-SUMMARY.md` when done.
</output>

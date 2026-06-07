---
phase: 01-solidworks-mode-foundation
plan: 02
type: execute
wave: 2
depends_on: [01-01]
files_modified:
  - src/Gui/FreeWorks/FwNavigationDefault.h
  - src/Gui/FreeWorks/FwNavigationDefault.cpp
  - src/Gui/FreeWorks/CMakeLists.txt
  - src/Gui/FreeWorks/FwWorkbench.cpp
  - src/Gui/FreeWorks/MACOS_NAV_PROFILE.md
autonomous: false
requirements: [NAV-01]
must_haves:
  truths:
    - "SolidWorks mouse navigation is the FreeWorks default when the user has not chosen a nav style"
    - "Rotate = MMB drag, pan = Ctrl+MMB, zoom = scroll-wheel zoom-to-cursor, roll = Alt+MMB, dolly = Shift+MMB, middle-click-entity then middle-drag rotates about it"
    - "macOS has an explicit no-middle-button / trackpad profile (modifier-emulated MMB default, GestureNavigationStyle selectable in one click)"
    - "An existing user-chosen NavigationStyle is never clobbered (default applied only when key unset)"
  artifacts:
    - path: "src/Gui/FreeWorks/FwNavigationDefault.cpp"
      provides: "Reads the NavigationStyle param key; writes Gui::SolidWorksNavigationStyle when unset"
      contains: "NavigationStyle"
    - path: "src/Gui/FreeWorks/FwNavigationDefault.h"
      provides: "FwNavigationDefault::applyDefault() declaration"
      contains: "applyDefault"
    - path: "src/Gui/FreeWorks/MACOS_NAV_PROFILE.md"
      provides: "Documented macOS modifier-emulated-MMB profile + GestureNavigationStyle alternative + Mac-hardware spike checklist"
      contains: "GestureNavigationStyle"
  key_links:
    - from: "src/Gui/FreeWorks/FwWorkbench.cpp"
      to: "FwNavigationDefault::applyDefault"
      via: "activated() override"
      pattern: "FwNavigationDefault::applyDefault"
    - from: "src/Gui/FreeWorks/FwNavigationDefault.cpp"
      to: "User parameter:BaseApp/Preferences/View NavigationStyle"
      via: "GetParameterGroupByPath + SetASCII when unset"
      pattern: "Gui::SolidWorksNavigationStyle"
---

<objective>
Make SolidWorks mouse navigation the FreeWorks default by defaulting the existing `NavigationStyle` preference key to the upstream `Gui::SolidWorksNavigationStyle` when unset (the skeleton's persisted-state read/write round-trip), and define the macOS no-middle-button / trackpad substitute profile (OQ-3). NAV-01 is a preference default, not new navigation code — `Gui::SolidWorksNavigationStyle` already ships and is registered upstream.

Purpose: Satisfies NAV-01 — the fork navigates like SolidWorks out of the box on Windows/Linux, with an explicit, selectable macOS profile.
Output: `FwNavigationDefault` (read/write of the `NavigationStyle` key, marked `// SW-FORK HOOK`), wiring into `FwWorkbench::activated()`, and `MACOS_NAV_PROFILE.md` documenting the modifier-emulated-MMB default + `GestureNavigationStyle` alternative + the Mac-hardware verification checklist.
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
@.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md
</context>

<artifacts_produced>
## Artifacts this phase produces (Plan 02)

| Symbol / Path | Kind |
|---|---|
| `FreeWorksGui::FwNavigationDefault` | class with `static void applyDefault()` |
| `src/Gui/FreeWorks/FwNavigationDefault.{h,cpp}` | files |
| `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` | doc (OQ-3 profile + Mac spike checklist) |
| `SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle")  // SW-FORK HOOK` | shared-state write #2 of 2 (the only allowed "SolidWorks" string) |
</artifacts_produced>

<tasks>

<task type="auto">
  <name>Task 1: Implement FwNavigationDefault (default NavigationStyle to Gui::SolidWorksNavigationStyle when unset)</name>
  <read_first>
    - 01-PATTERNS.md section "FwNavigationDefault" — analog `src/Gui/View3DSettings.cpp:279-288` (param group `User parameter:BaseApp/Preferences/View`, key `NavigationStyle`, current default `CADNavigationStyle`); the verified write pattern `hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle")`.
    - 01-PATTERNS.md Shared Patterns "// SW-FORK HOOK" marker — this is the second (and final) unavoidable shared-state touch.
    - 01-CONTEXT.md D-03 (the ONLY permitted "SolidWorks" identifier in new code is the upstream type-name string `"Gui::SolidWorksNavigationStyle"`).
    - 01-SKELETON.md "Navigation (NAV-01)" decision row + Runtime State Inventory note A2 (set only when key unset).
  </read_first>
  <action>
    Create `src/Gui/FreeWorks/FwNavigationDefault.{h,cpp}` declaring `class FwNavigationDefault` in `namespace FreeWorksGui` with `static void applyDefault()`. `applyDefault()` reads the parameter group `User parameter:BaseApp/Preferences/View` via `App::GetApplication().GetParameterGroupByPath(...)`, reads the `NavigationStyle` key, and ONLY when it is unset (empty / not present) writes `hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle");` marked with a trailing `// SW-FORK HOOK` comment. Must NOT clobber an existing user-chosen value (note A2). On macOS (`#ifdef Q_OS_MACOS` or `defined(__APPLE__)`), additionally select the modifier-emulated-MMB profile path per MACOS_NAV_PROFILE.md (still defaulting NavigationStyle to the SolidWorks style, with the Mac modifier emulation as the substitute for the absent physical MMB). Add `FwNavigationDefault.{h,cpp}` to `src/Gui/FreeWorks/CMakeLists.txt` sources. Wire `FwNavigationDefault::applyDefault()` into `FwWorkbench::activated()` (call it alongside `FwLayout::install(...)`). This file is the ONLY place the "SolidWorks" string is allowed (D-03 upstream-type exception); no other "SolidWorks" identifier may appear.
  </action>
  <verify>
    <automated>grep -q "applyDefault" src/Gui/FreeWorks/FwNavigationDefault.h &amp;&amp; grep -q "Gui::SolidWorksNavigationStyle" src/Gui/FreeWorks/FwNavigationDefault.cpp &amp;&amp; grep -q "SW-FORK HOOK" src/Gui/FreeWorks/FwNavigationDefault.cpp &amp;&amp; grep -q "NavigationStyle" src/Gui/FreeWorks/FwNavigationDefault.cpp &amp;&amp; grep -q "FwNavigationDefault::applyDefault" src/Gui/FreeWorks/FwWorkbench.cpp</automated>
  </verify>
  <acceptance_criteria>
    - `FwNavigationDefault::applyDefault()` writes `Gui::SolidWorksNavigationStyle` to the `NavigationStyle` key only when unset.
    - The write line carries a `// SW-FORK HOOK` marker.
    - `FwWorkbench::activated()` calls `FwNavigationDefault::applyDefault()`.
    - `"Gui::SolidWorksNavigationStyle"` is the sole "SolidWorks" string in the module (confirmed by Plan 04 leak grep allow-list).
  </acceptance_criteria>
  <done>On first FreeWorks activation with no prior nav choice, the NavigationStyle preference round-trips to Gui::SolidWorksNavigationStyle (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB); an existing user choice is preserved.</done>
</task>

<task type="auto">
  <name>Task 2: Author the macOS navigation substitute profile + GestureNavigationStyle alternative</name>
  <read_first>
    - 01-SKELETON.md "macOS nav substitute (OQ-3)" decision row — modifier-emulated-MMB default preserves SW button semantics; expose `Gui::GestureNavigationStyle` as a one-click trackpad alternative; flag a Mac-hardware spike.
    - 01-CONTEXT.md "Claude's Discretion — macOS navigation substitute" (ROADMAP Phase 1 SC3 "explicit macOS no-middle-button / trackpad profile").
    - 01-PATTERNS.md section "FwNavigationDefault" macOS substitute note.
  </read_first>
  <action>
    Create `src/Gui/FreeWorks/MACOS_NAV_PROFILE.md` (SPDX header line 1) documenting: (1) the default macOS profile — modifier-emulated MMB mapping that preserves SolidWorks button semantics (state the exact modifier chord chosen, e.g. a documented key+left-drag emulating MMB-rotate, plus the Ctrl/Alt/Shift combos for pan/roll/dolly); (2) the one-click alternative — selecting `Gui::GestureNavigationStyle` for native trackpad gestures, and where the user toggles it; (3) the precise rotate/pan/zoom/roll/dolly mapping table for trackpad and 2-button-mouse Macs; (4) a "Mac-hardware spike" checklist enumerating what must be verified on real Apple hardware (rotate/pan/zoom feel, gesture conflicts, modifier ergonomics). The modifier chord chosen here is the value the user confirms at the manual checkpoint (Task 3).
  </action>
  <verify>
    <automated>test -f src/Gui/FreeWorks/MACOS_NAV_PROFILE.md &amp;&amp; grep -q "GestureNavigationStyle" src/Gui/FreeWorks/MACOS_NAV_PROFILE.md &amp;&amp; grep -qi "spike" src/Gui/FreeWorks/MACOS_NAV_PROFILE.md</automated>
  </verify>
  <acceptance_criteria>
    - `MACOS_NAV_PROFILE.md` documents the modifier-emulated-MMB default mapping, the `Gui::GestureNavigationStyle` one-click alternative, the full rotate/pan/zoom/roll/dolly table, and a Mac-hardware spike checklist.
  </acceptance_criteria>
  <done>The macOS no-middle-button / trackpad profile is fully documented and selectable, with a concrete spike checklist for real-hardware verification.</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 3: Manual Mac-hardware navigation verification (no-middle-button / trackpad)</name>
  <what-built>SolidWorks navigation is defaulted via FwNavigationDefault, and the macOS modifier-emulated-MMB profile (plus GestureNavigationStyle alternative) is documented in MACOS_NAV_PROFILE.md. CI proves the build; navigation feel on real Mac trackpad hardware cannot be automated.</what-built>
  <how-to-verify>
    1. On a real macOS machine (trackpad and/or 2-button mouse — no physical MMB), build FreeCAD with FreeWorks and launch it.
    2. Activate the FreeWorks workbench; open or create a document with a 3D view.
    3. Using the documented default modifier chord (see MACOS_NAV_PROFILE.md), confirm: rotate, pan (Ctrl-equivalent), zoom-to-cursor (scroll), roll (Alt-equivalent), dolly (Shift-equivalent) all behave with SolidWorks parity.
    4. Switch to `Gui::GestureNavigationStyle` via the documented one-click path and confirm native trackpad gestures work as the alternative.
    5. Walk the Mac-hardware spike checklist in MACOS_NAV_PROFILE.md; record any gesture conflicts or modifier-ergonomics issues.
  </how-to-verify>
  <resume-signal>Type "approved" if SW-parity feel holds and the documented modifier chord is confirmed; otherwise describe the mapping changes needed and update MACOS_NAV_PROFILE.md.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| fork -> upstream preference store | Writing the NavigationStyle key must not clobber user state or drift from upstream key path |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-01-04 | Tampering | `NavigationStyle` preference write | mitigate | Write only when key unset (note A2); single line marked `// SW-FORK HOOK`; greppable by Plan 04 sync drill |
| T-01-05 | Repudiation/Legal | "SolidWorks" string usage | mitigate | Only the upstream type-name string `"Gui::SolidWorksNavigationStyle"` is used (D-03 exception); Plan 04 leak grep allow-lists exactly this token |
| T-01-06 | Denial of Service | macOS missing-MMB UX | accept | Low risk; modifier-emulated MMB + GestureNavigationStyle alternative documented; verified on real hardware at the blocking checkpoint |
</threat_model>

<verification>
- `NavigationStyle` round-trips to `Gui::SolidWorksNavigationStyle` on first activation when unset (asserted by Plan 03 GTest `NavigationStyle` default test).
- Existing user nav choice preserved (not clobbered).
- macOS profile documented + selectable; real-hardware feel confirmed at the blocking checkpoint.
</verification>

<success_criteria>
- SolidWorks navigation active by default in FreeWorks (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB, middle-click-entity rotate-about).
- Explicit macOS no-middle-button / trackpad profile documented and selectable; Mac-hardware spike verified.
</success_criteria>

<output>
Create `.planning/phases/01-solidworks-mode-foundation/01-02-SUMMARY.md` when done.
</output>

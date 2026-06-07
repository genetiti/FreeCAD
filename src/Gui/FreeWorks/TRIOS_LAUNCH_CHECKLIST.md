<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# FreeWorks Tri-OS Launch Verification Checklist (SHELL-02)

This checklist covers the parts of **SHELL-02** that CI cannot fully automate:
the GUI launch and the *feel* of the reference-CAD-style dock shell mounting on
**Windows, macOS, and Linux**. The per-OS **compile** is proven by the existing
CI matrix (`sub_buildWindows.yml` / `sub_buildUbuntu.yml` / `sub_buildPixi.yml`);
the **App/Gui separation** invariant is proven headlessly by
`sub_fwHeadlessCompat.yml`. What remains is a human confirming the shell mounts
cleanly and the navigation default is active on real hardware on all three OSes.

> Naming: the workbench label is **"FreeWorks"** (D-01). The only permitted
> upstream reference-CAD identifier anywhere is the navigation-style type string
> `Gui::SolidWorksNavigationStyle` (D-03).

## What is verified automatically (do NOT re-do these manually)

| Invariant | Where it is proven |
|-----------|--------------------|
| FreeWorks subdirectory compiles on Windows | `sub_buildWindows.yml` (CI matrix) |
| FreeWorks subdirectory compiles on Ubuntu/Linux | `sub_buildUbuntu.yml` (CI matrix) |
| FreeWorks subdirectory compiles on Pixi/macOS toolchain | `sub_buildPixi.yml` (CI matrix) |
| FwWorkbench registers + `Fw_*` docks + NavigationStyle default | `tests/src/Gui/FwWorkbench.cpp` GTest (`Gui_tests_run`) |
| No App-layer GUI-state leak (`.FCStd` round-trip) | `sub_fwHeadlessCompat.yml` (`HEADLESS_OK`) |

## What this checklist verifies manually (per OS)

For **each** of Windows, macOS, and Linux:

1. **Build** FreeCAD with the FreeWorks Gui submodule linked.
2. **Launch** the GUI binary (not `FreeCADCmd`).
3. **Activate** the **FreeWorks** workbench from the workbench selector.
4. Confirm the reference-CAD-style dock shell mounts in the correct geometry:
   - `Fw_FeatureManager` docked on the **left**.
   - `Fw_PropertyManager` docked on the **left** (below/with FeatureManager).
   - `Fw_TaskPane` reserved on the **right**.
   - The **top toolbar area is reserved/free** for the Phase 2 ribbon.
5. Confirm **no crash** on activation and on workbench switch (in and back out).
6. Confirm the **reference-CAD navigation default is active** (rotate = MMB drag,
   pan = Ctrl+MMB, zoom-to-cursor) on a fresh profile where `NavigationStyle`
   was previously unset; confirm an existing user choice is **not** clobbered.

---

## Per-OS build + launch commands

### Linux (Ubuntu / Pixi)

```bash
# Build (conda/pixi preset)
cmake --preset conda-linux-release
cmake --build --preset conda-linux-release
# Launch the GUI
./build/conda-linux-release/bin/FreeCAD
```

### Windows

```powershell
# Build (conda/pixi preset)
cmake --preset conda-windows-release
cmake --build --preset conda-windows-release
# Launch the GUI
.\build\conda-windows-release\bin\FreeCAD.exe
```

### macOS

```bash
# Build (conda/pixi preset)
cmake --preset conda-macos-release
cmake --build --preset conda-macos-release
# Launch the GUI
./build/conda-macos-release/bin/FreeCAD
```

> macOS note: the default 3-button-mouse model is substituted by the
> modifier-emulated-MMB profile documented in `MACOS_NAV_PROFILE.md`
> (Option + left-drag). Verify navigation against that profile on macOS, not
> against a literal MMB.

---

## Sign-off table

Fill in one row per OS. The CI-matrix column records that the per-OS build job
is green for the branch under test.

| OS | Branch | Build (CI matrix green) | GUI launches | FreeWorks activates | `Fw_FeatureManager` (left) | `Fw_PropertyManager` (left) | `Fw_TaskPane` (right) | Top toolbar area reserved | No crash (activate + switch) | Nav default active (no-clobber) | Verifier | Date |
|----|--------|-------------------------|--------------|---------------------|----------------------------|------------------------------|-----------------------|---------------------------|------------------------------|---------------------------------|----------|------|
| Windows | | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | | |
| macOS   | | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ (per `MACOS_NAV_PROFILE.md`) | | |
| Linux   | | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | ☐ | | |

## CI matrix confirmation

- [ ] `sub_buildWindows.yml` is green for the branch.
- [ ] `sub_buildUbuntu.yml` is green for the branch.
- [ ] `sub_buildPixi.yml` is green for the branch.
- [ ] `sub_fwHeadlessCompat.yml` is green for the branch (`HEADLESS_OK`, no leak).

## Resume signal

When the shell mounts cleanly and CI is green on all three OSes, the executor's
Task 3 checkpoint is satisfied: reply **"approved"**. Otherwise list the failing
OS and the specific symptom (which dock did not mount, crash trace, nav default
not applied, or which CI job is red).

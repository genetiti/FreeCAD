# Phase 1: SolidWorks Mode Foundation - Research

**Researched:** 2026-06-06
**Domain:** Additive FreeCAD Qt6/C++20 Gui module (`src/Gui/FreeWorks/`) + `FwWorkbench` activation seam, SolidWorks navigation default, upstream-merge + asset-provenance + headless-compat CI discipline
**Confidence:** HIGH (every integration seam verified by direct read of this checkout; non-functional gates verified against existing CI/test infrastructure)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Working product name is **FreeWorks** — a SolidWorks-evocative but legally distinct coinage. Treat as a **working name**; a real trademark search must clear it before public distribution (Phase 7 legal checkpoint).
- **D-02:** Code/module naming uses the **`Fw`** prefix, superseding the research's `Sw`/`src/Gui/SolidWorks/` convention everywhere downstream:
  - Directory: `src/Gui/FreeWorks/`
  - Classes: `FwWorkbench`, `FwLayout`, `FwTheme`, `FwRibbon`, `FwFeatureManager`, `FwPropertyManager`, etc.
  - Namespace: `FreeWorksGui`
  - The workbench shown in FreeCAD's workbench selector is labeled **"FreeWorks"**.
- **D-03:** The string "SolidWorks" (and any SolidWorks wordmark/logo) must appear in **neither** user-facing surfaces (window title, About box, workbench label, installer, menu/UI strings) **nor** code identifiers/paths. Internal descriptive comments referencing SolidWorks *behavior* are fine; identifiers are not.
  - **Exception (verified):** FreeCAD already ships `Gui::SolidWorksNavigationStyle` (upstream class). Do NOT rename it; default to it via preference. The trademark-avoidance rule applies to *new* FreeWorks code, not to referencing the existing upstream class.

### Claude's Discretion (resolve technically; surface recommendations at plan review)

- **Activation & coexistence model:** switchable `FwWorkbench` alongside stock FreeCAD (merge-safe default) vs. boot-straight-into-FreeWorks with classic chrome hidden. *Lean:* implement the `FwWorkbench` activation seam regardless; auto-activate-at-startup is a product toggle to confirm. → **Recommendation in §Open Questions OQ-1.**
- **Phase-1 shell contents:** labeled placeholder docks establishing the SW dock layout vs. dock skeleton reusing FreeCAD's current tree/combo temporarily. → **Recommendation in §Open Questions OQ-2.**
- **macOS navigation substitute:** modifier-emulated MMB vs. trackpad gestures for Mac trackpad / 2-button mice (ROADMAP Phase 1 SC3 "explicit macOS no-middle-button / trackpad profile"). → **Recommendation in §Open Questions OQ-3.**

### Deferred Ideas (OUT OF SCOPE)

None deferred. The three gray areas above are in-scope for this phase. Out-of-phase items (ribbon=P2, tree=P3, PropertyManager=P4, selection/canvas=P5, gestures/Instant3D=P6, theme/Task Pane/legal=P7) are owned by later phases.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SHELL-01 | User can launch FreeCAD into a "SolidWorks mode" that applies the full SW-style interface as one coherent layout | `FwWorkbench` (subclass `Gui::Workbench`) drives `setupDockWindows()`/`setupToolBars()`/`setupMenuBar()` + `activated()` → `FwLayout::install()`. Phase 1 delivers the **coherent dock shell** (entry seam); ribbon/tree/PropertyManager mount in later phases. (§Architecture Patterns P1; §Code Examples) |
| SHELL-02 | The SW-style interface builds and runs on Windows, macOS, and Linux | Native Qt6 Widgets only (no new deps) → cross-platform by construction; verified via existing `sub_buildWindows.yml` / `sub_buildUbuntu.yml` / `sub_buildPixi.yml` CI matrix + manual launch. (§Standard Stack; §Validation Architecture SC2) |
| NAV-01 | SolidWorks mouse navigation is the default (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB, click-entity-then-MMB-drag rotate-about) — with explicit macOS no-middle-button / trackpad profile | **Already implemented in-tree:** `Gui::SolidWorksNavigationStyle`. NAV-01 reduces to setting the `NavigationStyle` preference default to `Gui::SolidWorksNavigationStyle` (currently defaults to `CADNavigationStyle`). macOS substitute = §Open Questions OQ-3. (§Architecture Patterns P3; §Code Examples) |
</phase_requirements>

## Summary

This phase is overwhelmingly an **assembly + discipline** exercise, not a from-scratch build. Direct reads of this checkout confirm that the two functional deliverables (a registrable SW-mode shell and SolidWorks navigation as the default) ride existing, stable FreeCAD extension seams: the `Gui::Workbench` activation machinery installs the dock shell with zero `MainWindow.cpp` body edits, and `Gui::SolidWorksNavigationStyle` already ships fully implemented — NAV-01 is a one-line preference default. The entire FreeWorks module lives under `src/Gui/FreeWorks/`, talks to the frozen App DOM only through public Gui singletons, and touches shared files at exactly two marked seams: one `add_subdirectory` in `src/Gui/CMakeLists.txt` and one navigation-default preference. Both are conflict-resistant and greppable via `// SW-FORK HOOK`.

The harder, higher-value work in Phase 1 is the **non-functional foundation** that lets every later phase stay mergeable: the `// SW-FORK HOOK` discipline, an `ASSET_PROVENANCE.md` + CI guard, a headless/`--console` `.FCStd`-compat gate proving no Gui state leaks into App, and a scripted upstream-sync drill against a pinned commit. None of these exist yet in the repo (verified: no `ASSET_PROVENANCE.md`, no `SW-FORK HOOK` markers) — this is greenfield scaffolding, and it is the phase's actual risk surface. The three "Claude's Discretion" gray areas (coexistence model, shell contents, macOS nav substitute) all have a clear recommended resolution grounded in verified code paths (notably: FreeCAD already ships a touch/trackpad-oriented `GestureNavigationStyle` and grabs `Qt::PinchGesture`, giving a concrete macOS fallback).

**Primary recommendation:** Build `src/Gui/FreeWorks/` as a self-contained Gui submodule registering an `FwWorkbench` (`Gui::Workbench` subclass, label "FreeWorks") that installs a coherent placeholder dock shell via `FwLayout`; default the `NavigationStyle` preference to `Gui::SolidWorksNavigationStyle`; and stand up the four CI/discipline gates (provenance guard, headless `.FCStd` gate, `// SW-FORK HOOK` convention, scripted upstream-sync drill) as first-class, tested deliverables — touching shared files at no more than two marked seams.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| SW-mode activation / dock shell (SHELL-01) | Gui (`src/Gui/FreeWorks/`) | — | Pure presentation; `Gui::Workbench` owns layout. Zero App-layer changes (project hard invariant). |
| Dock window registration | Gui (`DockWindowManager`) | — | New dock *names* (e.g. `Fw_FeatureManager`) registered additively; avoids editing `Std_TreeView`/`Std_ComboView` blocks in `MainWindow.cpp`. |
| SolidWorks navigation (NAV-01) | Gui (existing `SolidWorksNavigationStyle`) | — | Already implemented + registered upstream; selected by `NavigationStyle` preference. No new nav code. |
| macOS / no-MMB nav substitute | Gui (nav style + viewer gesture grab) | — | Substitute defined over existing `GestureNavigationStyle` / `Qt::PinchGesture` + modifier-emulated MMB; still Gui-only. |
| Cross-platform build/run (SHELL-02) | Build (CMake) + CI matrix | Gui | Native Qt6 Widgets = cross-platform by construction; proven by existing per-OS CI workflows. |
| Asset-provenance guard | CI / repo tooling | — | Repo-level discipline (pre-commit + CI), not application code. |
| Headless `.FCStd`-compat gate | CI / test (`--console`, GTest) | App boundary check | Proves Gui-only changes don't pollute App DOM / `.FCStd`; runs against `MainCmd` + existing test path. |
| Upstream-sync drill | Repo tooling / Git workflow | — | Scripted Git workflow against a pinned upstream commit; no application code. |

## Standard Stack

### Core (all already present in FreeCAD — extend, never replace)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Widgets | 6.8.x (FreeCAD pin) | All FreeWorks chrome: dock shell, placeholder panels | FreeCAD's entire Gui is QtWidgets; native = zero new deps, cross-platform, merge-friendly `[VERIFIED: codebase grep — no ribbon/QML deps]` |
| `Gui::Workbench` | FreeCAD `main` | Base class for `FwWorkbench`; `activate()` calls `setupDockWindows()`/`setupToolBars()`/`setupMenuBar()` | The activation seam; installs the shell with no `MainWindow` startup edits `[VERIFIED: src/Gui/Workbench.h:89,117-123]` |
| `Gui::DockWindowManager` | FreeCAD `main` | Register new FreeWorks dock names additively | New dock names avoid touching churned `MainWindow.cpp` blocks `[CITED: .planning/research/ARCHITECTURE.md]` |
| `Gui::SolidWorksNavigationStyle` | FreeCAD `main` | SolidWorks mouse rotate/pan/zoom (NAV-01) | **Already shipped + registered**; selected via `NavigationStyle` preference `[VERIFIED: src/Gui/Navigation/SolidWorksNavigationStyle.cpp; NavigationStyle.h:456]` |
| `Gui::GestureNavigationStyle` | FreeCAD `main` | Touch/trackpad navigation — macOS no-MMB fallback candidate | Trackpad/pinch-oriented style already registered; viewer grabs `Qt::PinchGesture` `[VERIFIED: GestureNavigationStyle.cpp:954; View3DInventorViewer.cpp:718]` |
| `View3DSettings` / `NavigationStyle` preference | FreeCAD `main` | Applies nav-style default per viewer | Preference path `User parameter:BaseApp/Preferences/View`, key `NavigationStyle`, current default `CADNavigationStyle` `[VERIFIED: View3DInventor.cpp:233; View3DSettings.cpp:279-288]` |
| `Gui::ToolBarAreaWidget` / `ToolBarArea::TopToolBarArea` | FreeCAD `main` | Future ribbon mount point (P2) — Phase 1 only reserves the layout | The ribbon mounts here later; shell must leave the top area available `[CITED: .planning/research/STACK.md]` |
| `StyleParameters` + `FreeCAD.qss` + `PreferencePackManager` | FreeCAD `main` | Theming substrate for `FwTheme` (minimal in P1) | Parametric QSS + shippable preference packs `[VERIFIED: src/Gui/PreferencePackManager.{h,cpp} present]` |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt SVG (`QtSvg`) | 6.8.x | Scalable look-alike icons via `BitmapFactory::pixmapFromSvg()` | Theme work — mostly Phase 7; Phase 1 needs at most a workbench icon `[CITED: .planning/research/STACK.md]` |
| GTest (`Gui_tests_run`) | bundled | C++ unit tests for `FwWorkbench` registration / layout invariants | The `tests/src/Gui/` target already exists and links `FreeCADGui` `[VERIFIED: tests/src/Gui/CMakeLists.txt]` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Native Qt Widgets shell | SARibbon (MIT) | Not needed in Phase 1 (no ribbon yet); a P2 decision. Adds a vendored dependency. |
| `FwWorkbench` activation seam | In-place `MainWindow.cpp` startup edits | Rejected — deep `MainWindow` edits are the #1 merge-hell anti-pattern (Pitfall 3). |
| Default nav via preference | New nav-style subclass | Rejected — `SolidWorksNavigationStyle` already exists; subclassing/reimplementing diverges from upstream (Anti-Pattern 2). |

**Installation:** No new package-manager entries. Everything is already in `pixi.toml` / the FreeCAD build. The only build-system change is one `add_subdirectory(FreeWorks)` line in `src/Gui/CMakeLists.txt` (marked `// SW-FORK HOOK`, or `# SW-FORK HOOK` in CMake).

## Package Legitimacy Audit

Not applicable — this phase installs **no external packages**. All facilities are pre-existing FreeCAD/Qt6 internals already vetted in the upstream build. No npm/PyPI/crates/submodule additions in Phase 1. (SARibbon vendoring, if ever adopted, is a Phase 2 decision and would require its own audit.)

## Architecture Patterns

### System Architecture Diagram

```
[User selects "FreeWorks" in workbench combo]   [App startup, optional auto-activate toggle]
                │                                          │
                ▼                                          ▼
      Gui::WorkbenchManager ──activates──►  FwWorkbench (Gui::Workbench subclass)
                                                  │
            ┌─────────────────────────────────────┼───────────────────────────────────┐
            ▼ setupDockWindows()                   ▼ activated()                        ▼ setupToolBars()/MenuBar()
   DockWindowItems (Fw_* dock names)      FwLayout::install(getMainWindow())     ToolBarItem/MenuItem trees
            │                                  │  (reserve TopToolBarArea for P2 ribbon;       (reuse Std command set;
            ▼                                  │   apply coherent left/right dock layout)       ribbon renders them in P2)
   DockWindowManager::registerDockWindow ──────┘
                                                  │ FwTheme (minimal QSS / workbench icon)
                                                  ▼
                          ┌─────────── Coherent SW-style dock shell (placeholders) ───────────┐
                          │  left placeholder (future FeatureManager+PropertyManager)          │
                          │  top area reserved (future ribbon)   right reserved (future Pane)  │
                          └────────────────────────────────────────────────────────────────────┘

[3D viewport]  ── reads ──►  NavigationStyle preference ("User parameter:BaseApp/Preferences/View" → key "NavigationStyle")
                                   │ default flipped: CADNavigationStyle → Gui::SolidWorksNavigationStyle   // SW-FORK HOOK
                                   ▼
                      View3DInventorViewer::setNavigationType(Gui::SolidWorksNavigationStyle)
                                   │ (macOS: GestureNavigationStyle / PinchGesture + modifier-emulated MMB — OQ-3)
                                   ▼
                          SolidWorks mouse rotate/pan/zoom/roll/dolly (NAV-01)

   ── No App-layer arrow exists: FreeWorks never writes App DOM except via Gui::Command / property setters / Control().accept() ──
```

### Recommended Project Structure

```
src/Gui/FreeWorks/                  # NEW — entire FreeWorks presentation layer (namespace FreeWorksGui)
├── FwWorkbench.{h,cpp}             # Gui::Workbench subclass = "FreeWorks mode" entry point (label "FreeWorks")
├── FwLayout.{h,cpp}                # applies the coherent dock shell into MainWindow on activate()
├── FwTheme.{h,cpp}                 # minimal QSS hook + workbench icon (full theme = Phase 7)
├── FwNavigationDefault.{h,cpp}     # helper: ensure NavigationStyle pref defaults to SolidWorksNavigationStyle
├── PreCompiled.{h,cpp}             # matches FreeCAD module PCH convention
├── Resources/                      # workbench icon (recreated, original art); .qrc
└── CMakeLists.txt                  # built via one add_subdirectory in src/Gui/CMakeLists.txt  # SW-FORK HOOK

# Repo-root non-functional foundation (NOT application code):
ASSET_PROVENANCE.md                 # NEW — maps every binary image → original source (CI-enforced)
tools/fw-sync-upstream.sh           # NEW — scripted upstream-sync drill against a pinned commit
.github/workflows/ (extend)         # NEW jobs: provenance guard + headless .FCStd-compat gate
```

> Apply D-02 `Fw` naming as a find/replace over the research's `Sw*` examples — they are otherwise correct.

### Pattern 1: Additive Module + Workbench Activation (the mergeability backbone)

**What:** Implement FreeWorks as a new `Gui::Workbench` subclass overriding `setupDockWindows()` (and `setupToolBars()`/`setupMenuBar()` reusing the Std command set), plus an `activated()` override that installs the dock shell via `FwLayout`.
**When to use:** Primary integration mechanism for everything expressible as "what UI shows up."
**Trade-offs:** Pro — rides stable extension points, survives upstream churn, cleanly toggleable. Con — Phase 1 only delivers the *shell*; ribbon/tree/panels are later phases consuming the reserved areas.

```cpp
// Source: VERIFIED against src/Gui/Workbench.h:89,117-123 (this checkout)
// FwWorkbench.cpp — namespace FreeWorksGui
DockWindowItems* FwWorkbench::setupDockWindows() const {
    auto* root = StdWorkbench::setupDockWindows();   // reuse stock dock set as base
    // register Fw_* placeholder docks via DockWindowManager (see Pattern 4)
    return root;
}
void FwWorkbench::activated() {
    FwLayout::install(Gui::getMainWindow());          // apply coherent SW dock layout; reserve TopToolBarArea
    StdWorkbench::activated();
}
```

### Pattern 2: Observe-the-DOM, Never-Mutate-Directly

**What:** Every FreeWorks widget is a *view* subscribing to existing signals; it writes only via `Gui::Command`, property setters, or `Control().accept()`.
**When to use:** Always — the project's hard App/Gui invariant.
**Trade-offs:** Pro — undo/redo, recompute, dependency tracking keep working for free; no App edits. Con — must wire observers in `attach`/`detach`. (Phase 1's shell has little state, but the discipline must be established here.)

### Pattern 3: Default the Existing Nav Style (NAV-01 with zero nav code)

**What:** Flip the `NavigationStyle` preference default from `CADNavigationStyle` to `Gui::SolidWorksNavigationStyle`. The viewer resolves the type by name and instantiates it.
**When to use:** NAV-01 in full.
**Trade-offs:** Pro — no new nav code, rides maintained upstream class. Con — the *default-setting* mechanism must not edit `View3DSettings.cpp`'s fallback in place (that's a shared churned file); set the user/system parameter instead, or contribute a generic default-override hook upstream. Mark any unavoidable touch `// SW-FORK HOOK`.

```cpp
// Source: VERIFIED — View3DSettings.cpp:279-288 reads key "NavigationStyle"
//         from "User parameter:BaseApp/Preferences/View", default CADNavigationStyle type name.
// FreeWorks sets the default to the SolidWorks style's fully-qualified type name:
auto hGrp = App::GetApplication()
    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
// type name string is the C++ qualified name registered via TYPESYSTEM_SOURCE:
hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle");   // SW-FORK HOOK
```

### Pattern 4: New Dock Names, Not Edits to Existing Dock Blocks

**What:** Register FreeWorks docks under *new* names (`Fw_FeatureManager`, `Fw_PropertyManager`, …) via `DockWindowManager::registerDockWindow()` rather than editing the `Std_TreeView`/`Std_ComboView` setup blocks in `MainWindow.cpp`.
**When to use:** All FreeWorks dock registration.
**Trade-offs:** Pro — new names never collide with upstream's churned dock-setup code. Con — Phase 1 placeholders must be registered under their permanent Fw names so later phases swap content, not names.

### Anti-Patterns to Avoid

- **Deep-editing `MainWindow.cpp`/`Tree.cpp`/`ComboView` in place:** upstream's most-churned files → permanent merge hell. Use Workbench subclass + new dock names. (Pitfall 3)
- **Reimplementing a navigation style from scratch:** `SolidWorksNavigationStyle` already exists and is registered. Default to it; contribute gaps upstream. (Pitfall 2 / Anti-Pattern 2)
- **Pushing any UI state into the App layer:** breaks headless mode, pollutes `.FCStd`. Keep all FreeWorks state in Gui. The headless gate (SC4) exists to catch this. (Pitfall 4)
- **`#include` of Gui headers under `src/App/`:** instant App/Gui boundary breach. The whole module is `src/Gui/`-only.
- **Scattering unmarked shared-file edits:** every unavoidable shared touch must carry `// SW-FORK HOOK` so it's greppable for the sync drill (SC5).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SolidWorks mouse navigation | A new Coin3D event handler | `Gui::SolidWorksNavigationStyle` (default via preference) | Already implemented, registered, maintained upstream `[VERIFIED]` |
| macOS trackpad / no-MMB nav | A bespoke gesture recognizer | `Gui::GestureNavigationStyle` + viewer's `Qt::PinchGesture` grab as the base | FreeCAD already grabs pinch and ships a touch nav style `[VERIFIED: View3DInventorViewer.cpp:718]` |
| Mounting UI into the main window | In-place `MainWindow` startup edits | `Gui::Workbench::activated()` + `DockWindowManager` | The supported, low-conflict install seam `[VERIFIED: Workbench.h]` |
| Dock layout persistence | Custom save/restore | `QMainWindow` + `DockWindowManager` state save/restore | Native, already wired `[CITED: .planning/research/STACK.md]` |
| Command set for future ribbon | A parallel command registry | `StdWorkbench::setupToolBars()`/`setupMenuBar()` trees consumed as data | No duplicated command backend |
| Theming engine | Hard-coded `setStyleSheet()` | `StyleParameters` + QSS preference pack | Themeable, user-switchable, merge-friendly `[VERIFIED: PreferencePackManager present]` |

**Key insight:** In this phase, "hand-rolling" almost always means "diverging from a maintained upstream facility," which is exactly what creates merge hell. The default posture is *find the existing seam*.

## Common Pitfalls

### Pitfall 1: Deep edits to `MainWindow`/shared `src/Gui/*` → permanent merge hell
**What goes wrong:** Installing the shell by rewriting `MainWindow.cpp` (or `Tree.cpp`, `View3DInventorViewer.cpp`). Upstream churns these constantly (`1.2.0-dev` moving target); every sync becomes a multi-day battle and the fork freezes on an old base.
**Why it happens:** In-place edits are the path of least resistance.
**How to avoid:** Additive module + Workbench subclass + new dock names; concentrate unavoidable touches into single, `// SW-FORK HOOK`-marked hook lines; sync small and often against a pinned commit; keep an untouched upstream mirror branch.
**Warning signs:** One sync conflicts in >a handful of files or takes >a day; unmarked scattered edits; devs avoid syncing.

### Pitfall 2: UI state leaking into the App layer (breaks headless + pollutes `.FCStd`)
**What goes wrong:** A FreeWorks behavior stores state on `App::Document`/`App::DocumentObject`, or App code `#include`s Gui. Breaks `--console` operation, pollutes the `.FCStd` format, diverges App from upstream.
**Why it happens:** State "feels like model data," so the nearest object is an App object.
**How to avoid:** All FreeWorks state in Gui (controllers, ViewProvider props). The SC4 headless gate is the detector — a Gui-only change must never break `--console` load/recompute, and a FreeWorks-saved `.FCStd` must open in upstream FreeCAD.
**Warning signs:** New properties on App subclasses; Gui `#include`s under `src/App/`; headless tests fail after a "pure UI" change.

### Pitfall 3: Windows-only navigation feel; macOS has no middle button
**What goes wrong:** Nav is verified only on Windows; ships unusable on macOS (no MMB, trackpad gestures, Cmd vs Ctrl) and possibly Linux.
**Why it happens:** Audience is Windows-heavy; per-platform input deltas are easy to forget.
**How to avoid:** Define the macOS substitute up front (OQ-3) — modifier-emulated MMB and/or `GestureNavigationStyle`/pinch; test rotate/pan/zoom on all three OSes this phase, not at the end. `SolidWorksNavigationStyle::mouseButtons()` is bare MMB/Ctrl+MMB with **no built-in MMB-emulation** (verified: no `emulate`/`TwoButton` code in `NavigationStyle.cpp`) — the substitute is genuinely new work.
**Warning signs:** Nav code assumes MMB always present; no macOS/Linux nav testing; Mac users can't rotate.

### Pitfall 4: "SolidWorks" string/identifier leaking into new FreeWorks surfaces
**What goes wrong:** The word "SolidWorks" appears in a new UI string, identifier, path, About box, or installer — violating D-03 and creating trademark exposure.
**Why it happens:** Copy-paste from research/examples that used `Sw`/SolidWorks naming.
**How to avoid:** Enforce `Fw`/FreeWorks naming everywhere new; a lint/CI grep that fails on "SolidWorks" in new `src/Gui/FreeWorks/` identifiers and user-facing strings (allow-list the one legitimate reference to the upstream `Gui::SolidWorksNavigationStyle` type name).
**Warning signs:** "SolidWorks" in a new identifier/path/string outside the single nav-style reference.

### Pitfall 5: Asset provenance not enforced from day one
**What goes wrong:** A binary image lands with no source entry; later it's impossible to prove non-infringement, and a proprietary asset may slip into git history permanently.
**Why it happens:** Provenance feels like a Phase 7 (theme) concern, but the *guard* must exist before any image lands.
**How to avoid:** Create `ASSET_PROVENANCE.md` and the CI guard in Phase 1 (SC4), before the first FreeWorks icon. The guard rejects any binary image lacking a provenance entry.
**Warning signs:** A commit adds binary images with no provenance row; CI guard not yet wired.

## Code Examples

### Registering the FreeWorks workbench (C++ path)
```cpp
// Source: pattern verified against Gui::Workbench API (src/Gui/Workbench.h) + module registration model
// namespace FreeWorksGui — label shown in the workbench selector is "FreeWorks" (D-02/D-03)
// FwWorkbench is registered like any module workbench via the Gui workbench factory/addWorkbench path.
// Phase 1 deliverable: registration + activation install a coherent placeholder dock shell.
```

### Defaulting SolidWorks navigation (NAV-01)
```cpp
// Source: VERIFIED — View3DSettings.cpp:279-288 + View3DInventorViewer::setNavigationType (1696-1720)
auto hGrp = App::GetApplication()
    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle");   // SW-FORK HOOK
// The viewer's View3DSettings observer reacts to the "NavigationStyle" key change and calls
// setNavigationType(), which resolves the type by name and instantiates SolidWorksNavigationStyle.
```

### Headless `.FCStd`-compat gate (SC4 — concept)
```bash
# Source: verified MainCmd.cpp exists; pattern per .planning/codebase/TESTING.md
# 1. (GUI build) create + save a document while FreeWorks mode is active → fw_doc.FCStd
# 2. (console build) load it headless; assert recompute succeeds and no FreeWorks/Gui keys are present:
FreeCADCmd -c "import FreeCAD; d=FreeCAD.openDocument('fw_doc.FCStd'); d.recompute(); \
  assert d.Objects, 'no objects'; print('HEADLESS_OK')"
# 3. Assert the saved .FCStd opens in an UNMODIFIED upstream FreeCAD build (format not polluted).
```

## Runtime State Inventory

> Phase 1 is **greenfield** (a new additive module + new CI scaffolding). There is no rename/refactor/migration of existing stored state. The one configuration default that changes is the `NavigationStyle` preference key — a new default value, not a migration of existing user data.

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — no databases/datastores keyed on a renamed string. Verified: phase adds a new module, edits no persisted records. | None |
| Live service config | None — desktop app, no external services. | None |
| OS-registered state | None — no OS task/service registration in this phase. | None |
| Secrets/env vars | None — no secrets touched. | None |
| Build artifacts | New `src/Gui/FreeWorks/` build output (fresh); one `add_subdirectory` in `src/Gui/CMakeLists.txt`. No stale artifacts to purge (new code). | Build via standard CMake reconfigure |
| Config defaults (note) | `NavigationStyle` preference default value (`CADNavigationStyle` → `Gui::SolidWorksNavigationStyle`). This is a *new default*, applied only when the key is unset; it does not rewrite existing user preferences. | Set default via parameter / `// SW-FORK HOOK`; document so users who set their own nav style keep it |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Edit `MainWindow` to inject custom chrome | Additive Workbench + `WorkbenchManipulator` + `ToolBarAreaWidget` custom areas | `ToolBarAreaWidget` added 2024; `WorkbenchManipulator` 2023 | Custom UI can be additive — the basis for staying mergeable `[CITED: .planning/research/STACK.md]` |
| Per-widget `setStyleSheet()` | `StyleParameters` parametric QSS + preference packs | Recent (2026) | Themeable, switchable theme delivery (Phase 7 mostly) |
| Qt5 / Qt5-compat APIs | Qt 6.8 LTS Widgets only | Qt5 deprecated ~2026-08 (per codebase STACK) | Use Qt 6.8 only in new FreeWorks code |

**Deprecated/outdated:**
- Qt5-compat APIs — do not use in new FreeWorks code.
- QxRibbon (if ever considered for P2) — upstream UNMAINTAINED; SARibbon is the maintained MIT option.

## Project Constraints (from CLAUDE.md)

- **Tech stack hard-pin:** C++20 / Qt 6.8 / Coin3D / OCCT 7.8. Respect App/Gui separation and the Workbench plugin pattern. Don't bypass Property change notifications or modify the document during recompute.
- **No App-layer changes:** the entire FreeWorks module lives under `src/Gui/`.
- **Single-threaded GUI:** Qt event loop is single-threaded; long operations stay off the GUI thread (existing task-panel/worker pattern). (Minimal relevance in Phase 1's shell.)
- **Fork mergeability:** favor additive/overriding Gui components over deep edits to shared code.
- **Legal:** no verbatim SolidWorks proprietary assets; recreated look-alike art only.
- **Naming/style:** Headers `.h`, impl `.cpp`, `PascalCase` classes, namespaces `PascalCase` (here `FreeWorksGui`), private members `_lowerCamelCase`, methods `camelCase`. clang-format (LLVM base, 4-space, 100-col) enforced via pre-commit. SPDX `LGPL-2.1-or-later` header required on every new file.
- **GSD workflow:** file changes must go through a GSD command; pre-commit + `sub_lint.yml` must pass.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The `NavigationStyle` type-name string accepted by the preference is the fully-qualified C++ name `Gui::SolidWorksNavigationStyle` (as produced by `TYPESYSTEM_SOURCE`/`getName()`). | Pattern 3 / Code Examples | Low — verified `setNavigationType` resolves by `type.getName()` and the fallback default uses `CADNavigationStyle::getClassTypeId().getName()`; the SW name follows the same convention. Confirm at plan time by reading the exact `getName()` output if any doubt. |
| A2 | Setting the default at the user-parameter layer (rather than editing `View3DSettings.cpp`'s in-code fallback) is sufficient to make SW nav the default for FreeWorks users without a shared-file edit. | Pattern 3 | Medium — if the product wants SW nav as the *system* default for all FreeWorks installs (not just first-run user param), a small marked hook or a preference-pack default may be needed. Decide at plan review (ties to OQ-1). |
| A3 | `GestureNavigationStyle` + the viewer's `Qt::PinchGesture` grab is a viable base for the macOS substitute without new App-layer code. | OQ-3 / Don't Hand-Roll | Medium — gesture style is trackpad-oriented but may not 1:1 map SW rotate/pan/roll/dolly; spike on a Mac early. |
| A4 | The existing per-OS CI workflows (`sub_buildWindows.yml`, `sub_buildUbuntu.yml`, `sub_buildPixi.yml`) will compile the new `src/Gui/FreeWorks/` subdirectory once registered, giving SC2 build coverage for free. | Validation Architecture | Low — they build `src/Gui`; a new registered subdirectory is included. Manual launch on each OS is still required (CI builds, doesn't interactively test nav). |
| A5 | A lint/grep CI check can reliably flag "SolidWorks" in new FreeWorks code while allow-listing the single legitimate reference to the upstream `Gui::SolidWorksNavigationStyle`. | Pitfall 4 / Validation | Low — a path/identifier scope + allow-list pattern is standard. |

## Open Questions

1. **OQ-1 — Activation & coexistence model (Claude's Discretion).**
   - What we know: `FwWorkbench` as a switchable workbench alongside stock FreeCAD is the merge-safe, verified default (rides the workbench plugin path; zero `MainWindow` startup edits). Auto-activate-at-startup and "hide classic chrome" are product choices layered on top.
   - What's unclear: whether the product should boot straight into FreeWorks (hiding FreeCAD chrome) or present FreeWorks as a selectable mode.
   - **Recommendation:** Implement the switchable `FwWorkbench` activation seam unconditionally. Add an *optional* "activate FreeWorks at startup" preference (default OFF for Phase 1, confirm at plan review). This keeps the merge-safe core and makes "boot into FreeWorks" a one-flag product decision, not an architectural fork.

2. **OQ-2 — Phase-1 shell contents (Claude's Discretion).**
   - What we know: ribbon (P2), tree (P3), PropertyManager (P4) don't exist yet. The shell must establish the SW dock *geometry* and reserve mount points (top = ribbon, left = tree+PropertyManager, right = Task Pane).
   - What's unclear: labeled empty placeholder docks vs. temporarily reusing FreeCAD's current tree/combo in the SW dock positions.
   - **Recommendation:** Register **permanent Fw dock names** (`Fw_FeatureManager`, `Fw_PropertyManager`, reserve `Fw_TaskPane`) showing **labeled placeholder widgets** in the correct SW positions, with the **top toolbar area reserved** for the ribbon. Rationale: later phases swap *content* into stable dock names without re-laying-out, and a labeled-placeholder shell makes "coherent SW layout" demonstrable now (SHELL-01) without borrowing FreeCAD chrome that would later have to be unwound. Optionally host the existing `Std_TreeView` content inside the `Fw_FeatureManager` dock temporarily so the shell isn't visually empty — but under the Fw dock name, not by editing the Std dock.

3. **OQ-3 — macOS navigation substitute (Claude's Discretion; ROADMAP SC3).**
   - What we know: `SolidWorksNavigationStyle` uses bare MMB / Ctrl+MMB with **no built-in middle-button emulation** (verified). FreeCAD ships a touch/trackpad `GestureNavigationStyle` and the viewer grabs `Qt::PinchGesture`. macOS lacks a default 3-button mouse.
   - What's unclear: whether SW parity on Mac is best served by (a) emulating MMB via a modifier (e.g. a designated key + left-drag = rotate) layered on the SW style, or (b) defaulting Mac to `GestureNavigationStyle` (two-finger pan, pinch-zoom) which feels native but diverges from SW button semantics.
   - **Recommendation:** Provide **both, with a documented default**: on macOS default FreeWorks to a **modifier-emulated-MMB profile** (preserves SW muscle-memory semantics: emulated-MMB-drag = rotate, Ctrl/Cmd+emulated-MMB = pan, scroll = zoom-to-cursor), and expose `GestureNavigationStyle` as a one-click trackpad alternative. Spike this on a real Mac early in the phase (Pitfall 3). Keep it Gui-only. Confirm the exact emulation modifier at plan review.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Qt 6.8 Widgets/SVG | FreeWorks shell, theme | ✓ (FreeCAD pin) | 6.8.x | — |
| `Gui::SolidWorksNavigationStyle` | NAV-01 | ✓ (in-tree) | FreeCAD `main` | — |
| `Gui::GestureNavigationStyle` + `Qt::PinchGesture` | macOS nav substitute | ✓ (in-tree) | FreeCAD `main` | Modifier-emulated MMB |
| GTest `Gui_tests_run` target | FwWorkbench unit tests | ✓ | bundled | — |
| `FreeCADCmd` / `MainCmd` (console) | Headless `.FCStd` gate (SC4) | ✓ | FreeCAD `main` | — |
| Per-OS CI (`sub_buildWindows/Ubuntu/Pixi`) | SHELL-02 build proof (SC2) | ✓ | repo | — |
| macOS hardware (Apple Silicon + trackpad) | SC3 manual nav verify, OQ-3 spike | ✗ (not confirmed available to dev) | — | CI builds prove *compile*; interactive nav needs a real Mac — flag for human |
| Windows + Linux machines | SC2 manual launch verify | ✗ (not confirmed) | — | CI builds; manual launch flagged for human |
| `ASSET_PROVENANCE.md` + provenance CI guard | SC4 | ✗ (does not exist yet) | — | Create in this phase (greenfield) |
| `// SW-FORK HOOK` markers + sync script | SC5 | ✗ (none exist yet) | — | Create in this phase (greenfield) |

**Missing dependencies with no fallback:**
- Interactive cross-platform launch/nav verification (Win/macOS/Linux) requires real machines — CI proves build, not feel. **Flag for human** (SC2, SC3).

**Missing dependencies with fallback:**
- Provenance file, CI guard, hook markers, sync script — all greenfield, created within this phase (they ARE deliverables, not blockers).
- macOS MMB substitute — `GestureNavigationStyle`/pinch + modifier-emulation cover it.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Google Test (C++ `Gui_tests_run`) + Python `unittest` (FreeCAD `-t`) + CI build matrix |
| Config file | `tests/src/Gui/CMakeLists.txt` (exists); CI in `.github/workflows/CI_primary.yml` + `sub_build*.yml` |
| Quick run command | `ctest -R Gui --output-on-failure` (after `cmake --build . --target tests`) |
| Full suite command | `ctest --output-on-failure` |

### Success Criterion → Verification Map
| SC | Behavior | Verification Type | Command / Gate | Exists? |
|----|----------|-------------------|----------------|---------|
| SC1 (SHELL-01) | "FreeWorks" workbench activates and mounts a coherent dock shell with **zero** `MainWindow.cpp` body edits beyond marked `// SW-FORK HOOK` | GTest + grep gate | `ctest -R Gui` (FwWorkbench registers + `setupDockWindows` returns Fw docks); CI grep asserts no unmarked `MainWindow.cpp` diff | ❌ Wave 0 (new test) |
| SC2 (SHELL-02) | Builds + runs on Win/macOS/Linux | CI build matrix + manual launch | `sub_buildWindows.yml` / `sub_buildUbuntu.yml` / `sub_buildPixi.yml` green; manual launch on 3 OSes | ✅ build CI / ❌ manual (human) |
| SC3 (NAV-01) | SW nav is the default; macOS no-MMB/trackpad profile defined | GTest (default pref) + manual nav | GTest asserts `NavigationStyle` default resolves to `Gui::SolidWorksNavigationStyle`; manual rotate/pan/zoom/roll/dolly on 3 OSes incl. Mac trackpad | ❌ Wave 0 + human |
| SC4a | Asset-provenance CI guard rejects any binary image lacking an `ASSET_PROVENANCE.md` entry | CI job | New workflow job: diff added binary images vs provenance file → fail if unlisted | ❌ Wave 0 |
| SC4b | Headless `--console` `.FCStd`-compat gate proves no Gui state leaks to App | CI/script | Save in FreeWorks mode → load via `FreeCADCmd` headless → recompute OK + no Fw/Gui keys; opens in unmodified upstream FreeCAD | ❌ Wave 0 |
| SC5 | Scripted upstream-sync drill against a pinned commit completes in hours; all shared touches greppable via `// SW-FORK HOOK` | Script + grep | `tools/fw-sync-upstream.sh` runs against pinned commit; `grep -rn "SW-FORK HOOK" src/` enumerates every shared touch; timed run | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest -R Gui --output-on-failure` + pre-commit (clang-format, "SolidWorks"-leak grep, provenance guard).
- **Per wave merge:** full `ctest` + per-OS build (CI matrix).
- **Phase gate:** all six SC verifications green; manual tri-platform launch + nav signed off; sync drill timed and documented.

### Wave 0 Gaps
- [ ] `tests/src/Gui/FwWorkbench.cpp` — GTest: FwWorkbench registers, `setupDockWindows()` returns `Fw_*` docks, nav default resolves to `Gui::SolidWorksNavigationStyle` (covers SC1, SC3).
- [ ] `ASSET_PROVENANCE.md` — provenance ledger (covers SC4a).
- [ ] `.github/workflows/` job — asset-provenance guard (SC4a).
- [ ] `.github/workflows/` job + script — headless `.FCStd`-compat gate (SC4b).
- [ ] CI grep job — "SolidWorks" leak check (allow-list the upstream nav-style reference) (Pitfall 4).
- [ ] CI grep job — assert no unmarked edits to `MainWindow.cpp`/shared files outside `// SW-FORK HOOK` (SC1, SC5).
- [ ] `tools/fw-sync-upstream.sh` — scripted upstream-sync drill against a pinned commit (SC5).
- [ ] Manual cross-platform launch + nav checklist (Win/macOS/Linux) — human verification (SC2, SC3).

## Security Domain

> `security_enforcement: true`, ASVS level 1, block-on: high (from `.planning/config.json`). This is a desktop CAD GUI module with no network/auth/data-handling surface in Phase 1; most ASVS categories are N/A. The domain-relevant risks are IP/legal integrity and upstream-security-patch flow.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No auth surface in this module |
| V3 Session Management | no | No sessions |
| V4 Access Control | no | No access-control surface |
| V5 Input Validation | partial | Only the `NavigationStyle` preference value is read; resolved via type-name lookup that already validates against `NavigationStyle::getClassTypeId()` (a bad name yields a safe no-op, verified). No untrusted external input introduced. |
| V6 Cryptography | no | No crypto introduced — never hand-roll any |
| V14 Configuration | yes | New preference default + new CI gates; keep config additive and documented |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Proprietary SW asset / wordmark entering repo or git history | Tampering / Repudiation (IP integrity) | `ASSET_PROVENANCE.md` + CI provenance guard + "SolidWorks"-string CI grep; no binary image without a source entry (SC4a, Pitfall 4/5) |
| Fork drifting off upstream security patches (OCCT/Qt/Python CVEs) | Elevation / DoS via stale deps | Scripted small-frequent upstream-sync drill against a pinned commit keeps security fixes flowing (SC5) |
| GUI state polluting `.FCStd` / breaking headless integrity | Tampering (data integrity) | Headless `--console` `.FCStd`-compat gate; upstream FreeCAD must open FreeWorks-saved files (SC4b) |
| Untrusted nav-style preference value | Tampering | Type-name resolution validates against `NavigationStyle` type hierarchy; invalid → no-op (verified `setNavigationType` 1701-1715) |

## Sources

### Primary (HIGH confidence — direct reads of this checkout)
- `src/Gui/Workbench.h` (89, 117-123) — `activated()`, `setupMenuBar/ToolBars/DockWindows` virtuals — the FwWorkbench seam.
- `src/Gui/Navigation/SolidWorksNavigationStyle.cpp` (37 `TYPESYSTEM_SOURCE`, 45-58 `mouseButtons`, 61+ `processSoEvent`) — NAV-01 already implemented; bare MMB/Ctrl+MMB, no MMB-emulation.
- `src/Gui/Navigation/NavigationStyle.h` (456 `SolidWorksNavigationStyle`, 474 `MayaGestureNavigationStyle`) — style hierarchy.
- `src/Gui/Navigation/GestureNavigationStyle.cpp` (954 `TYPESYSTEM_SOURCE`) + `src/Gui/View3DInventorViewer.cpp` (718 `grabGesture(Qt::PinchGesture)`) — macOS trackpad substitute base.
- `src/Gui/View3DSettings.cpp` (279-288) — `NavigationStyle` preference read, **default `CADNavigationStyle`** → flip to SW.
- `src/Gui/View3DInventor.cpp` (233 `Preferences/View` group; 912-920 nav-style event) — preference path + apply point.
- `src/Gui/View3DInventorViewer.cpp` (880 `new CADNavigationStyle()`; 1696-1720 `setNavigationType` resolves by type name) — nav default + safe resolution.
- `tests/src/Gui/CMakeLists.txt` — `Gui_tests_run` GTest target links `FreeCADGui` (FwWorkbench unit-test home).
- `src/Main/MainCmd.cpp` (exists) — headless entry for SC4b.
- `.github/workflows/` listing — `CI_primary.yml`, `sub_buildWindows/Ubuntu/Pixi.yml`, `sub_lint.yml` (SC2 + gate hosts).
- Repo scans — **no** `ASSET_PROVENANCE.md`, **no** `// SW-FORK HOOK` markers, **no** MMB-emulation code exist yet (greenfield scaffolding confirmed).
- `.planning/config.json` — `nyquist_validation: true`, `security_enforcement: true` (ASVS 1).

### Secondary (project research — HIGH within project context)
- `.planning/research/ARCHITECTURE.md` — additive module + Workbench activation backbone, hook-point table, build order (apply `Fw` over `Sw`).
- `.planning/research/STACK.md` — existing-facility verdict, native-Qt path, no-new-deps.
- `.planning/research/PITFALLS.md` — merge-hell, App/Gui boundary, cross-platform nav, asset-cloning, "looks-done-but-isn't" checklist.
- `.planning/research/SOLIDWORKS-UI.md` — verified SW nav defaults (rotate=MMB, pan=Ctrl+MMB, zoom-to-cursor, roll=Alt+MMB, dolly=Shift+MMB, MMB-click-entity rotate-about) for NAV-01.
- `.planning/codebase/TESTING.md`, `CONVENTIONS.md` — GTest/ctest patterns, naming/style/SPDX rules.
- `.planning/ROADMAP.md` §Phase 1, `.planning/REQUIREMENTS.md` (SHELL-01/02, NAV-01), `.planning/phases/01-.../01-CONTEXT.md` (D-01/02/03).

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — every facility verified by direct read; no new deps.
- Architecture / seams: HIGH — `FwWorkbench` activation, dock registration, and nav-default mechanism all verified against actual signatures and the preference-read path.
- NAV-01 default mechanism: HIGH — preference path, key, current default, and type-name resolution all read directly.
- macOS substitute: MEDIUM — `GestureNavigationStyle`/pinch base verified, but exact SW-parity mapping needs a Mac spike (A3, OQ-3).
- Non-functional gates (provenance/headless/sync): HIGH on mechanism, MEDIUM on effort — greenfield, but built on existing CI/test infra.
- Pitfalls: HIGH — grounded in the codebase maps and verified absence of MMB-emulation/provenance scaffolding.

**Research date:** 2026-06-06
**Valid until:** ~2026-07-06 for FreeCAD-internal seams (stable but `main` is a moving target — re-verify the `NavigationStyle` default fallback and `Workbench.h` signatures at plan time if the tracked upstream commit advances).

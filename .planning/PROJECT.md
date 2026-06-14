# FreeCAD: SolidWorks-Style UI

## What This Is

A fork of FreeCAD that replaces the entire GUI with a SolidWorks-faithful interface — the CommandManager ribbon, the FeatureManager design tree, the sliding PropertyManager, and SolidWorks-style mouse/selection/navigation behavior. The goal is full *workflow parity*: not just a reskin, but an interface that looks, navigates, and behaves like SolidWorks, built on FreeCAD's parametric modeling engine underneath.

It is aimed at **SolidWorks users** so they can move to FreeCAD with effectively zero relearning of muscle memory.

## Core Value

A SolidWorks user can open it and be immediately productive — it *looks* like SolidWorks, *navigates* like SolidWorks, and *behaves* like SolidWorks — with no FreeCAD tutorial required. If everything else is cut, this experience is the thing that must hold.

## Requirements

### Validated

<!-- Inherited capabilities of the FreeCAD codebase this fork builds on. Confirmed by the codebase map (.planning/codebase/). -->

- ✓ Parametric modeling engine — App-layer Document Object Model, property-driven objects, dependency-ordered recompute — existing
- ✓ Part / PartDesign / Sketcher modeling workflow (sketch → feature) — existing
- ✓ Qt 6 GUI framework with the Workbench plugin system, CommandManager, Task panels, tree view, and property editor — existing
- ✓ Coin3D 3D viewport with navigation, selection, and ViewProvider visualization bridge — existing
- ✓ FCStd document save/restore and transaction-based undo/redo — existing
- ✓ Cross-platform build for Windows, macOS, and Linux (CMake + Pixi/Conda, version 1.2.0-dev) — existing

<!-- Fork-delivered SolidWorks-parity capabilities, validated by phase verification + UAT. -->

- ✓ SolidWorks-mode shell + SolidWorks **mouse navigation** default (FwWorkbench, tri-OS) — Phase 1 (SHELL-01/02, NAV-01)
- ✓ SolidWorks-style **CommandManager ribbon** — tabbed Features/Sketch/Evaluate, large labeled buttons, flyout split-buttons, context-driven tab switching, replaces FreeCAD menus+toolbars in FreeWorks mode — Phase 2 (RIBBON-01, RIBBON-02; UAT 5/5, security 13/13)

### Active

<!-- The SolidWorks-parity goals. Hypotheses until shipped and validated against real SW users. -->

- [ ] SolidWorks-style **FeatureManager design tree** — ordered feature history with a rollback bar
- [ ] SolidWorks-style **PropertyManager** — left-side panel that slides in with contextual options when running a command or editing a feature
- [ ] SolidWorks **mouse navigation** (rotate / pan / zoom), **selection model**, selection filters, and context toolbars
- [ ] SolidWorks **sketch → feature modeling flow** parity (plane/face pick → sketch → extrude/revolve/etc.)
- [ ] SolidWorks **visual theme** — recreated look-alike icons, color scheme, fonts, and layout
- [ ] A SolidWorks user is **productive without a tutorial** across Windows, macOS, and Linux

### Out of Scope

<!-- Explicit boundaries with reasoning, to prevent re-adding. -->

- Copying SolidWorks **proprietary assets verbatim** (icons, theme files, branding) — legal risk; parity is achieved with recreated *look-alike* assets, which is fine. Layout and behavior mimicry is not a legal issue.
- **Standalone front-end / headless backend** architecture — rejected in favor of forking the GUI directly.
- **Addon / theme-pack only** approach — insufficient for full behavioral parity; a fork of the C++ Gui is required.
- **SolidWorks file format (.sldprt/.sldasm) round-trip** — out of scope for UI parity (potential future milestone).

## Context

- **Codebase:** This is a brownfield fork of FreeCAD. Full architecture, stack, structure, conventions, integrations, testing, and concerns maps live in `.planning/codebase/`.
- **Fork baseline:** Current FreeCAD `main` (version `1.2.0-dev`), this working checkout (commit `768e237091`).
- **Architecture to work within:** Strict App/Gui separation. The App layer (Document Object Model, parametric features, recompute) stays intact — this project lives almost entirely in the **Gui layer** (`src/Gui/*` and the `Gui/` subdirectories of `src/Mod/*`). Key abstractions to extend/replace: `Gui::MainWindow`, `Gui::Workbench` (menu/toolbar/dock setup), `Gui::CommandManager`, the Tree view, the Property editor / Task panels, and the Coin3D `View3DInventor` navigation/selection.
- **Why the Gui-layer focus matters:** SolidWorks parity is a *presentation and interaction* problem, not a geometry problem. OCCT (geometry kernel) and the App DOM already provide the modeling power; the work is re-presenting it the SolidWorks way.
- **Stack:** C++20, Qt 6.8 (PySide6/Shiboken bindings), Coin3D, OpenCASCADE 7.8, built via CMake/Pixi.

## Constraints

- **Tech stack**: C++20 / Qt 6.8 / Coin3D / OCCT 7.8 — all UI work must respect FreeCAD's App/Gui separation and the Workbench plugin pattern. Don't bypass Property change notifications or modify the document during recompute (see codebase ARCHITECTURE.md anti-patterns).
- **Fork baseline**: Tracks FreeCAD `main` (`1.2.0-dev`) — a moving target. The fork must be structured to absorb upstream changes without constant merge pain (favor additive/overriding Gui components over deep edits to shared code where feasible).
- **Platforms**: Must build and run on Windows, macOS, and Linux. Qt/Coin3D give cross-platform rendering, but SolidWorks-specific mouse/keyboard conventions must be reproduced consistently across all three.
- **Legal**: No verbatim SolidWorks proprietary assets. Recreated look-alike icons/themes only.
- **Single-threaded GUI**: Qt event loop is single-threaded; long operations must stay off the GUI thread (existing FreeCAD task-panel/worker pattern).

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Fork FreeCAD's C++ Gui rather than build an addon/theme pack | Full SolidWorks *behavioral* parity (ribbon, PropertyManager, navigation, selection) can't be achieved by stylesheets/config alone | — Pending |
| Target full workflow parity ("everything"), staged via roadmap | The end goal is a complete SW experience; incremental phases make it shippable | — Pending |
| Aim at SolidWorks migrants as the audience | Defines success as "no relearning" — drives every UX choice toward SW conventions | — Pending |
| Fork from current `main` (1.2.0-dev) | Newest internals; accept upstream-tracking cost over starting from an older stable base | — Pending |
| Recreate look-alike assets instead of copying SW assets | Achieve visual parity while avoiding proprietary-asset legal risk | — Pending |
| Support Windows + macOS + Linux | Reach SW users (mostly Windows) without dropping FreeCAD's cross-platform reach | — Pending |
| Build the ribbon natively on Qt (QTabWidget + QToolBar) rather than vendoring SARibbon | Spike confirmed native reaches the SolidWorks-parity bar (D-03); zero new deps, best upstream-merge story | ✓ Phase 2 — native committed, SARibbon shelved |
| Drive the ribbon from FreeCAD's existing command registry via a curated declarative map + auto-derive fallback | No duplicated command backend; merge-safe data file; never-empty ribbon on uncurated workbenches | ✓ Phase 2 — RIBBON-01 shipped |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-14 after Phase 2 (CommandManager Ribbon)*

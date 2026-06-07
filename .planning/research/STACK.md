# Stack Research

**Domain:** SolidWorks-style UI (ribbon + FeatureManager tree + PropertyManager) inside FreeCAD's Qt 6 / C++20 Gui layer
**Researched:** 2026-06-06
**Confidence:** HIGH (grounded in the actual FreeCAD `main` checkout at commit `768e237091`; third-party versions verified against upstream repos/package indexes June 2026)

---

## TL;DR — The Prescriptive Verdict

**Build the SolidWorks UI almost entirely with facilities that already exist in FreeCAD's Gui layer. Add only ONE third-party C++ library, and only if a native-Qt ribbon proves insufficient.**

| Concern | Recommendation | Already in FreeCAD? |
|---------|----------------|---------------------|
| Ribbon (CommandManager) | **Build natively** on `ToolBarManager` + `ToolBarAreaWidget` + `QTabWidget`/`QToolBar`. Fall back to **SARibbon (MIT)** only if native parity stalls. | Toolbar infra: YES. Ribbon shell: NO (must add). |
| Docking / panel layout | **Use FreeCAD's existing `QMainWindow` + `DockWindowManager` + `OverlayManager`.** Do NOT add Qt-Advanced-Docking-System. | YES — including slide-in overlay panels. |
| FeatureManager tree | **Extend `Gui::TreeWidget`** (`QTreeWidget` + custom delegate). Add rollback bar as a custom widget. | Tree: YES. Rollback bar: NO (must add). |
| PropertyManager (left slide-in) | **Reuse `Gui::Control` / `TaskView` task-panel system, relocated/restyled via `OverlayManager`.** | YES — this is the closest 1:1 SW analog. |
| Theming | **FreeCAD QSS + `StyleParameters` parametric engine + `PreferencePackManager`.** Ship a "SolidWorks" preference pack. | YES (must author the pack). |
| Icons | **SVG via `BitmapFactory` + Qt resource (`.qrc`) + `QtSvg`.** Recreate look-alike SVGs. | Pipeline: YES. Assets: NO (must author). |
| Mouse navigation / selection | **Extend the existing `Gui::SolidWorksNavigationStyle`** (it already ships) + `NavigationStyle` selection hooks. | YES — a SW nav style already exists. |

The single most important finding: **FreeCAD already ships `src/Gui/Navigation/SolidWorksNavigationStyle.cpp`, a full overlay/slide-in panel system (`OverlayManager`/`OverlayWidgets`), a parametric QSS theming engine (`StyleParameters`), a clean `WorkbenchManipulator` extension point, and a modern custom toolbar-area widget (2024).** This project is far more "assemble + restyle existing FreeCAD Gui machinery" than "import new UI libraries."

---

## Recommended Stack

### Core Technologies (all already present in FreeCAD — extend, don't replace)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Qt Widgets | 6.8.x (LTS, FreeCAD's pin) | All chrome: ribbon, tree, panels, docks | FreeCAD's entire Gui is QtWidgets; staying native = zero new deps, full cross-platform, upstream-merge-friendly |
| Qt SVG (`QtSvg`) | 6.8.x | Scalable look-alike icon rendering | FreeCAD already links it; `BitmapFactory::pixmapFromSvg()` exists; SVG is mandatory for HiDPI/4K parity with SolidWorks |
| `Gui::ToolBarManager` / `ToolBarItem` / `ToolBarAreaWidget` | FreeCAD `main` (`ToolBarAreaWidget` added 2024) | Foundation for the ribbon's command groups | Already converts `CommandManager` commands → `QToolBar` widgets and supports **custom toolbar areas** (incl. menu-bar-corner areas) — the hook a ribbon needs |
| `Gui::DockWindowManager` + `QMainWindow` docking | FreeCAD `main` | Panel registration, persistence, layout | Native `QMainWindow` docking with `GroupedDragging`; FreeCAD wraps it with registration + state save/restore |
| `Gui::OverlayManager` / `OverlayWidgets` (`OverlayTabWidget`) | FreeCAD `main` | **Slide-in / auto-hide side panels** — the PropertyManager behavior | This is the existing FreeCAD analog of SW's sliding PropertyManager; reuse instead of inventing |
| `Gui::Control` + `Gui::TaskView::TaskView` / `TaskDialog` | FreeCAD `main` | Contextual command UI (the PropertyManager *content*) | SW's PropertyManager == "command-scoped option panel"; FreeCAD's task-panel system is exactly that, already wired to commands/transactions |
| `Gui::TreeWidget` (`QTreeWidget` + `TreeWidgetItemDelegate`) | FreeCAD `main` | FeatureManager design tree | Already a document-mirroring, selection-observing, drag/drop-capable feature tree; needs SW visual + rollback bar, not a rewrite |
| `Gui::NavigationStyle` + `SolidWorksNavigationStyle` | FreeCAD `main` | Mouse rotate/pan/zoom + selection model | **A SolidWorks nav style already ships**; extend it for full middle-mouse-rotate / selection-filter / context-toolbar parity |
| `Gui::WorkbenchManipulator` | FreeCAD `main` (added 2023) | Inject/override menus, toolbars, dock windows | Lets the fork **add the ribbon and reshape every workbench's UI additively**, minimizing edits to shared code (key for upstream tracking) |
| StyleParameters engine + `FreeCAD.qss` + `PreferencePackManager` | FreeCAD `main` | Theming (colors, metrics, QSS) | Parametric QSS (variables, arithmetic, tuples) + shippable "preference packs" = a clean way to deliver a "SolidWorks" theme |

### Supporting Libraries (candidate additions — add at most one)

| Library | Version | License | Purpose | When to Use |
|---------|---------|---------|---------|-------------|
| **SARibbon** | v2.8.0 (May 2026) | **MIT** | Drop-in Microsoft/SolidWorks-style ribbon (tabs, panels, ribbon buttons, gallery) | ONLY if a native `QToolBar`/`QTabWidget` ribbon cannot reach SW visual parity (collapse-to-tab, contextual tabs, application button) within budget. MIT + Qt 6.8 LTS + cross-platform make it the safest external option. |
| `QtSvg` (already linked) | 6.8.x | LGPLv3/commercial (Qt) | Icon rendering | Always (already a dependency) |

> **Do not add a docking library.** See "What NOT to Use."

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Qt Designer (`.ui` files) | Lay out static task/property panels | FreeCAD already uses `.ui` + AUTOUIC; keep this for PropertyManager panel content |
| `lupdate`/Crowdin | i18n of new strings | All new SW-labelled commands must use `QT_TR_NOOP`/`tr()` (FreeCAD convention) |
| `.qrc` resource compiler (`rcc`) | Bundle recreated SVG icons | Mirror `src/Gui/Icons/` resource pattern; one `.qrc` per workbench |
| Inkscape / SVG editor | Author look-alike icons | Recreate, never copy SW assets (legal constraint); keep a documented source set |
| clang-format / clang-tidy | Match FreeCAD code style | Configs already in repo; required for upstream-mergeable diffs |

---

## Installation / Wiring

No new package-manager entries are required for the recommended (native) path — everything is already in `pixi.toml` / the FreeCAD build.

**If (and only if) SARibbon is adopted**, vendor it as a CMake subproject rather than a system dependency, to keep cross-platform builds reproducible under Pixi:

```bash
# Vendored, not via pixi — keeps Win/macOS/Linux builds identical
git submodule add https://github.com/czyt1988/SARibbon.git src/3rdParty/SARibbon
# In src/3rdParty/CMakeLists.txt: add_subdirectory(SARibbon)
# SARibbon supports Qt 6.8 LTS and CMake out of the box.
```

> Rationale for vendoring: FreeCAD already vendors UI-adjacent 3rd-party code under `src/3rdParty/` (e.g., `FastSignals`, `zipios++`). A submodule avoids adding a Conda-Forge dependency that may lag Qt 6.8 on one platform.

---

## How the pieces map to SolidWorks concepts

```
SolidWorks concept        →  FreeCAD facility (existing)            →  Work to add
─────────────────────────────────────────────────────────────────────────────────
CommandManager (ribbon)   →  ToolBarManager + ToolBarAreaWidget    →  Ribbon shell widget
                             + WorkbenchManipulator                    (tabs/panels), per-WB tabs
FeatureManager tree       →  Gui::TreeWidget (+delegate)           →  SW styling + rollback bar
PropertyManager (slide)   →  Gui::Control/TaskView + OverlayManager →  Reposition left, SW styling
Mouse nav + selection     →  SolidWorksNavigationStyle (ships!)    →  Selection filters,
                             + NavigationStyle hooks                   context toolbars
Theme (color/icons/font)  →  StyleParameters + QSS + PreferencePack →  "SolidWorks" pack + SVGs
Layout persistence        →  DockWindowManager + OverlayManager save→  (mostly free)
```

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Native ribbon on `ToolBarAreaWidget` + `QTabWidget` | **SARibbon (MIT, v2.8.0)** | If native build can't match SW ribbon polish (contextual tabs, collapsible panels, gallery, app button) within schedule. MIT + Qt 6.8 LTS verified — the only external ribbon worth considering. |
| Native ribbon | **QtitanRibbon (commercial)** | Only if a paid, vendor-supported ribbon with Qt 6.10 binaries is mandated. **Rejected by default**: commercial license is incompatible with FreeCAD's LGPL distribution model and the "open fork" intent. |
| FreeCAD `OverlayManager` for slide-in panels | **Qt-Advanced-Docking-System (ADS) 4.4.x (LGPL-2.1)** | If you needed Visual-Studio-class tab-merge/auto-hide that FreeCAD's overlay system genuinely can't do. **Not the case here** — FreeCAD's overlay + dock system already covers SW's layout. Adding ADS would duplicate/conflict with `DockWindowManager`. |
| Extend `Gui::TreeWidget` (`QTreeWidget`) | Rewrite tree on `QTreeView` + custom `QAbstractItemModel` | Only if you need >100k-node virtualized performance. SW feature trees are small; the existing item-based tree is fine and already document-synced. |
| Extend `SolidWorksNavigationStyle` | Write a fresh `NavigationStyle` subclass | If SW's exact gesture set diverges so far it's cleaner to start from `UserNavigationStyle`. Unlikely — start from the shipped SW style. |
| `StyleParameters` + QSS preference pack | Hard-coded `setStyleSheet()` calls | Never for shipping; only for throwaway prototyping. The parametric pack is themeable, user-switchable, and merge-friendly. |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **Qt-Advanced-Docking-System (ADS)** | Duplicates FreeCAD's `DockWindowManager` + `OverlayManager`; two docking systems in one `QMainWindow` causes layout-state, focus, and persistence conflicts; adds an LGPL dependency for capability FreeCAD already has | FreeCAD's native dock + overlay system |
| **QtitanRibbon / any commercial ribbon** | License incompatible with an LGPL open fork; per-seat cost; closed source can't be patched for FreeCAD-specific integration | Native ribbon, or MIT SARibbon |
| **QxRibbon** | Upstream repo is explicitly **UNMAINTAINED** (folded into qtcanpool) | SARibbon (actively maintained, MIT) |
| **Qt Quick / QML for the chrome** | FreeCAD's Gui is QtWidgets end-to-end; QML would need a parallel integration layer, breaks `WorkbenchManipulator`/`Action`/`Command` wiring, and complicates Coin3D embedding | QtWidgets (native) |
| **Qt5 / Qt5-compat APIs** | FreeCAD pins Qt 6.8 LTS and Qt5 support is deprecated (per codebase STACK.md, ~2026-08) | Qt 6.8 widgets only |
| **Hard-coded `setStyleSheet()` per widget** | Unmaintainable, not user-switchable, fights the QSS cascade and `StyleParameters` | `StyleParameters` + preference pack |
| **Copying SolidWorks icon/theme assets** | Legal risk (PROJECT.md constraint) | Recreate look-alike SVGs from scratch |
| **A new `QAbstractItemModel` tree rewrite** | Throws away FreeCAD's document-sync, selection-observer, and drag/drop logic in `TreeWidget`; large surface for upstream-merge conflicts | Extend `Gui::TreeWidget` |

---

## Stack Patterns by Variant

**If native ribbon reaches parity (preferred path):**
- Build a `RibbonBar` `QWidget` (a `QTabWidget` whose pages host `ToolBarAreaWidget`s / `QToolBar` command groups), inserted via `WorkbenchManipulator::modifyToolBars` / a `MainWindow` menu-area widget.
- Because `ToolBarManager` already turns `CommandManager` commands into `QToolBar`s, the ribbon is mostly *re-layout*, not new command plumbing.
- Zero new dependencies; cleanest upstream tracking.

**If native ribbon stalls on SW polish:**
- Adopt **SARibbon** (MIT, vendored submodule), feed it the same `Gui::Action`/`Command` objects.
- Accept one vendored dependency; keep it isolated under `src/3rdParty/` so it doesn't leak into App layer.

**If a workbench needs SW "contextual tabs" (tabs that appear during an operation):**
- Drive ribbon tab visibility from `Gui::Control` active-`TaskDialog` state (same signal that shows the PropertyManager).

**For the PropertyManager:**
- Content = `TaskView`/`TaskDialog` panels (reuse existing per-feature task panels).
- Container/behavior = `OverlayManager` left-side auto-hide/slide, restyled via QSS. Do not build a bespoke animation system first; try the overlay slide.

---

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| Qt 6.8.x (FreeCAD pin) | SARibbon v2.8.0 | SARibbon explicitly lists **Qt 6.8 LTS** support; CMake build; MIT |
| Qt 6.8.x | Qt-Advanced-Docking-System 4.4.x | Compatible, but **rejected** for architectural reasons above |
| C++20 (FreeCAD required) | SARibbon | SARibbon builds under modern C++; no conflict |
| `OverlayManager` | `DockWindowManager` | Co-designed in FreeCAD; safe together. A *third* docking system (ADS) is the conflict risk |
| `StyleParameters` QSS | Qt 6.8 QSS | FreeCAD extends standard QSS with a variable/expression preprocessor; emits standard QSS to Qt |
| Cross-platform (Win/macOS/Linux) | All recommended (native) components | Qt + Coin3D give parity; **the SW-specific mouse/keyboard conventions must be reproduced identically per-OS in `SolidWorksNavigationStyle`** (macOS lacks a 3-button mouse by default — plan trackpad/modifier fallbacks) |

### Cross-platform call-outs (per `<downstream_consumer>`)

- **Native ribbon / QtWidgets:** identical on all three OSes (Qt-handled). No risk.
- **SARibbon (if used):** upstream notes Windows UI is "deeply optimized," macOS/Linux "supported but not deeply optimized." Budget QA time for macOS/Linux ribbon polish if adopted. (LOW-risk but real.)
- **SVG icons via `BitmapFactory`:** uniform across OSes; HiDPI handled by Qt. No risk.
- **QSS theming:** native-control rendering differs subtly per OS; FreeCAD already overrides with `FreeCADStyle` (a `QProxyStyle`/`QStyle`) — extend it rather than relying on raw platform styles.
- **Mouse navigation:** highest cross-platform risk. macOS middle-mouse / Ctrl+MMB conventions differ; the shipped `SolidWorksNavigationStyle` uses MMB for drag and Ctrl+MMB for pan — verify trackpad/modifier behavior on macOS and Linux explicitly.

---

## Confidence Assessment

| Claim | Confidence | Basis |
|-------|-----------|-------|
| FreeCAD ships `SolidWorksNavigationStyle` | HIGH | File read: `src/Gui/Navigation/SolidWorksNavigationStyle.cpp` (333 lines) |
| FreeCAD has overlay/slide-in panel system | HIGH | Files: `OverlayManager.{h,cpp}`, `OverlayWidgets.{h,cpp}` |
| FreeCAD has parametric QSS theming + preference packs | HIGH | `StyleParameters/`, `Stylesheets/FreeCAD.qss`, `PreferencePackManager`, recent commits (Jun 2026) |
| `WorkbenchManipulator` is the additive extension hook | HIGH | Header read: modify menus/toolbars/dock windows per workbench |
| Tree is `QTreeWidget`-based, extendable | HIGH | `src/Gui/Tree.h`: `class TreeWidget: public QTreeWidget` |
| `ToolBarAreaWidget` supports custom areas (ribbon foundation) | HIGH | `src/Gui/ToolBarAreaWidget.h` (2024) — custom `ToolBarArea` enum incl. menu-corner areas |
| SARibbon: MIT, Qt 6.8 LTS, v2.8.0, cross-platform, maintained | HIGH | Upstream repo (czyt1988/SARibbon), verified June 2026 |
| ADS: 4.4.x, LGPL-2.1, Qt6 | HIGH | GitHub releases + distro packages, verified June 2026 |
| No ribbon library currently in FreeCAD | HIGH | `grep -ril ribbon src/` → no matches; `src/3rdParty/` listing |
| Native ribbon is achievable without a library | MEDIUM | Strong infra exists (`ToolBarManager`, areas); SW polish (contextual tabs, gallery) is the open risk — hence SARibbon fallback |

---

## Sources

- FreeCAD `main` checkout @ `768e237091` — read: `src/Gui/Navigation/SolidWorksNavigationStyle.cpp`, `OverlayManager.h`, `OverlayWidgets`, `WorkbenchManipulator.h`, `ToolBarAreaWidget.h`, `ToolBarManager.h`, `Tree.h`, `Control.h`, `ComboView.h`, `MainWindow.cpp`, `BitmapFactory.h`, `StyleParameters/`, `Stylesheets/`, `cMake/FreeCAD_Helpers/SetupQt.cmake` — HIGH
- https://github.com/czyt1988/SARibbon — license (MIT), Qt 6.8 LTS support, v2.8.0 (May 2026), cross-platform — HIGH
- https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System — v4.4.x, LGPL-2.1, Qt5/Qt6 — HIGH (verified, then rejected on architecture grounds)
- https://github.com/canpool/QxRibbon — confirmed UNMAINTAINED — HIGH
- https://www.devmachines.com/qtitanribbon-overview.html — commercial, Qt 6.x — HIGH (rejected on license grounds)
- `.planning/PROJECT.md`, `.planning/codebase/{STACK,ARCHITECTURE,INTEGRATIONS}.md` — project constraints & existing stack — HIGH

---
*Stack research for: SolidWorks-style UI on FreeCAD Qt 6 Gui layer*
*Researched: 2026-06-06*

# Phase 2: CommandManager Ribbon — Research

**Researched:** 2026-06-07
**Domain:** SolidWorks-style tabbed CommandManager ribbon built natively on Qt 6.8 widgets inside FreeCAD's `src/Gui/FreeWorks/` additive module, driven by the existing `CommandManager`/`ToolBarItem` registry, with context-driven tab switching wired to FreeCAD's edit-mode signals.
**Confidence:** HIGH

## Summary

Phase 2 is far more an *assembly + re-layout* job than a new-subsystem job. Every primitive the ribbon needs already ships in FreeCAD's Gui layer and was confirmed by reading the actual headers in this checkout:

- **Mount point:** `Gui::ToolBarAreaWidget` with the `ToolBarArea::TopToolBarArea` enum value exists; Phase 1 deliberately left the top toolbar area free.
- **Command source:** `Gui::CommandManager` (`getCommandByName`, `runCommandByName`, `getGroupCommands`, `getModuleCommands`) plus `ToolBarItem`/`MenuItem` trees. Buttons are thin triggers — no backend duplication.
- **Flyout split-buttons come for free:** FreeCAD's `Gui::GroupCommand` (the `*_Comp*` commands such as `PartDesign_CompPrimitiveAdditive`, `Sketcher_CompLine`) produces a `Gui::ActionGroup` whose `addTo(QToolBar)` already calls `QToolButton::setPopupMode(MenuButtonPopup)` + `setMenu(...)` — i.e. a native split-button (verified in `src/Gui/Action.cpp:454-488`). The ribbon does not invent flyouts; it reuses this.
- **Contextual switching (RIBBON-02) has an exact, signal-based hook:** `Gui::Application::Instance->signalInEdit` / `signalResetEdit` (`fastsignals::signal<void(const Gui::ViewProviderDocumentObject&)>`, `src/Gui/Application.h:154-156`). The OverlayManager already consumes exactly these. Identify "is a sketch" by `vp.getTypeId().getName() == "SketcherGui::ViewProviderSketch"` — a string comparison that needs **no** compile-time dependency on the Sketcher module (preserves additive/merge discipline). `Gui::Control::activeDialog()` is a getter only (no signal) — do **not** poll it; use the edit signals.
- **Curated map raw material already exists as data:** PartDesign's `setupToolBars()` and its `TaskWatcherCommands` groups (`src/Mod/PartDesign/Gui/Workbench.cpp`) enumerate the exact command IDs and groupings the curated map should reproduce.

**Primary recommendation:** Build `FwRibbon` natively (D-01) as a `QTabWidget` whose pages host `QToolBar`s populated by feeding registry `Command`/`ActionGroup` objects through their existing `addTo()` path, set to `Qt::ToolButtonTextUnderIcon` with a 32px icon size. Author the curated tab/command map as a **declarative C++ table** (compiled `constexpr`/`std::array` of `{tab, panel, commandId}` rows) under `src/Gui/FreeWorks/`. Wire context switching to `Application::signalInEdit/signalResetEdit`. Mount via `FwLayout`/`FwWorkbench` (not `WorkbenchManipulator`). Persist via `QMainWindow::saveState()`/`restoreState()` (mostly free). Run a **2-day time-boxed spike** against the D-03 parity bar before the full build; SARibbon fallback is pre-blessed and needs no second decision round.

## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Native-first. Build the ribbon shell natively on Qt (`QTabWidget` + `QToolBar` on `Gui::ToolBarAreaWidget` / `TopToolBarArea`). Zero new deps.
- **D-02:** Validate with a time-boxed spike before committing the full build.
- **D-03:** Spike pass/fail bar = tabs + panel groups + large labeled icons + working flyout split-buttons + context tab-switch, looking convincingly SW-like. Galleries, application button, collapse-to-tab/minimize are explicitly NOT in the bar.
- **D-04:** SARibbon (MIT, v2.8.0, Qt 6.8) is the pre-blessed fallback, vendored as a git submodule under `src/3rdParty/SARibbon` (CMake `add_subdirectory`) — only if the native spike misses the D-03 bar. Vendor it, not a Pixi/Conda dep.
- **D-05:** Curated SW-faithful declarative map (tab → panels → FreeCAD command IDs), a self-contained data file under `src/Gui/FreeWorks/`. No command backend duplicated.
- **D-06:** Coverage = the core modeling loop — PartDesign + Sketcher tabs (Features, Sketch, Evaluate, optionally Surfaces).
- **D-07:** Uncurated workbenches auto-derive their tabs from that workbench's existing `ToolBarItem` groups (ribbon never empty).
- **D-08:** Gap policy = omit-missing, no-extras. SW command with no FreeCAD equivalent is silently skipped; FreeCAD-only commands NOT auto-appended to curated tabs. Track notable gaps as notes.
- **D-09:** Sketch-edit drives v1 switching. Entering sketch-edit auto-activates the Sketch tab; leaving restores the previously active tab. Wired to `Gui::Control` active-dialog / edit state.
- **D-10:** Context wins, then restore. Entering a sketch always jumps to the Sketch tab even over a manual selection; on exit, restore whatever tab was active before.
- **D-11:** Full ribbon-only chrome — hide FreeCAD's menu bar AND redundant toolbars in FreeWorks mode. Must be reversible / scoped to FreeWorks mode.
- **D-12:** Discoverability escape hatch required — at minimum keep FreeCAD keyboard shortcuts working; ideally a command-search or "more commands" affordance. Do not strand commands.
- **D-13:** Button fidelity = structure + behavior, not final pixels. Large icon-over-label primary buttons + working flyout split-buttons driven by the registry's command groups, using existing icons. Exact SW theming is Phase 7.
- **D-14:** Layout persists across restarts — tab state and panel positions restore. Reuse `DockWindowManager` / toolbar-area state save-restore. v1 layout is fixed/curated (not user-customizable).

### Claude's Discretion (research/planning resolves; confirm at plan review)

- Exact spike time-box duration + concrete D-03 parity checklist.
- Curated-map file format + exact tab/panel/command-ID contents (verify against version-pinned SOLIDWORKS Help).
- Contextual switching: signal/observer vs polling; the precise FreeCAD "entered/left sketch edit" hook.
- Exact D-12 discoverability escape-hatch mechanism.
- Mount seam: `WorkbenchManipulator` vs direct `FwWorkbench`/`FwLayout`.

> Research recommendations for all five discretion items are in their respective sections below. They remain **proposals** until confirmed at plan review.

### Deferred Ideas (OUT OF SCOPE)

- Broader SW contextual tabs (auto-switch/appear by selection type beyond sketch-edit).
- Ribbon galleries, application button, collapse-to-tab/minimize.
- User customization of the ribbon (add/remove/reorder commands).
- Full pixel-level SW theming of ribbon buttons (Phase 7).
- Curated tab rosters for non-core workbenches (Sheet Metal, Weldments, assembly-equivalents) — auto-derive (D-07) covers them functionally.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RIBBON-01 | Tabbed CommandManager ribbon at top (Features / Sketch / Evaluate / …) with large labeled icons and flyout split-buttons, driven by FreeCAD's command registry | `QTabWidget`+`QToolBar` on `ToolBarAreaWidget::TopToolBarArea`; buttons via `Command::getAction()`/`addTo()`; large icons via `Qt::ToolButtonTextUnderIcon` + 32px `setIconSize`; flyouts via native `GroupCommand`/`ActionGroup` (`Action.cpp:454-488`); command IDs sourced from `CommandManager` + PartDesign/Sketcher `setupToolBars()` |
| RIBBON-02 | Active ribbon tab switches by context (Sketch tab activates on entering a sketch) | `Gui::Application::Instance->signalInEdit`/`signalResetEdit` (`Application.h:154-156`); sketch detection via `vp.getTypeId().getName()`; restore-previous-tab on `signalResetEdit` (D-10) |

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Ribbon shell widget (tabs/panels/buttons) | Gui (FreeWorks) | — | Pure chrome; lives in `src/Gui/FreeWorks/`, never touches App |
| Command firing | Gui (`CommandManager`) | — | Ribbon buttons are thin triggers; `runCommandByName`/existing `QAction` |
| Command metadata (icon, text, group) | Gui (`Command`/`Action`) | — | Read via getters; observe-the-DOM |
| Context tab switch (sketch edit) | Gui (`Application` edit signals) | App (edit state lives on doc objects, read-only) | Signal is Gui-layer relay; ribbon only *reads* it |
| Curated tab/command map | Gui (FreeWorks data file) | — | Static data referencing command-ID strings; no App coupling |
| Layout persistence | Gui (`QMainWindow`/`DockWindowManager` state) | — | Qt-native state save/restore keyed by `objectName` |
| Menu-bar/toolbar hiding | Gui (`MainWindow`/`ToolBarManager`) | — | `getMainWindow()->menuBar()->hide()`, `ToolBarManager::setState(...ForceHidden)`; reversible |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Widgets | 6.8.x (FreeCAD pin) | `QTabWidget`, `QToolBar`, `QToolButton` — the ribbon shell | FreeCAD's entire Gui is QtWidgets; native = zero new deps, best merge story (D-01) [CITED: .planning/research/STACK.md] |
| `Gui::ToolBarAreaWidget` + `ToolBarArea::TopToolBarArea` | FreeCAD `main` | Reserved mount point in the top toolbar area | Verified `src/Gui/ToolBarAreaWidget.h:36-46` — enum value `TopToolBarArea` exists; `addWidget()`/`insertWidget()` public [VERIFIED: codebase grep] |
| `Gui::CommandManager` | FreeCAD `main` | Resolve + fire commands; enumerate module/group commands | Verified `src/Gui/Command.h:980-1022` — `getCommandByName`, `runCommandByName`, `getGroupCommands`, `getModuleCommands`, `getAllCommands` [VERIFIED: codebase grep] |
| `Gui::Command` / `Gui::Action` / `Gui::ActionGroup` | FreeCAD `main` | Per-command `QAction`/button, and flyout split-buttons | `Command::getAction()` (`Command.h:273`); `Action::addTo(QWidget*)`; `ActionGroup` drop-down (`Action.cpp:454-488`) [VERIFIED: codebase grep] |
| `Gui::GroupCommand` | FreeCAD `main` | The native flyout/"Comp" command (`PartDesign_CompPrimitiveAdditive`, `Sketcher_CompLine`, …) | `Command.h:746-784`, `setDropDownMenu(bool)`; produces split-button on a toolbar [VERIFIED: codebase grep] |
| `Gui::ToolBarItem` / `Gui::MenuItem` | FreeCAD `main` | Registry tree consumed as data (curated + auto-derive) | `ToolBarManager.h:49-94`, `MenuManager.h:39-63` — `getItems()`, `hasItems()`, `command()` [VERIFIED: codebase grep] |
| `Gui::Application::Instance` signals | FreeCAD `main` | `signalInEdit`/`signalResetEdit` for RIBBON-02 | `Application.h:154-156` — app-level relay of per-document edit signals; pattern proven in `OverlayManager.cpp:406-409` [VERIFIED: codebase grep] |
| `QMainWindow` state + `Gui::DockWindowManager` | FreeCAD `main` | Layout persistence (SC5/D-14) | `MainWindow` IS-A `QMainWindow` (`MainWindow.h:74`); `DockWindowManager::saveState()/loadState()` (`DockWindowManager.h:113-114`) [VERIFIED: codebase grep] |
| `Gui::ToolBarManager::setState(...ForceHidden)` + `QMainWindow::menuBar()->hide()` | FreeCAD `main` | Hide stock chrome in FreeWorks mode (D-11), reversibly | `ToolBarManager.h:154-174` State enum incl. `ForceHidden`/`RestoreDefault` [VERIFIED: codebase grep] |
| `Gui::BitmapFactory::pixmapFromSvg()` | FreeCAD `main` | Icon rendering (icons come from existing command actions this phase) | Existing pipeline; D-13 reuses registry icons, authors none [CITED: .planning/codebase] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **SARibbon** | v2.8.0 (Qt 5.12–6.8 LTS, MIT, CMake + amalgamated `SARibbon.h/.cpp`) | Drop-in ribbon widget | ONLY if the D-02/D-03 native spike fails. Vendored at `src/3rdParty/SARibbon` as a submodule + one `add_subdirectory` (D-04) [VERIFIED: github.com/czyt1988/SARibbon] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Native `QTabWidget`+`QToolBar` ribbon | SARibbon | Fallback only (D-04). Adds one vendored dep; macOS/Linux UI "supported but not deeply optimized" per upstream — budget cross-platform QA if adopted |
| Native ribbon | QtitanRibbon (commercial) / QxRibbon (unmaintained) | Both rejected at project level — license incompatible / abandoned [CITED: .planning/research/STACK.md] |
| `FwLayout`/`FwWorkbench` direct mount | `WorkbenchManipulator` | See "Mount Seam Decision" — direct mount is more additive for a single fork-owned workbench |
| Declarative C++ table for curated map | JSON file | See "Curated-Map File Format" — C++ table is compile-checked, zero parse/IO, simplest build wiring |

**Installation (native path):** No new package-manager entries. Sources added via `target_sources(FreeCADGui PRIVATE ...)` in `src/Gui/FreeWorks/CMakeLists.txt` (existing pattern).

**Installation (fallback only, if spike fails):**
```bash
git submodule add https://github.com/czyt1988/SARibbon.git src/3rdParty/SARibbon
# src/3rdParty/CMakeLists.txt (or a marked // SW-FORK HOOK add_subdirectory): add_subdirectory(SARibbon)
```

## Package Legitimacy Audit

> Native path installs **no external packages** — everything is in-tree Qt 6.8 + FreeCAD Gui. The audit below covers the single conditional fallback dependency.

| Package | Registry | Age | Downloads | Source Repo | slopcheck | Disposition |
|---------|----------|-----|-----------|-------------|-----------|-------------|
| SARibbon | GitHub (vendored submodule, not a package registry) | ~8 yrs (active) | n/a (vendored source) | github.com/czyt1988/SARibbon | n/a (not on npm/PyPI/crates) | Conditional — adopt only on spike failure; vendor-vet gate per UI-SPEC (confirm MIT, pin v2.8.0, clear `fw-string-leak-grep.sh` + `fw-provenance-guard.sh`) |

**Packages removed due to slopcheck [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
**slopcheck status:** not applicable — no npm/PyPI/crates package is installed on the native path; SARibbon (fallback) is vendored C++ source from a known, long-maintained GitHub repo (MIT, verified June 2026). The vendor-vetting gate (UI-SPEC § Registry Safety) governs its adoption, not slopcheck.

## Architecture Patterns

### System Architecture Diagram

```
                          FreeWorks mode active (FwWorkbench::activated)
                                          │
                                          ▼
        ┌───────────────────────────────────────────────────────────────┐
        │ FwLayout::install()  (Gui layer only — observe-the-DOM)         │
        │   • hide MainWindow menuBar() + stock toolbars (D-11, reversible)│
        │   • build + mount FwRibbon in ToolBarArea::TopToolBarArea        │
        └───────────────────────────────────────────────────────────────┘
                                          │
                                          ▼
   ┌──────────────────────────────────────────────────────────────────────┐
   │ FwRibbon (QTabWidget)                                                   │
   │  ┌───────────┬──────────┬───────────┐                                  │
   │  │ Features  │  Sketch  │ Evaluate  │  ← tabs (active = Semibold)       │
   │  └───────────┴──────────┴───────────┘                                  │
   │   each tab page = QWidget hosting QToolBar panel-groups                 │
   └──────────────────────────────────────────────────────────────────────┘
              ▲                                  │ button click
   build      │ reads (data)                     ▼
   ┌──────────┴───────────┐          ┌──────────────────────────────┐
   │ FwRibbonMap (curated │          │ CommandManager               │
   │ C++ table) + auto-   │  resolve │  runCommandByName(id)         │
   │ derive from          │─────────▶│  getCommandByName(id)->getAction()
   │ ToolBarItem groups   │  cmd IDs │  GroupCommand → ActionGroup    │
   │ (D-05/D-07)          │          │   → QToolButton split-button   │
   └──────────────────────┘          └──────────────────────────────┘
                                          (existing backend — no dup)

   Context switch (RIBBON-02):
   App::DocumentObject enters sketch edit
        │
        ▼
   Gui::Application::Instance->signalInEdit(vp)  ──▶ FwRibbon slot:
        if vp.getTypeId().getName()=="SketcherGui::ViewProviderSketch":
            remember currentIndex; setCurrentTab("Sketch")     (D-09/D-10)
   signalResetEdit(vp)  ──▶  restore remembered tab            (D-10)

   Persistence (SC5/D-14):
   QMainWindow::saveState()/restoreState() keyed by objectName ──▶ tab/area restore
```

### Recommended Project Structure

```
src/Gui/FreeWorks/
├── FwRibbon.{h,cpp}        # QTabWidget shell: builds tabs/panels, mounts in TopToolBarArea
├── FwRibbonMap.{h,cpp}     # Declarative curated map (C++ table) + auto-derive fallback (D-05/D-07)
├── FwRibbonContext.{h,cpp} # signalInEdit/ResetEdit subscriber → tab switch (RIBBON-02)
├── FwLayout.{h,cpp}        # EXTEND: mount FwRibbon, hide stock chrome (D-11), persistence hook
├── FwWorkbench.{h,cpp}     # EXTEND: activated() builds ribbon; deactivation restores chrome
└── CMakeLists.txt          # EXTEND: target_sources() += new ribbon files
```

### Pattern 1: Build a large labeled ribbon button from a registry command

**What:** Resolve a command ID to its existing `QAction`, add it to a panel `QToolBar` set for icon-over-label.
**When to use:** Every primary ribbon button (RIBBON-01, D-13).
**Example:**
```cpp
// Source: derived from src/Gui/Command.h:273,1011-1016 + src/Gui/Action.cpp:98
// A panel group is a QToolBar styled for SW large buttons.
auto* panel = new QToolBar(tabPage);
panel->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);   // icon over label
panel->setIconSize(QSize(32, 32));                        // UI-SPEC large-icon metric

Gui::Command* cmd = Gui::Application::Instance->commandManager().getCommandByName(id);
if (cmd) {
    cmd->addTo(panel);   // adds the command's QAction (label, icon, tooltip, shortcut) — thin trigger
}
// firing is handled by the existing QAction; or explicitly:
//   Gui::Application::Instance->commandManager().runCommandByName(id);
```

### Pattern 2: Flyout split-button is a GroupCommand (no custom code)

**What:** A "Comp" command (e.g. `PartDesign_CompPrimitiveAdditive`, `Sketcher_CompLine`) is a `Gui::GroupCommand`; its `ActionGroup::addTo(QToolBar)` already builds a `QToolButton` with `MenuButtonPopup` + a `QMenu` of the sub-commands.
**When to use:** Wherever SW shows a flyout (Fillet ▸ Chamfer, additive primitives, line/arc/circle variants) — D-13 / SC3.
**Example:**
```cpp
// Source: src/Gui/Action.cpp:472-485 (ActionGroup::addTo for a QToolBar)
//   widget->addAction(action());
//   QToolButton* tb = ...constLast();
//   tb->setPopupMode(QToolButton::MenuButtonPopup);
//   tb->setMenu(menu);  // menu->addActions(groupAction()->actions())
// => Curated-map authoring rule: to get a flyout, put the *Comp* command id in the map.
//    e.g. Features panel: "PartDesign_Pad", "PartDesign_CompPrimitiveAdditive", "PartDesign_Fillet", ...
//    The dressup flyout (Fillet/Chamfer/Draft/Thickness) groups via these existing group commands.
```

### Pattern 3: Context tab switch via edit signals (NOT polling)

**What:** Subscribe once to the application-level edit signals; switch to Sketch on enter, restore on exit.
**When to use:** RIBBON-02 / SC4.
**Example:**
```cpp
// Source: src/Gui/Application.h:154-156 ; pattern mirrors src/Gui/OverlayManager.cpp:406-409
namespace sp = std::placeholders; // or a lambda
inEditConn = Gui::Application::Instance->signalInEdit.connect(
    [this](const Gui::ViewProviderDocumentObject& vp) {
        if (std::string(vp.getTypeId().getName()) == "SketcherGui::ViewProviderSketch") {
            previousTab_ = ribbon_->currentIndex();   // D-10 remember
            ribbon_->setCurrentTab("Sketch");         // D-09 context wins
        }
    });
resetEditConn = Gui::Application::Instance->signalResetEdit.connect(
    [this](const Gui::ViewProviderDocumentObject& vp) {
        if (std::string(vp.getTypeId().getName()) == "SketcherGui::ViewProviderSketch") {
            ribbon_->setCurrentIndex(previousTab_);   // D-10 restore
        }
    });
// fastsignals connections are scoped; store them so they disconnect on teardown.
```
> Type-name string comparison avoids a compile/link dependency on the Sketcher module — keeps FreeWorks additive and free of `Mod/Sketcher` includes (App/Gui + merge discipline). Confirmed VP type name is `SketcherGui::ViewProviderSketch` (`PROPERTY_SOURCE_WITH_EXTENSIONS(SketcherGui::ViewProviderSketch, ...)`, `ViewProviderSketch.cpp:562`).

### Anti-Patterns to Avoid

- **Polling `Control::activeDialog()` on a timer for sketch state.** `Control.h` exposes no open/close signal — but the *edit* signals on `Application` are the correct, event-driven source. Polling is laggy and racy. (Use Pattern 3.)
- **`#include` of `Mod/Sketcher/...` from FreeWorks.** Creates a module dependency and merge surface. Identify the sketch by type-name string instead.
- **Editing `MainWindow.cpp` to host the ribbon.** Pitfall 3 + Pitfall 10. Mount through `ToolBarAreaWidget`/`DockWindowManager` from `FwLayout`; any unavoidable shared touch gets `// SW-FORK HOOK`.
- **Hard-coding hex colors / `setStyleSheet()` on buttons.** UI-SPEC mandates palette/`QStyle` roles only this phase; Phase 7 owns theming.
- **Destroying stock toolbars/menu instead of hiding them.** D-11 requires *reversible*, FreeWorks-scoped hiding (`ForceHidden`/`menuBar()->hide()`), restored on workbench deactivation so other workbenches keep stock chrome.
- **Duplicating command logic.** Buttons must be thin triggers into the existing `Command`/`CommandManager` (SC2).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Flyout split-button | Custom `QToolButton`+`QMenu` wiring per group | `GroupCommand`/`ActionGroup` (`*_Comp*` command IDs) | FreeCAD already builds the `MenuButtonPopup` split-button + menu in `Action.cpp` — reuse keeps shortcuts/icons/labels consistent |
| Command→button | New action objects, re-resolved icons/text/shortcuts | `Command::addTo(widget)` / `getAction()` | Single source of truth; auto-syncs with upstream command changes (SC2) |
| Sketch-edit detection | Custom selection/observer polling | `Application::signalInEdit/ResetEdit` | Event-driven, already relayed app-wide, used by OverlayManager |
| Layout persistence | Bespoke INI/JSON of widget geometry | `QMainWindow::saveState()/restoreState()` + `DockWindowManager` | Qt-native, keyed by `objectName`, multi-monitor/float aware (D-14) |
| Hiding stock chrome | Deleting/recreating toolbars | `ToolBarManager::setState(ForceHidden)` + `menuBar()->hide()` | Reversible, scoped, no upstream edit |
| Icon rendering/HiDPI | New SVG raster pipeline | Existing command-action icons via `BitmapFactory` | D-13 authors no icons this phase |

**Key insight:** SolidWorks' "flyout split-button" maps 1:1 onto FreeCAD's existing `GroupCommand`. The curated map's main authoring lever is simply *which command IDs (including the `*_Comp*` group IDs) go in which panel* — the widget mechanics are already built.

## Common Pitfalls

### Pitfall 1: Trying to read sketch state from `Gui::Control`
**What goes wrong:** Planners assume `Control::activeDialog()` is the SC4 hook; it is a getter with no change signal, forcing a polling loop.
**Why it happens:** CONTEXT D-09 says "wired to `Gui::Control` active-dialog / edit state," which reads like Control is the event source.
**How to avoid:** Use `Application::signalInEdit/signalResetEdit` (event-driven). Control's edit state is *consistent with* these signals; the signals are the trigger. Note this reinterpretation explicitly at plan review.
**Warning signs:** A `QTimer` polling Control; tab switch lags the actual sketch entry.

### Pitfall 2: Module coupling via sketch detection
**What goes wrong:** Including Sketcher headers to `dynamic_cast` the view provider breaks additive/merge discipline and adds a link dependency.
**Why it happens:** Type-safe casting feels cleaner than string compare.
**How to avoid:** Compare `vp.getTypeId().getName()` against the literal `"SketcherGui::ViewProviderSketch"`. (Document the literal as a known external type-name contract.)
**Warning signs:** `#include <Mod/Sketcher/...>` appearing under `src/Gui/FreeWorks/`.

### Pitfall 3: Non-reversible chrome hiding leaks into other workbenches
**What goes wrong:** Hiding the menu bar globally (or destroying toolbars) leaves stock FreeCAD workbenches broken after the user switches away from FreeWorks.
**Why it happens:** `menuBar()` / toolbars are owned by the shared `MainWindow`, not the workbench.
**How to avoid:** Hide on `FwWorkbench::activated()`, restore on deactivation (`RestoreDefault` + `menuBar()->show()`). Verify with a headless test that toggles workbenches.
**Warning signs:** Menu bar missing in Part/Sketcher workbench after using FreeWorks.

### Pitfall 4: Layout not round-tripping because of non-unique `objectName`
**What goes wrong:** `QMainWindow::saveState()/restoreState()` silently fails to restore toolbars/docks whose `objectName` is empty or duplicated (Phase 1 already hit this — see `FwLayout.cpp` CR-02 note).
**Why it happens:** Qt keys state by `objectName`.
**How to avoid:** Give the ribbon and each persisted panel a unique, stable `objectName` (e.g. `Fw_Ribbon`, `Fw_RibbonPanel_Features`). Reuse Phase 1's `Fw_*` naming convention.
**Warning signs:** "objectName not unique" warnings; ribbon position resets on restart.

### Pitfall 5: Stranded commands after D-11 hides menus
**What goes wrong:** With menus/toolbars hidden but only the core loop curated (D-06), commands like Std_Save or non-curated workbench commands become unreachable.
**Why it happens:** D-11 + D-06 interaction (D-12 exists precisely to close this).
**How to avoid:** See "Discoverability Escape Hatch" — at minimum keep keyboard shortcuts live (they survive `menuBar()->hide()`); recommended: a "More commands…" overflow button on the ribbon.
**Warning signs:** No way to reach Save/Undo/file ops with menus hidden.

## Code Examples

### Mounting the ribbon in the reserved top area
```cpp
// Source: src/Gui/ToolBarAreaWidget.h:48-64 (addWidget) + Phase-1 FwLayout pattern
// FwLayout extends to insert the ribbon into the TopToolBarArea widget that Phase 1 left free.
// Obtain the top ToolBarAreaWidget via ToolBarManager / MainWindow, then:
//   topArea->addWidget(fwRibbon);   // fwRibbon is the QTabWidget shell
// Give it objectName "Fw_Ribbon" so QMainWindow saveState() can persist it.
```

### Auto-derive a tab for an uncurated workbench (D-07)
```cpp
// Source: src/Gui/ToolBarManager.h:75-87 (ToolBarItem tree) + Workbench::setupToolBars()
// For a workbench with no curated map, walk its ToolBarItem groups; each top-level
// group becomes a panel, each child command() becomes a button:
for (Gui::ToolBarItem* group : workbenchToolBars->getItems()) {
    QToolBar* panel = newPanel(/*title from group*/);
    for (Gui::ToolBarItem* leaf : group->getItems()) {
        const std::string& id = leaf->command();        // "" => separator
        if (!id.empty()) addCommandButton(panel, id);
    }
}
```

### Headless GTest (mirrors Phase 1 `tests/src/Gui/FwWorkbench.cpp`)
```cpp
// Source: tests/src/Gui/FwWorkbench.cpp + tests/src/Gui/CMakeLists.txt
// Add FwRibbon.cpp to Gui_tests_run; use tests::initApplication() in SetUpTestSuite.
// Testable headless:
//  • map → command resolution: every curated id resolves via getCommandByName (or is a known gap)
//  • context switch logic: feed a fake VP type-name through the switch function, assert tab index
//  • persistence round-trip: save then restore a QMainWindow state blob, assert ribbon objectName present
// Needs manual tri-OS checklist (GUI-only): visual large-icon sizing, real flyout popup, live sketch entry.
```

## Curated-Map File Format (Discretion item — recommendation)

**Recommendation: a declarative C++ table compiled into the module**, not JSON.

| Format | Merge-safety | Build integration | Authoring | Verdict |
|--------|-------------|-------------------|-----------|---------|
| **C++ `constexpr`/`std::array` table** | Self-contained under `src/Gui/FreeWorks/` (no shared edits) | Zero — compiles with the module; command IDs are compile-time checked against nothing but typos are caught by the headless test | Slightly more verbose; structured `{tab, panel, cmdId}` rows | **Recommended** |
| JSON resource file | Self-contained | Needs `.qrc` packaging + runtime parse + error handling for missing/typo IDs | Easiest to read/edit; non-coders can edit | Viable but adds IO + parse failure modes; deferred |
| YAML | Self-contained | FreeCAD has `StyleParameters` YAML infra but that's overkill here | Readable | Rejected — extra dep surface for no benefit at v1 |

**Why C++ table:** v1 layout is fixed/curated (D-14, no user customization), so runtime editability buys nothing; a compiled table has no parse/IO failure path, no `.qrc` step, and the headless test can assert every ID resolves. If user-customization arrives later, migrate to JSON then. Confirm at plan review.

### Verified core-loop command IDs (from the actual codebase)

These are real, registered command-ID strings read from `src/Mod/PartDesign/Gui/Workbench.cpp` and `src/Mod/Sketcher/Gui/Workbench.cpp` — the curated map should draw from these (D-08: only real IDs placed). [VERIFIED: codebase grep]

**Features tab (PartDesign):**
- Sketch entry: `PartDesign_NewSketch`
- Additive (flyout via `PartDesign_CompPrimitiveAdditive`): `PartDesign_Pad`, `PartDesign_Revolution`, `PartDesign_AdditiveLoft`, `PartDesign_AdditivePipe`, `PartDesign_AdditiveHelix`
- Subtractive (flyout via `PartDesign_CompPrimitiveSubtractive`): `PartDesign_Pocket`, `PartDesign_Hole`, `PartDesign_Groove`, `PartDesign_SubtractiveLoft`, `PartDesign_SubtractivePipe`, `PartDesign_SubtractiveHelix`
- Dress-up (SW "Fillet ▸ Chamfer" flyout): `PartDesign_Fillet`, `PartDesign_Chamfer`, `PartDesign_Draft`, `PartDesign_Thickness`
- Patterns/transform: `PartDesign_Mirrored`, `PartDesign_LinearPattern`, `PartDesign_PolarPattern`, `PartDesign_MultiTransform`
- Reference/body: `PartDesign_Body`, `PartDesign_ShapeBinder`, `PartDesign_SubShapeBinder`, `PartDesign_Clone`, `PartDesign_Boolean`

**Sketch tab (Sketcher):**
- Lifecycle: `Sketcher_NewSketch`, `Sketcher_EditSketch`, `Sketcher_LeaveSketch`, `Sketcher_MapSketch`, `Sketcher_ValidateSketch`
- Geometry (flyouts via `Sketcher_CompLine`, `Sketcher_CompCreateArc`, `Sketcher_CompCreateConic`): `Sketcher_CreateLine`, `Sketcher_CreatePolyline`, `Sketcher_CreateArc`, `Sketcher_Create3PointArc`, `Sketcher_CreateCircle`, `Sketcher_Create3PointCircle`, `Sketcher_CreateEllipseByCenter`, `Sketcher_CreateRectangle` (verify), constraints group (verify the constraint command IDs in `Sketcher/Gui/CommandConstraints.cpp` at plan time)

**Evaluate tab:** `Part_CheckGeometry`, `Materials_InspectMaterial`, `Materials_InspectAppearance` (plus Measure tools — verify exact `Measure_*`/`Std_Measure*` IDs at plan time).

> **SW tab roster (verify at plan time):** SOLIDWORKS-UI.md confirms the CommandManager is a context-tabbed toolbar but does NOT enumerate the exact SW2024/2025 default tab order. The default SW part-document CommandManager tabs are commonly Features / Sketch / Surfaces / Sheet Metal / Weldments / Markup / Evaluate / MBD Dimensions / SOLIDWORKS Add-Ins — but the exact pinned roster must be confirmed against version-pinned SW Help (`help.solidworks.com/2024/.../c_commandmanager.htm`) before authoring. [ASSUMED] For D-06 the curated set is Features / Sketch / Evaluate (+ optional Surfaces).

## Contextual Switching Mechanism (Discretion item — recommendation)

**Recommendation: signal/observer, NOT polling.** Subscribe to `Gui::Application::Instance->signalInEdit` and `signalResetEdit` (Pattern 3). These are the exact "entered/left edit mode" hooks; `Control::activeDialog()` is a getter with no signal and must not be polled. Sketch identity via `vp.getTypeId().getName() == "SketcherGui::ViewProviderSketch"`. Restore-previous-tab on reset (D-10). [VERIFIED: codebase grep — `Application.h:154-156`, `OverlayManager.cpp:406-409`, `ViewProviderSketch.cpp:562`]

## Discoverability Escape Hatch (Discretion item — recommendation)

**Recommendation (lowest-risk, layered):**
1. **Baseline (mandatory, free):** Keyboard shortcuts survive `menuBar()->hide()` — `QAction` shortcuts remain active while the action exists. Ensure command actions stay alive (they do, owned by `CommandManager`). This alone satisfies D-12's minimum.
2. **Recommended affordance:** A single **"More commands…" overflow `QToolButton`** pinned at the ribbon's right edge, whose menu is built from `CommandManager::getModuleCommands(activeModule)` / `getAllCommands()` grouped by module. Low risk: reuses existing actions, no new backend, one button.
3. **Optional stretch:** A command-search (`QLineEdit` filtering `getAllCommands()` by `getMenuText()`). Higher effort; defer unless the spike has spare budget.

Trade-off: option 2 is the sweet spot — it guarantees no command is stranded (closes the D-11 gap) with minimal surface. Confirm at plan review. [VERIFIED: codebase grep — `Command.h:988-1005`]

## Mount Seam Decision (Discretion item — recommendation)

**Recommendation: direct mount via `FwLayout`/`FwWorkbench`, NOT `WorkbenchManipulator`.**

| Seam | Pros | Cons |
|------|------|------|
| **`FwLayout`/`FwWorkbench` direct** | Already the established Phase 1 activation seam; full control over a single fork-owned workbench; ribbon is a real widget mounted in `TopToolBarArea`; chrome hide/restore naturally scoped to `activated()`/deactivation | Logic lives in FreeWorks (fine — that's the point) |
| `WorkbenchManipulator` | Additive across *all* workbenches; good if you wanted to reshape every workbench's `ToolBarItem` tree | Operates on `ToolBarItem`/`MenuItem` *trees* before setup, not on hosting a custom `QTabWidget` widget; it modifies the toolbar *data*, it does not give you a mount point for a ribbon widget; broader blast radius than needed |

`WorkbenchManipulator::modifyToolBars(ToolBarItem*)` is about rewriting the toolbar *data tree*, whereas the ribbon needs to *host a custom QWidget in a toolbar area* and *switch tabs on edit signals* — both are widget/runtime concerns better owned by `FwLayout`. Use `WorkbenchManipulator` only if a later phase needs to inject the FreeWorks ribbon into non-FreeWorks workbenches (out of scope). Confirm at plan review. [VERIFIED: codebase grep — `WorkbenchManipulator.h:107-117`]

## Spike Plan (D-02/D-03 — recommendation)

**Recommended time-box: 2 working days (≈16h).** Rationale: every primitive is confirmed to exist (mount area, command→button, native flyout, edit signals), so the spike is integration-risk validation, not capability discovery. If 2 days does not reach the bar, the native approach has a real gap and the pre-blessed SARibbon fallback (D-04) triggers without a second decision round.

**Concrete D-03 parity checklist (pass = ALL true):**
- [ ] `QTabWidget` with ≥3 tabs (Features/Sketch/Evaluate) mounted in `TopToolBarArea`, visibly above the 3D view.
- [ ] Each tab shows panel groups (≥2 panels with a 1px separator) of **large icon-over-label** buttons (32px icon, label wraps to 2 lines, e.g. "Extruded\nBoss/Base").
- [ ] Buttons fire the real command (e.g. clicking Pad opens the PartDesign Pad task) — proven via the existing `QAction`.
- [ ] At least one **working flyout split-button** (Fillet ▸ Chamfer/Draft/Thickness via `PartDesign_CompPrimitiveSubtractive` or the dress-up group) — primary action fires, dropdown lists alternatives.
- [ ] **Context tab-switch**: entering a sketch activates the Sketch tab; leaving restores the previous tab (via `signalInEdit/ResetEdit`).
- [ ] Reads "convincingly SW-like" to a SW user (looks bigger than a stock toolbar; tabbed).

**Explicitly NOT in the bar (deferred):** galleries, application button, collapse-to-tab/minimize, final theming/colors/fonts, multi-document-type tabs, user customization.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Improvise ribbon by hacking `MainWindow` toolbars | `ToolBarAreaWidget` custom areas (incl. `TopToolBarArea`) | FreeCAD 2024 | A custom widget can mount in a named toolbar area without `MainWindow.cpp` edits |
| Per-command custom split-button code | `GroupCommand`/`ActionGroup` produce native `MenuButtonPopup` buttons | Long-standing | Flyouts are declarative (just use the `*_Comp*` command ID) |
| Per-workbench observer for edit state | App-level `signalInEdit/ResetEdit` relay | Long-standing | One subscription covers all documents |

**Deprecated/outdated:**
- Qt5 / Qt5-compat APIs — FreeCAD pins Qt 6.8 LTS; Qt5 support deprecated (~2026-08). Use Qt 6.8 widgets only.
- QxRibbon — upstream unmaintained (folded into qtcanpool); SARibbon is the maintained MIT option.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Exact SW2024/2025 default CommandManager tab roster/order (Features/Sketch/Surfaces/Sheet Metal/…/Evaluate) | Curated-Map | Low — D-06 only curates Features/Sketch/Evaluate; verify roster against version-pinned SW Help before authoring |
| A2 | `Sketcher_CreateRectangle` and exact constraint/measure command IDs | Curated-Map | Low — must grep `Sketcher/Gui/CommandConstraints.cpp` and Measure module at plan time; D-08 omit-missing makes a wrong ID a silent no-op, caught by the headless resolution test |
| A3 | 2-day spike time-box is sufficient | Spike Plan | Medium — if exceeded, D-04 SARibbon fallback triggers (already pre-blessed, no decision round) |
| A4 | SARibbon v2.8.0 is the current pinned version to vendor | Standard Stack | Low — confirm latest tag on github.com/czyt1988/SARibbon/releases at adoption time; MIT + Qt 6.8 support verified |

## Open Questions

1. **Exact SW default tab roster (version-pinned).**
   - What we know: CommandManager is a context-tabbed toolbar; D-06 curates Features/Sketch/Evaluate (+ optional Surfaces).
   - What's unclear: the precise SW2024/2025 default tab order and per-tab command grouping.
   - Recommendation: confirm against `help.solidworks.com/2024/...c_commandmanager.htm` during planning; the FreeCAD-side IDs are already verified.

2. **Constraint + measure command IDs for Sketch/Evaluate tabs.**
   - What we know: geometry/lifecycle IDs are verified from Sketcher `Workbench.cpp`.
   - What's unclear: exact constraint command IDs (`Sketcher_Constrain*`) and Evaluate measure IDs.
   - Recommendation: grep `src/Mod/Sketcher/Gui/CommandConstraints.cpp` and the Measure module at plan time; headless test asserts each resolves.

3. **Does hiding stock toolbars via `ForceHidden` round-trip cleanly with FreeWorks deactivation?**
   - What we know: `ToolBarManager` has `ForceHidden`/`RestoreDefault` states; `menuBar()` is on the shared `QMainWindow`.
   - What's unclear: exact restore call sequence on workbench switch.
   - Recommendation: validate in the spike + a headless workbench-toggle test (Pitfall 3).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Qt 6.8 Widgets | Ribbon shell | ✓ (in-tree) | 6.8.x | — |
| FreeCAD Gui (`CommandManager`, `ToolBarAreaWidget`, edit signals) | Whole phase | ✓ (in-tree) | `main` 1.2.0-dev | — |
| GTest | Headless ribbon tests | ✓ (in-tree, `Gui_tests_run`) | repo-pinned | — |
| SARibbon | Fallback ribbon only | ✗ (not vendored) | v2.8.0 (would add) | Native ribbon is the default; SARibbon only on spike failure |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** SARibbon is intentionally absent; the native path needs nothing new.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | GoogleTest (GTest/GMock), in `Gui_tests_run` |
| Config file | `tests/src/Gui/CMakeLists.txt` (add `FwRibbon.cpp`) |
| Quick run command | `ctest -R Gui_tests_run` (or run the `Gui_tests_run` exe) |
| Full suite command | `ctest` from the build tree |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| RIBBON-01 | Curated map IDs all resolve via `getCommandByName` (or are recorded gaps) | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| RIBBON-01 | Ribbon builds ≥3 tabs from the curated map (headless construction) | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| RIBBON-01 | Flyout group resolves to an `ActionGroup` with >1 action (e.g. dress-up group) | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| RIBBON-02 | Switch function: sketch VP type-name → Sketch tab index; reset → previous index | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| SC5/D-14 | `QMainWindow` state save→restore preserves `Fw_Ribbon` objectName | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| D-11 | Workbench toggle hides then restores menu bar/toolbars | unit | `ctest -R Gui_tests_run` | ❌ Wave 0 |
| RIBBON-01/02 (GUI-only) | Large-icon sizing, live flyout popup, real sketch entry, SW look | manual | tri-OS checklist (mirror `TRIOS_LAUNCH_CHECKLIST.md`) | manual |

### Sampling Rate
- **Per task commit:** `ctest -R Gui_tests_run`
- **Per wave merge:** full `ctest`
- **Phase gate:** full suite green + tri-OS manual checklist before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `tests/src/Gui/FwRibbon.cpp` — covers RIBBON-01, RIBBON-02, SC5, D-11 (add to `Gui_tests_run` in `tests/src/Gui/CMakeLists.txt`)
- [ ] Tri-OS manual checklist doc (mirror Phase 1 `TRIOS_LAUNCH_CHECKLIST.md`) for GUI-only criteria

*(Framework already present — no install needed.)*

## Security Domain

> `security_enforcement` not explicitly set; included for completeness. This is a desktop UI phase with no auth/session/network/crypto surface.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — (desktop app, no auth) |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | partial | Command-ID strings in the curated map are developer-authored constants, resolved through `getCommandByName` (null-checked, D-08 omit-missing). No user-supplied input parsed this phase |
| V6 Cryptography | no | — (no crypto; never hand-roll if added later) |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Trademark/asset leak ("SolidWorks" string or proprietary icon) | Repudiation/Legal | `tools/fw-string-leak-grep.sh` + `tools/fw-provenance-guard.sh` CI gates; reuse registry icons (D-13); `Fw` naming |
| Shared-file merge tampering | Tampering | Additive module discipline; `// SW-FORK HOOK` on any unavoidable shared edit |
| App/Gui separation breach | Info Disclosure | No `App/` includes in FreeWorks; read edit state via Gui signals only |
| Unmaintained 3rd-party dep (if SARibbon adopted) | Supply chain | Vendor-vet gate: MIT confirm, pin v2.8.0, leak/provenance grep, no Pixi dep |

## Sources

### Primary (HIGH confidence)
- FreeCAD `main` checkout — read: `src/Gui/ToolBarAreaWidget.h`, `ToolBarManager.h`, `Command.h`, `Action.cpp`, `WorkbenchManipulator.h`, `Control.h`, `Document.h`, `Application.h`, `MainWindow.h`, `DockWindowManager.h`, `MenuManager.h`; `src/Mod/PartDesign/Gui/Workbench.cpp`, `src/Mod/Sketcher/Gui/Workbench.cpp`, `src/Mod/Sketcher/Gui/ViewProviderSketch.cpp`; `src/Gui/FreeWorks/*`; `tests/src/Gui/FwWorkbench.cpp`, `tests/src/Gui/CMakeLists.txt`, `tests/CMakeLists.txt`
- `.planning/research/STACK.md`, `.planning/research/PITFALLS.md` (Pitfall 10), `.planning/research/SOLIDWORKS-UI.md`
- `.planning/phases/02-commandmanager-ribbon/02-CONTEXT.md`, `02-UI-SPEC.md`
- `.planning/phases/01-solidworks-mode-foundation/01-01-SUMMARY.md`

### Secondary (MEDIUM confidence)
- https://github.com/czyt1988/SARibbon — MIT, Qt 5.12–6.8 LTS, CMake + amalgamated header (verified June 2026)

### Tertiary (LOW confidence)
- SW2024/2025 default CommandManager tab roster — to be confirmed against version-pinned `help.solidworks.com` at plan time (A1)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — every API verified by reading the actual headers in this checkout
- Architecture / mount / flyout / context switch: HIGH — confirmed in `Action.cpp`, `Application.h`, `OverlayManager.cpp`, `ToolBarAreaWidget.h`
- Curated command IDs (FreeCAD side): HIGH — read directly from PartDesign/Sketcher `Workbench.cpp`
- SW tab roster (SolidWorks side): LOW — needs version-pinned SW Help confirmation
- Pitfalls: HIGH — grounded in Phase 1 experience + project PITFALLS.md

**Research date:** 2026-06-07
**Valid until:** ~2026-07-07 (FreeCAD `main` is a moving target; re-verify seam APIs if the upstream pin advances significantly)

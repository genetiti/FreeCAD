# Architecture Research

**Domain:** SolidWorks-style UI fork of FreeCAD's C++ Gui layer (tracking upstream `main`, 1.2.0-dev)
**Researched:** 2026-06-06
**Confidence:** HIGH (grounded in direct reads of `src/Gui` in this checkout; hook points verified against actual class/method signatures)

## Standard Architecture

The fork should add a **SolidWorks presentation layer** that sits *beside* FreeCAD's existing Gui layer and drives the unchanged App DOM through the existing public Gui singletons (`Selection`, `Command`, `Control`, `Gui::Document`). The App layer (Document Object Model, recompute, OCCT) is never touched.

The single most important architectural finding: **almost every SolidWorks surface has an existing FreeCAD extension seam** — you do not need to fork shared widgets, you register alternatives by name or select existing implementations via preference. One surface (navigation) is *already fully implemented in-tree* (`Gui::SolidWorksNavigationStyle`).

### System Overview

```
┌──────────────────────────────────────────────────────────────────────┐
│                 SolidWorks Presentation Layer  (NEW)                  │
│                    src/Gui/SolidWorks/  (additive module)            │
│  ┌────────────┐  ┌──────────────┐  ┌───────────────┐  ┌───────────┐  │
│  │ Ribbon /   │  │ FeatureMgr   │  │ PropertyMgr   │  │ SW Theme  │  │
│  │ CommandMgr │  │ (tree reskin)│  │ (TaskView host)│  │ (qss+icons)│ │
│  └─────┬──────┘  └──────┬───────┘  └──────┬────────┘  └─────┬─────┘  │
│        │ uses           │ wraps           │ hosts            │ styles │
└────────┼────────────────┼─────────────────┼──────────────────┼───────┘
         ▼                ▼                 ▼                  ▼
┌──────────────────────────────────────────────────────────────────────┐
│              EXISTING FreeCAD Gui Layer  (unchanged / extended)       │
│  CommandManager   TreeWidget/ComboView   Control+TaskView   Selection │
│  Workbench(setupToolBars/MenuBar/Dock)   DockWindowManager            │
│  View3DInventorViewer + NavigationStyle  (SolidWorksNavigationStyle ✓)│
└────────────────────────────────┬─────────────────────────────────────┘
                                 ▼  (signals / Selection / Command — no edits)
┌──────────────────────────────────────────────────────────────────────┐
│        EXISTING FreeCAD App Layer  (DOM, recompute, OCCT) — FROZEN    │
│        App::Document  App::DocumentObject  Property system            │
└──────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Concrete FreeCAD hook point(s) |
|-----------|----------------|--------------------------------|
| **Ribbon / CommandManager** | Tabbed ribbon (Features/Sketch/Evaluate) replacing menubar+toolbars | `Gui::Workbench::setupToolBars()` / `setupMenuBar()` as the *source of truth* for commands; a new `QWidget` ribbon hosted in `MainWindow`'s top `ToolBarAreaWidget` (`src/Gui/ToolBarAreaWidget.h`, `ToolBarArea::TopToolBarArea`); commands fired via existing `Gui::CommandManager` |
| **FeatureManager** | Ordered design tree + rollback bar | Reskin/subclass `Gui::TreeWidget` (`src/Gui/Tree.h`), a `QTreeWidget` + `SelectionObserver` + `DocumentObserver`; register under a new dock name via `DockWindowManager::registerDockWindow()` |
| **PropertyManager** | Left sliding panel: command options + feature edit | Host existing `Gui::TaskView::TaskView` driven by `Gui::ControlSingleton` (`src/Gui/Control.h` — `showDialog`/`accept`/`reject`) on the left dock area; reuse `Gui::PropertyEditor` (`src/Gui/propertyeditor/`) for static feature properties |
| **Navigation** | SW mouse rotate/pan/zoom + selection feel | **Already exists:** `Gui::SolidWorksNavigationStyle` (`src/Gui/Navigation/SolidWorksNavigationStyle.cpp`), registered in `SoFCDB.cpp`, selected via the `NavigationStyle` preference key. No code needed beyond defaulting it. |
| **Selection parity** | SW selection model, filters, context toolbar | `Gui::SelectionSingleton` (`src/Gui/Selection/Selection.h`) — observe `signalSelectionChanged`, drive `SelectionFilter`, `setSelectionStyle()`; context toolbar is a new overlay widget reacting to selection signals |
| **SW Theme** | Look-alike icons, colors, fonts, layout | Qt stylesheet (qss) + recreated icon set via `Gui::BitmapFactory`; no C++ structural change |
| **SolidWorks mode switch** | One toggle that wires all of the above | A new C++ `Gui::Workbench` subclass ("SolidWorks") OR a preference-driven `MainWindow` layout applied at startup |

## Recommended Project Structure

Keep everything additive under one new directory so upstream diffs stay surgical:

```
src/Gui/
├── SolidWorks/                  # NEW — the entire SW presentation layer
│   ├── SwRibbon.{h,cpp}         # ribbon widget; consumes ToolBarItem/MenuItem trees
│   ├── SwRibbonTab.{h,cpp}      # one tab (Features/Sketch/Evaluate…)
│   ├── SwCommandBar.{h,cpp}     # SW "CommandManager" strip mapping
│   ├── SwFeatureManager.{h,cpp} # TreeWidget subclass/reskin + rollback bar
│   ├── SwRollbackBar.{h,cpp}    # rollback bar overlay on the tree
│   ├── SwPropertyManager.{h,cpp}# left-dock host for Control/TaskView + PropertyEditor
│   ├── SwSelectionContextBar.{h,cpp} # context toolbar driven by Selection signals
│   ├── SwWorkbench.{h,cpp}      # Gui::Workbench subclass = "SolidWorks mode" entry point
│   ├── SwLayout.{h,cpp}         # applies dock layout / installs ribbon into MainWindow
│   ├── SwTheme.{h,cpp}          # loads qss + look-alike icons via BitmapFactory
│   └── CMakeLists.txt
└── (existing files — touched only at thin, well-known seams; see merge strategy)
```

### Structure Rationale

- **`src/Gui/SolidWorks/`:** One self-contained subtree means a single new entry in `src/Gui/CMakeLists.txt` and almost no edits to existing `.cpp` files — the key to staying mergeable with a moving upstream.
- **`SwWorkbench` as entry point:** FreeCAD already calls `Workbench::activate()` → `setupToolBars()` / `setupDockWindows()` (`src/Gui/Workbench.cpp:450-479`). Subclassing `Gui::Workbench` lets the SW layout be *installed by the existing activation machinery* with zero edits to `MainWindow`'s startup path.
- **No `App/` changes:** The whole tree lives under `src/Gui/` and talks to the DOM only through public Gui singletons.

## Architectural Patterns

### Pattern 1: Additive Module + Workbench Activation (the mergeability backbone)

**What:** Implement the SW UI as a new `Gui::Workbench` subclass that overrides `setupToolBars()`, `setupMenuBar()`, `setupDockWindows()` to return the SW layout, plus an `activated()` override that installs the ribbon widget and applies the dock layout.
**When to use:** This is the primary integration mechanism — favor it for everything that can be expressed as "what UI shows up."
**Trade-offs:** Pro — rides existing, stable extension points; survives upstream churn; cleanly toggleable. Con — a ribbon is not a `QToolBar`, so `setupToolBars()` alone can't fully express it; you consume the returned `ToolBarItem*`/`MenuItem*` command trees as *data* and render them in your own ribbon widget instead of handing them to `ToolBarManager`.

**Example:**
```cpp
// SwWorkbench.cpp
ToolBarItem* SwWorkbench::setupToolBars() const {
    ToolBarItem* root = StdWorkbench::setupToolBars(); // reuse command set
    return root; // SwRibbon reads this tree; ToolBarManager is bypassed for the ribbon
}
void SwWorkbench::activated() {
    SwLayout::install(getMainWindow());   // mount ribbon in TopToolBarArea, set dock layout
}
```

### Pattern 2: Observe-the-DOM, Never-Mutate-Directly

**What:** Every SW widget is a *view* that subscribes to existing signals (`SelectionSingleton::signalSelectionChanged`, `Gui::Document` object signals, `DocumentObserver`) and issues changes only through `Gui::Command` / property setters / `Control().accept()`.
**When to use:** Always — this is how the UI stays correct without touching App.
**Trade-offs:** Pro — undo/redo, recompute, and dependency tracking keep working for free; no App edits. Con — you must wire observers in `attach`/`detach` and avoid the documented anti-pattern of direct property mutation.

**Example:**
```cpp
class SwFeatureManager : public Gui::TreeWidget {   // already a SelectionObserver
    void onSelectionChanged(const SelectionChanges& msg) override; // sync highlight
    void slotNewObject(const Gui::ViewProviderDocumentObject&);     // add tree row
};
```

### Pattern 3: Host, Don't Reimplement (PropertyManager over TaskView)

**What:** The PropertyManager is a *container* that docks the existing `TaskView` (driven by `Control()`) on the left, plus the existing `PropertyEditor` for non-command feature editing. SolidWorks "PropertyManager when running a command" maps 1:1 onto FreeCAD's `Control().showDialog(TaskDialog*)`.
**When to use:** For all command-time option panels and feature edit panels.
**Trade-offs:** Pro — every existing PartDesign/Sketcher task panel works unmodified inside the SW PropertyManager. Con — visual restyling only; deep behavioral SW parity (e.g., PMP green-check/red-X header) is a reskin of `TaskView`/`TaskDialog` chrome, not a rewrite.

## Data Flow

### Command / Feature-edit Flow (SW → DOM → SW, no App edits)

```
[Ribbon button click]
    ↓  CommandManager::runCommandByName()
[Gui::Command::activate()]                         (existing module command)
    ↓
[Control().showDialog(TaskDialog)] ──hosted in──► [SwPropertyManager (left dock)]
    ↓ user fills options, presses OK
[Command opens App::Document transaction, sets Properties]
    ↓ (App recompute — UNCHANGED)
[Gui::Document slotChangedObject → ViewProvider::update → Coin3D redraw]
    ↓ signals
[SwFeatureManager adds/updates row]  +  [SwPropertyManager closes panel]
```

### Selection Flow

```
[3D pick OR FeatureManager click]
    ↓
[SelectionSingleton::addSelection()]               (existing)
    ↓ signalSelectionChanged
┌──────────────┬───────────────────┬─────────────────────────┐
▼              ▼                   ▼                         ▼
SwFeatureMgr   SwSelectionContextBar  SwPropertyManager     3D highlight
(highlight)    (show context tools)   (show props of sel)   (existing VP)
```

### Key Data Flows

1. **UI ↔ DOM is one-way-in, signal-out:** SW widgets *write* only via `Command`/`Property`/`Control`; they *read* state only via signals and getters. No SW widget holds authoritative state — the App DOM does.
2. **Navigation is fully self-contained:** Mouse events flow into `View3DInventorViewer` → `SolidWorksNavigationStyle::processSoEvent` (already implemented). The fork only ensures this style is the default (`NavigationStyle` preference key, value `Gui::SolidWorksNavigationStyle`).

## Upstream-Mergeability Strategy

**Verdict: additive `src/Gui/SolidWorks/` module + a SolidWorks `Gui::Workbench`, with the SW layout applied via existing Workbench activation and dock registration — NOT deep edits to `MainWindow`.** Confidence: HIGH.

| Surface | Recommended approach | Why this minimizes merge conflicts |
|---------|----------------------|-----------------------------------|
| Navigation | **Zero edits** — default the existing `Gui::SolidWorksNavigationStyle` via preference | Already upstream; just a config default |
| Ribbon | New `SwRibbon` widget mounted in `MainWindow`'s `TopToolBarArea` from `SwWorkbench::activated()` | Uses public `getMainWindow()` + `ToolBarAreaWidget`; consumes the `ToolBarItem`/`MenuItem` command trees as data |
| FeatureManager | Subclass `Gui::TreeWidget`; register as a *new* dock name (e.g. `Sw_FeatureManager`) via `DockWindowManager::registerDockWindow()` | New dock name avoids touching the `Std_TreeView`/`Std_ComboView` setup blocks in `MainWindow.cpp` |
| PropertyManager | New left-dock host wrapping existing `TaskView`/`Control` + `PropertyEditor` | Reuses public `Control()` API; no shared-widget edits |
| Selection / context bar | New observer widget on `SelectionSingleton` signals | Pure consumer of existing signals |
| Theme | qss + icons via `BitmapFactory` | Resource-only |
| SolidWorks mode toggle | A `Gui::Workbench` subclass (preferred) registered like any module workbench | Rides the existing workbench plugin path; one-line registration |

**Why a Workbench (not in-place `MainWindow` edits):** `MainWindow.cpp`'s dock-setup methods (`setupTaskView`, `updateTreeView`, `updateComboView`, …, around lines 609-804) and the workbench `activate()` path (`Workbench.cpp:450`) are exactly the seams upstream changes most often. Re-rendering the SW layout from a Workbench subclass means upstream refactors to those methods rarely collide with your code. The few unavoidable shared-file edits — registering the new module in `src/Gui/CMakeLists.txt`, and possibly one default in the nav-style preference — are small, localized, and conflict-resistant.

**Discipline to keep merges clean:**
- Keep all SW code under `src/Gui/SolidWorks/`; never edit a shared `.cpp` body where a public API or new dock-name registration would do.
- Track upstream `main` on a regular cadence; the additive surface keeps each rebase to CMake + a handful of include points.
- Treat any required edit to a shared file as a code smell — prefer subclass/register/observe.

## Build Order (dependencies)

```
0. SolidWorks navigation default  ──► trivial; ship first (already implemented in-tree)
1. SW module skeleton + SwWorkbench + SwLayout + SwTheme
       │  (gives a registrable "SolidWorks mode" + theming shell)
       ▼
2. Ribbon (SwRibbon/SwRibbonTab)        ── depends on (1) for hosting + command trees
       ▼
3. FeatureManager (SwFeatureManager)    ── depends on (1) for dock registration; pairs with Selection
       ▼
4. PropertyManager (SwPropertyManager)  ── depends on (1) dock + Control/TaskView; consumes Selection
       ▼
5. Selection parity + context bar       ── threads through 3 & 4; finalizes interaction feel
```

**Rationale:** Navigation first because it is free and immediately demonstrates "SW feel." The module skeleton/Workbench (1) must exist before any panel because it owns the layout and hosting. Ribbon (2) before tree/property because it's the dominant visual and exercises the command-tree consumption that later panels also rely on. FeatureManager (3) before PropertyManager (4) because the property panel's "show props of selected feature" depends on tree selection being wired. Selection parity (5) is woven through 3-4 and tuned last once both panels can react to it.

## Anti-Patterns

### Anti-Pattern 1: Deep-editing `MainWindow`/`ComboView`/`Tree` in place

**What people do:** Rip out menubar/toolbar code in `MainWindow.cpp` and rewrite `ComboView`/`TreeWidget` bodies to look like SolidWorks.
**Why it's wrong:** These are upstream's most actively changed Gui files; every rebase becomes a manual merge battle, defeating the "track `main`" requirement.
**Do this instead:** Add a `Gui::Workbench` subclass and new dock-registered widgets; subclass `TreeWidget` rather than editing it; mount the ribbon via the public `MainWindow`/`ToolBarAreaWidget` API.

### Anti-Pattern 2: Re-implementing a navigation style from scratch

**What people do:** Write a brand-new Coin3D event handler for SW mouse behavior.
**Why it's wrong:** `Gui::SolidWorksNavigationStyle` already exists, is registered (`SoFCDB.cpp`), and is selectable via the `NavigationStyle` preference. Re-implementing duplicates maintained code and diverges from upstream.
**Do this instead:** Default to `Gui::SolidWorksNavigationStyle`; if behavior gaps exist, contribute fixes *upstream* to that class so the fork carries no nav diff.

### Anti-Pattern 3: Mutating the App DOM directly from SW widgets

**What people do:** A SW panel sets `obj->Placement = ...` or edits document objects to reflect UI state.
**Why it's wrong:** Bypasses property change notifications, undo/redo, and dependency tracking (documented in codebase ARCHITECTURE.md). Breaks the App/Gui contract this whole project depends on.
**Do this instead:** Go through `Gui::Command` + property setters and `Control().accept()`; let the existing transaction/recompute machinery run untouched.

## Integration Points

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| SW layer ↔ App DOM | `Gui::Command`, `Property` setters, `Control().accept()` (write); signals + getters (read) | No App-layer edits; the hard invariant of the project |
| SW Ribbon ↔ Commands | `Gui::CommandManager::runCommandByName()`; command trees from `setupToolBars()`/`setupMenuBar()` | Ribbon renders the trees itself instead of handing them to `ToolBarManager` |
| SW FeatureManager ↔ Selection/Document | `SelectionObserver` + `DocumentObserver` (`TreeWidget` already implements both) | Subclass, don't fork |
| SW PropertyManager ↔ TaskView | `Gui::ControlSingleton` (`showDialog`/`accept`/`reject`) + `Gui::PropertyEditor` | Existing module task panels work unmodified inside it |
| SW context bar ↔ Selection | `SelectionSingleton::signalSelectionChanged`, `SelectionFilter`, `setSelectionStyle()` | Pure consumer |
| SW mode ↔ MainWindow | `Gui::Workbench::activate()` path + `DockWindowManager::registerDockWindow()` | The chosen, low-conflict install seam |

## Sources

- Direct reads of working checkout (commit `768e237091`):
  - `src/Gui/Workbench.{h,cpp}` — `setupToolBars/MenuBar/DockWindows`, `activate()` flow (HIGH)
  - `src/Gui/MainWindow.{h,cpp}` — dock setup/registration (`registerDockWindow`, `setupTaskView`, `updateTreeView`, `updateComboView`) (HIGH)
  - `src/Gui/DockWindowManager.h` — `registerDockWindow`/`addDockWindow` API (HIGH)
  - `src/Gui/Navigation/SolidWorksNavigationStyle.cpp`, `SoFCDB.cpp` — SW nav style exists + registered; `View3DInventor.cpp` `NavigationStyle` preference key (HIGH)
  - `src/Gui/Control.h`, `src/Gui/TaskView/TaskView.h` — PropertyManager host mechanism (HIGH)
  - `src/Gui/Tree.h` — `TreeWidget` is `QTreeWidget`+`SelectionObserver`, has `slotNewObject`/`onSelectionChanged` (HIGH)
  - `src/Gui/Selection/Selection.h` — `signalSelectionChanged`, `SelectionFilter`, `setSelectionStyle` (HIGH)
  - `src/Gui/ToolBarAreaWidget.h`, `ToolBarManager.h`, `MenuManager.h` — ribbon mount area + command-tree model (HIGH)
- `.planning/PROJECT.md`, `.planning/codebase/ARCHITECTURE.md`, `STRUCTURE.md`, `CONVENTIONS.md` (project context)

---
*Architecture research for: SolidWorks-style UI fork of FreeCAD Gui layer*
*Researched: 2026-06-06*

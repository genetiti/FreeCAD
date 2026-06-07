# Feature Research

**Domain:** SolidWorks-faithful UI/UX layer on top of FreeCAD's parametric modeling engine (CAD desktop application, migration-focused)
**Researched:** 2026-06-06
**Confidence:** HIGH (SW feature set verified against SOLIDWORKS official help + multiple training/reseller sources; FreeCAD current state verified directly against the `src/Gui` and `src/Mod/PartDesign` source in this checkout)

> Scope note: This document maps the **interaction/presentation feature set** that constitutes "the SolidWorks experience." It does NOT cover modeling operations (extrude/revolve/loft/etc.) — those already exist in FreeCAD's PartDesign/Part/Sketcher and are validated in PROJECT.md. Every item below is a UI/UX behavior a SolidWorks user reaches for by muscle memory.
>
> **Key prior-art finding:** FreeCAD already ships a `Gui::SolidWorksNavigationStyle` (`src/Gui/Navigation/SolidWorksNavigationStyle.cpp`) that reproduces SW's rotate=MMB / pan=Ctrl+MMB / zoom=wheel conventions. Selection filters (`src/Gui/Selection/SelectionFilter.cpp`), box selection (`BoxSelection.cpp`), and hover pre-selection highlight (`SoFCUnifiedSelection.cpp`) all exist. The bulk of the gap is the **interaction "shells"** that wrap these primitives: the ribbon, the slide-in PropertyManager, and the on-canvas accelerators (S-key, mouse gestures, confirmation corner, heads-up toolbar, context toolbar/breadcrumb).

---

## Feature Landscape

### Table Stakes (SW user leaves without these)

These are the interface elements a SolidWorks user sees within the first 10 seconds and uses constantly. If any is missing or behaves wrong, the "zero relearning" promise breaks immediately.

| # | SW Feature | What the SW user expects | FreeCAD current equivalent | The parity gap | Complexity |
|---|-----------|--------------------------|----------------------------|----------------|------------|
| T1 | **CommandManager ribbon** | Tabbed ribbon docked at top (Features / Sketch / Surfaces / Sheet Metal / Evaluate / Assembly...). Large icons with text labels, flyout split-buttons (e.g. Fillet ▾ → Chamfer), tab switches by context. | FreeCAD uses classic Qt menus + small toolbars + a *workbench selector combo*. No ribbon. `ToolBarManager.cpp` manages flat toolbars; commands registered via `Gui::CommandManager`. | Build a true ribbon widget (`QTabWidget`-style container hosting grouped large-icon `QToolButton`s with flyouts), driven by the existing Command registry. Map SW tabs → FreeCAD workbenches/commands. The command *backend* exists; the ribbon *frontend* is net-new. | **HIGH** |
| T2 | **FeatureManager design tree (feature history)** | Left-docked ordered tree of features in creation order; origin/planes node at top; each feature expandable to its sketch; icons per feature type. | FreeCAD `Tree.cpp` / `TreeView.cpp` shows a document tree with PartDesign Body holding ordered features and sketches nested under them. Concept is *already present and ordered*. | Largely **present**. Gaps are cosmetic/behavioral: SW puts it on the **left** (FreeCAD `Std_TreeView` defaults to right dock), SW shows a flat ordered feature list (FreeCAD's tree is more document-object-graph flavored), and SW icon/label conventions differ. Re-skin + re-dock + ordering presentation. | **MEDIUM** |
| T3 | **Rollback bar** | A draggable horizontal bar in the design tree; dragging it up "rolls back" the model to an earlier feature state (suppresses everything below) for inserting/editing mid-history. | PartDesign has the underlying concept: a Body has a **Tip** (`Body.h showTip`, `ViewProviderBody.cpp setTipIcon`) marking the active/last-computed feature; `moveTip`-style operations exist at the App level. But there is **no draggable rollback-bar widget** and rolling the tip is done via context menu ("Set tip"), not a drag bar. | The *engine capability* (suppress features after a point, recompute to a tip) exists. Missing: the draggable bar UI in the tree, hover/drag affordance, and "insert before tip" command flow. This is the single highest-value SW tree behavior. | **HIGH** |
| T4 | **PropertyManager (left slide-in panel)** | When you start a feature or sketch command, a panel slides in on the **left** with: a title bar with green-✓/red-✗ pin, collapsible rollout groups, **selection reference boxes** (blue/pink list boxes you click into then pick geometry), live-updating numeric fields, and dynamic options that appear/disappear based on choices. Model updates live as you type. | FreeCAD's `Gui::TaskView` / `ControlSingleton` (`Control.h showDialog`) is the direct analog: a `TaskDialog` panel with collapsible task boxes, OK/Cancel, and live preview. It docks in the **ComboView/right** by default. PartDesign feature dialogs (`TaskFeatureParameters.cpp`) already do live preview + selection. | The *mechanism* is a near-perfect match — this is FreeCAD's strongest existing parity. Gaps: (a) dock it **left**, slide-in animation; (b) the **selection-reference-box** idiom (click box → it highlights → picked geometry fills it, with add/remove) is only partially present in FreeCAD dialogs and needs a reusable widget; (c) green-✓ pin styling; (d) SW's "always-on" PropertyManager tab. | **MEDIUM–HIGH** |
| T5 | **Mouse navigation conventions** | Rotate = MMB drag; Pan = Ctrl+MMB (or MMB+arrow keys); Zoom = scroll wheel (and zoom-to-cursor); double-MMB or middle-click-on-entity to rotate about it. | **Already implemented**: `Gui::SolidWorksNavigationStyle` reproduces exactly this (verified: SELECTION=LMB, PANNING=Ctrl+MMB, DRAGGING/rotate=MMB, ZOOMING=wheel). | **Mostly present.** Gaps: make it the **default** nav style for this fork (FreeCAD defaults to CAD/Gesture), verify zoom-to-cursor direction + speed match SW, and "rotate about clicked point" parity. Low remaining work. | **LOW** |
| T6 | **Selection model (click + box + filters)** | Left-click selects face/edge/vertex; click empty space deselects; box-select left→right = enclosed only, right→left = crossing; selection filters (faces only, edges only) toggled by toolbar/F-keys; Ctrl-click to multi-select. | `Selection.cpp` (unified selection), `BoxSelection.cpp` (rubber-band), `SelectionFilter.cpp` (filter grammar), `SoFCUnifiedSelection.cpp` (pick resolution). All primitives present. | The selection *primitives* exist. Gaps: SW's **left-vs-right box-select semantics** (enclose vs crossing) — FreeCAD box-select is uniform; SW filter **toolbar/F-key bindings** and the floating filter toolbar UI; matching highlight colors. Mostly wiring + a filter toolbar. | **MEDIUM** |
| T7 | **Hover pre-selection highlight** | Hovering any face/edge/vertex highlights it (preselection) before clicking, with the entity name shown. | **Present**: `SoFCUnifiedSelection.cpp` does preselection highlighting; status bar shows the hovered element. | **Mostly present.** Gap is cosmetic: SW highlight color/feel and the on-hover tooltip styling. | **LOW** |
| T8 | **In-tree feature editing (double-click / rename)** | Double-click a feature in the tree → opens its PropertyManager to edit; slow-double-click or F2 to rename; expand to see/edit the sketch. | **Present**: `ViewProvider::doubleClicked()` + `setEdit()` open the feature's task dialog; tree supports rename (F2) and the feature→sketch nesting. | **Mostly present.** Gap: route double-click to the (left, SW-styled) PropertyManager from T4 and match SW's edit-entry behavior. Inherits T4's work. | **LOW** (given T4) |
| T9 | **Drag-to-reorder features in tree** | Drag a feature up/down in the FeatureManager to change its position in history (subject to dependency validity). | **Present**: FreeCAD `Tree.cpp` supports drag-and-drop of tree items; PartDesign enforces dependency/feature-order rules. | **Mostly present.** Gap: SW's drag affordance/insertion-line visuals and validity feedback. Behavior largely exists. | **LOW–MEDIUM** |
| T10 | **Sketch → feature flow conventions** | Pick a plane/planar face → "Sketch" → draw → exit sketch → the just-finished sketch is pre-selected and the Features tab is ready → pick Extrude → PropertyManager opens with that sketch as the profile. | FreeCAD PartDesign does exactly this flow (sketch on plane/face → close sketch → create pad/pocket using that sketch), via task dialogs. | **Mostly present** in workflow; gaps are presentation: SW auto-activates the **Features tab** after sketch exit, auto-selects the finished sketch as profile, and the on-screen "Exit Sketch" confirmation-corner affordance (see T13). Plumbing of tab-switch + auto-select. | **MEDIUM** |
| T11 | **Heads-up View toolbar** | A floating, semi-transparent toolbar at the **top of the graphics area** with: view orientation (front/top/iso + view-cube/spaceball menu), display style (wireframe/shaded/shaded-with-edges), hide/show, section view, zoom-to-fit, apply-scene/appearance. Always visible, on-canvas. | FreeCAD has these commands (`Std_ViewFitAll`, view orientation, draw styles) but they live in the **menu/toolbar at window chrome level**, not as an on-canvas floating overlay. `OverlayManager.cpp`/`OverlayWidgets.cpp` provide a canvas-overlay mechanism that could host it. | Build a floating overlay toolbar anchored top-center of the 3D view, populated with the existing view commands. Overlay infra exists (`OverlayManager`); the SW-styled heads-up widget is net-new. | **MEDIUM** |
| T12 | **View orientation cube / standard views** | Quick front/top/right/iso buttons + a navigation cube in a corner; spacebar opens the orientation dialog. | FreeCAD has standard view commands and a **Navigation Cube** (`NaviCube`) already rendered in the corner of the 3D view. | **Mostly present** (NaviCube exists). Gaps: spacebar→orientation-menu binding, SW cube styling. | **LOW** |

### Differentiators (parity polish — SW power users notice, casual users survive without)

These distinguish a *faithful* SW clone from an *approximate* one. They are the "muscle memory accelerators" SW power users rely on; reproducing them is what makes the fork feel genuinely like SW rather than "FreeCAD wearing a costume."

| # | SW Feature | Value proposition (why a SW migrant values it) | FreeCAD current equivalent | The parity gap | Complexity |
|---|-----------|-----------------------------------------------|----------------------------|----------------|------------|
| D1 | **S-key shortcut bar** | Press `S` → a small, fully customizable, context-aware command palette pops up **at the cursor** (different sets for part/assembly/sketch/drawing). Core to SW power-user speed. | No equivalent. FreeCAD has global keyboard shortcuts and a command completer (`CommandCompleter.cpp`) but no on-cursor context toolbar. | Net-new: a frameless popup toolbar spawned at cursor on `S`, populated per-context (sketch vs part) from the Command registry, user-customizable. High muscle-memory value, self-contained scope. | **MEDIUM** |
| D2 | **Mouse gestures (RMB radial "donut")** | Hold RMB + drag in a direction → fires a mapped command; the 4- or 8-way radial guide appears. Configurable per context. Lets power users invoke common ops without moving to the ribbon. | No equivalent. FreeCAD's `GestureNavigationStyle` is a *touch/navigation* gesture system, NOT SW-style command gestures. | Net-new: RMB-hold detection that distinguishes a *gesture drag* from a *context-menu click* (the classic SW ambiguity — see anti-feature A4), radial overlay, per-direction command mapping, per-context profiles. | **MEDIUM–HIGH** |
| D3 | **Confirmation corner** | A translucent ✓/✗ overlay in the **top-right corner of the graphics area** while in a mode (sketch, feature edit); click ✓ to accept / ✗ to cancel without going to the panel. `D` key teleports it to the cursor. | No on-canvas confirmation corner. FreeCAD confirms via the TaskDialog OK/Cancel buttons in the panel. | Net-new overlay tied to active-edit state (sketch mode, feature edit). Should reuse the active TaskDialog's accept/reject. The `D`-key "bring to cursor" is a nice touch. | **MEDIUM** |
| D4 | **Context toolbar + breadcrumb (on-selection mini toolbar)** | Selecting geometry pops a small floating mini-toolbar near the cursor with the most likely next commands; a **breadcrumb** trail (top-left of canvas) shows the selection's parent chain (Body > Feature > Face) and lets you select up the hierarchy. | FreeCAD shows a **right-click context menu** with relevant actions; no on-selection floating mini-toolbar and no breadcrumb. | Net-new: a context-sensitive floating toolbar triggered on selection (reuse Command registry to pick relevant commands), plus a breadcrumb widget reading the selection's owner chain from the DOM. | **MEDIUM–HIGH** |
| D5 | **PropertyManager selection-reference boxes (full fidelity)** | The blue/pink "selection list" boxes with right-click "Clear Selections", reordering, and the active-box auto-advance behavior. | Partial: FreeCAD task dialogs have selection buttons/lists but not the polished SW idiom. | Build one reusable `SelectionBoxWidget` (binds to a link/list property, toggles selection gate, shows picked refs, supports clear/remove) used across all feature dialogs. Strongly enhances T4. | **MEDIUM** |
| D6 | **Filter toolbar (F5/F6 + floating bar)** | Floating selection-filter toolbar; `F5` toggles filter on/off, `F6`/keys toggle face/edge/vertex filters. | `SelectionFilter` grammar exists but no SW-style floating filter bar or F-key bindings. | Wire a floating filter toolbar to the existing `SelectionFilter` engine; add F-key bindings. Enhances T6. | **LOW–MEDIUM** |
| D7 | **"Instant3D" / on-canvas drag handles** | Drag arrows/rulers appear on a feature to change extrude depth, fillet radius, etc. directly in the viewport. | FreeCAD has 3D dragger nodes (Coin draggers) used in some editors but not a general SW-style Instant3D layer. | Net-new general handle system on features. High effort, lower muscle-memory necessity than S-key/gestures. Genuine differentiator if done. | **HIGH** |
| D8 | **SW visual theme (look-alike icons/colors/fonts)** | The whole thing *looks* like SW: icon style, blue accent, panel chrome, fonts. | FreeCAD is fully themeable via Qt stylesheets + icon sets; `BitmapFactory` handles icons. | Recreate **look-alike** (not copied) icon set + Qt stylesheet + color scheme. Spread across all phases; cross-cuts every widget above. Legal: recreated assets only (per PROJECT.md Out of Scope). | **MEDIUM** (but broad/ongoing) |

### Anti-Features (deliberately do NOT replicate)

| # | SW "feature" | Why it's tempting | Why it's problematic | What to do instead |
|---|-------------|-------------------|----------------------|--------------------|
| A1 | **Verbatim SW proprietary assets** (exact icons, theme files, sound, branding) | Fastest path to pixel-perfect parity. | Copyright/trademark infringement; PROJECT.md explicitly bars it. Layout/behavior mimicry is legally fine; copying the actual art is not. | Recreate **look-alike** icons/themes from scratch (D8). Match style and layout, not bytes. |
| A2 | **SW file-format round-trip (.sldprt/.sldasm) as part of the UI milestone** | "A SW user will want to open their files." | Out of scope per PROJECT.md; it's a geometry/interop problem, not a UI problem, and a huge separate effort. Conflating it stalls the UI milestone. | Defer to a future interop milestone. UI parity is about *interaction*, not file compat. |
| A3 | **Replicating SW bugs/quirks for "faithfulness"** (e.g. specific dialog glitches, modal traps) | "Muscle memory includes the quirks." | Reproducing defects wastes effort and degrades the product; SW users don't have muscle memory for bugs, only for *intended* behavior. | Replicate intended behavior only; quietly fix the quirks. |
| A4 | **RMB-gesture that swallows the right-click context menu** | Naive mouse-gesture impl: any RMB-drag = gesture. | Breaks the (table-stakes) right-click context menu — a classic SW complaint when the threshold is wrong. Selection and context menu are higher priority than gestures. | Implement gestures with a **drag-distance/time threshold** so a click (or tiny move) still opens the context menu; make gestures opt-in. (Affects D2.) |
| A5 | **Fully modal PropertyManager that locks the whole UI** | Simpler to implement than SW's semi-modal panel. | SW's PropertyManager lets you still rotate/zoom and pick geometry while open; a hard-modal version would feel wrong and block selection-box picking. | FreeCAD's `TaskView`/`Control` is already non-modal-with-active-dialog — keep that; don't regress to modal dialogs. |
| A6 | **Per-document floating toolbars / SW's deep toolbar customization sprawl** | SW lets users float every toolbar everywhere. | Reproducing the full legacy floating-toolbar customization matrix is huge and low-value once the ribbon + S-key + gestures cover fast access. | Ship ribbon + S-key + gestures + heads-up bar. Treat exhaustive floating-toolbar customization as out of scope. |
| A7 | **Assembly/Drawing/Sheet-Metal-specific UI in v1** | "Full parity means everything." | These are large additional contexts (Assembly mate UI, Drawing sheets via TechDraw, Sheet Metal). Trying to ship all contexts at once prevents validating the core part-modeling experience. | Land the **part-modeling** SW experience first (where PartDesign/Sketcher already give us the engine), validate with SW users, then extend the same shells to assembly/drawing. |

---

## Feature Dependencies

```
T4 PropertyManager (left slide-in panel)
    ├──enables──> T8 In-tree double-click edit (routes edit into the PM)
    ├──enhanced by──> D5 Selection-reference boxes (the blue/pink pick lists)
    └──used by──> T10 Sketch→feature flow (feature creation opens the PM)

T2 FeatureManager tree
    └──hosts──> T3 Rollback bar (drag bar lives in the tree; needs Body Tip engine)

T6 Selection model
    ├──requires──> T7 Hover preselection (pick resolution shared)
    ├──enhanced by──> D6 Filter toolbar (F-keys drive the SelectionFilter engine)
    └──feeds──> D4 Context toolbar/breadcrumb (selection triggers the mini-toolbar)
    └──feeds──> D5 Selection-reference boxes (PM boxes consume the selection)

T1 CommandManager ribbon
    ├──shares backend with──> D1 S-key bar (both read the Command registry)
    ├──shares backend with──> D2 Mouse gestures (gesture→command mapping)
    └──drives──> T10 Sketch→feature flow (tab auto-switch on sketch exit)

T11 Heads-up toolbar  ──built on──> OverlayManager (existing canvas-overlay infra)
D3 Confirmation corner ──built on──> OverlayManager + active TaskDialog accept/reject
D4 Context toolbar     ──built on──> OverlayManager + Command registry

T5 Mouse navigation (SolidWorksNavigationStyle) ──independent, already exists──
    └──must coexist with──> D2 gestures (RMB) and T6 selection (LMB) without conflict
```

### Dependency Notes

- **T4 is the keystone.** The PropertyManager is the spine of feature creation/editing; T8 (in-tree edit) and T10 (sketch→feature) both terminate in it, and D5 (selection boxes) lives inside it. Build T4 early and well.
- **T3 (rollback bar) requires the PartDesign Body Tip engine**, which already exists (`Body.h`, `ViewProviderBody.cpp setTipIcon`). The dependency is satisfied at the App level; only the tree-widget drag UI is missing.
- **T1/D1/D2 share the Command registry backend.** Building the ribbon's command-grouping model makes the S-key bar and gesture mapping cheaper — design the command-grouping data structure once.
- **OverlayManager is a shared foundation** for T11 (heads-up), D3 (confirmation corner), and D4 (context toolbar). Establishing a clean canvas-overlay pattern early pays off three times.
- **A4 conflict:** D2 (RMB gestures) competes with the right-click context menu and must not break it — gesture detection needs a threshold; treat as a constraint on D2, not a separate feature.
- **T5 already exists** and is essentially free; it just needs to be the fork default and live-tested against SW for zoom-direction/speed feel.

---

## MVP Definition

> "MVP" here = the minimum SW-interaction set that lets a SolidWorks user model a part **without a tutorial** and feel at home. Modeling power already exists in FreeCAD; this is about the *shell*.

### Launch With (v1) — the "feels like SolidWorks for part modeling" core

- [ ] **T5 SolidWorks mouse navigation as default** — already built; flip the default + verify feel. Cheapest table-stakes win.
- [ ] **T1 CommandManager ribbon** — the single most visible "this is SolidWorks" signal; without it the app reads as FreeCAD instantly.
- [ ] **T2 FeatureManager tree (left, SW-styled, ordered)** — the second most visible signal; mostly re-dock + re-skin of existing tree.
- [ ] **T4 PropertyManager (left slide-in, OK/✓ pin, dynamic options)** — keystone of feature creation; FreeCAD TaskView is 70% of the way there.
- [ ] **T6 Selection model + T7 hover highlight** — must match SW click/box/filter feel; primitives exist.
- [ ] **T8 in-tree edit + T9 drag-reorder** — low cost given existing tree; high "it behaves like SW" payoff.
- [ ] **T10 sketch→feature flow conventions** — the actual modeling loop; wire tab-switch + auto-select.
- [ ] **T11 heads-up view toolbar + T12 standard views/cube** — constant on-canvas reference; cube already exists.

### Add After Validation (v1.x) — the power-user accelerators

- [ ] **T3 rollback bar** — high value but high effort; the Body Tip engine exists so it's deferrable, not blocked. Add once core loop is validated.
- [ ] **D5 selection-reference boxes (full fidelity)** — upgrade T4's panels to the blue/pink pick-list idiom.
- [ ] **D1 S-key shortcut bar** — top SW power-user accelerator; self-contained.
- [ ] **D3 confirmation corner** — on-canvas accept/cancel; build on OverlayManager once it's proven by T11.
- [ ] **D6 filter toolbar (F-keys)** — wire to existing SelectionFilter engine.
- [ ] **D8 visual theme** — ongoing, but the recognizable icon/color pass lands here to cross the "looks like SW" line.

### Future Consideration (v2+)

- [ ] **D2 mouse gestures (RMB donut)** — high muscle-memory value but the trickiest to get right (A4 context-menu conflict); defer until selection + context menu are rock-solid.
- [ ] **D4 context toolbar + breadcrumb** — net-new overlay + selection-chain reading; nice polish after the core shells exist.
- [ ] **D7 Instant3D on-canvas handles** — large effort, genuine differentiator; only after the rest feels like SW.
- [ ] **A7 contexts beyond part modeling** (Assembly mate UI, Drawing/TechDraw SW-styling, Sheet Metal) — extend the same shells to new contexts once the part experience is validated.

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| T5 SW mouse navigation (default) | HIGH | LOW (exists) | P1 |
| T1 CommandManager ribbon | HIGH | HIGH | P1 |
| T2 FeatureManager tree (left/SW) | HIGH | MEDIUM | P1 |
| T4 PropertyManager (left slide-in) | HIGH | MEDIUM–HIGH | P1 |
| T6 Selection model | HIGH | MEDIUM | P1 |
| T7 Hover preselection highlight | MEDIUM | LOW (exists) | P1 |
| T8 In-tree double-click edit | HIGH | LOW (given T4) | P1 |
| T9 Drag-reorder features | MEDIUM | LOW–MEDIUM | P1 |
| T10 Sketch→feature flow | HIGH | MEDIUM | P1 |
| T11 Heads-up view toolbar | HIGH | MEDIUM | P1 |
| T12 View cube / standard views | MEDIUM | LOW (cube exists) | P1 |
| T3 Rollback bar | HIGH | HIGH | P2 |
| D5 Selection-reference boxes | HIGH | MEDIUM | P2 |
| D1 S-key shortcut bar | MEDIUM–HIGH | MEDIUM | P2 |
| D3 Confirmation corner | MEDIUM | MEDIUM | P2 |
| D6 Filter toolbar (F-keys) | MEDIUM | LOW–MEDIUM | P2 |
| D8 Visual theme (look-alike) | HIGH | MEDIUM (broad) | P2 (ongoing) |
| D2 Mouse gestures (RMB donut) | MEDIUM–HIGH | MEDIUM–HIGH | P3 |
| D4 Context toolbar + breadcrumb | MEDIUM | MEDIUM–HIGH | P3 |
| D7 Instant3D handles | MEDIUM | HIGH | P3 |

**Priority key:** P1 = must have for launch · P2 = add after validation · P3 = future consideration

---

## Competitor Feature Analysis

How the same UX problem is solved across the reference points, and the approach for this fork.

| UX problem | SolidWorks (target) | FreeCAD (baseline) | Our approach (the fork) |
|-----------|---------------------|--------------------|-------------------------|
| Command access | CommandManager ribbon (tabs, large icons, flyouts) | Menus + flat toolbars + workbench combo | **Build the ribbon** over the existing Command registry (T1) |
| Feature history | FeatureManager tree + rollback bar | Document tree + PartDesign Body Tip (no drag bar) | Re-dock/re-skin tree (T2) + add drag rollback bar over the Tip engine (T3) |
| Feature creation UI | Left PropertyManager, slide-in, ✓ pin, selection boxes | TaskView/Control dialogs (right dock, non-modal, live preview) | **Re-dock left + re-style** the already-strong TaskView (T4) + reusable selection-box widget (D5) |
| Navigation | MMB rotate / Ctrl+MMB pan / wheel zoom | Pluggable nav styles, incl. existing `SolidWorksNavigationStyle` | **Make SW style the default** (T5) — already implemented |
| Selection | Click/box (enclose vs crossing) + filters + preselect | Unified selection, box select, filter grammar, preselection | Wire SW box semantics + filter toolbar/F-keys onto existing primitives (T6/D6) |
| Fast command invocation | S-key bar + RMB mouse gestures | Global shortcuts + command completer | Net-new S-key popup (D1) + thresholded RMB gesture donut (D2) |
| On-canvas view control | Heads-up floating toolbar + view cube | View commands in window chrome + existing NaviCube | Floating overlay toolbar via OverlayManager (T11) + existing cube (T12) |
| Accept/cancel a mode | Confirmation corner overlay (✓/✗), D-key | TaskDialog OK/Cancel buttons | Overlay corner reusing active TaskDialog accept/reject (D3) |
| Contextual next action | On-selection mini toolbar + breadcrumb | Right-click context menu | Add floating context toolbar + breadcrumb over selection chain (D4), keep context menu (don't break it per A4) |

---

## Sources

**SolidWorks UI feature set (HIGH confidence — official + multiple independent sources):**
- SOLIDWORKS Help — Mouse Gestures: https://help.solidworks.com/2021/english/SolidWorks/sldworks/c_mouse_sestures.htm
- TriMech — Anatomy of the SOLIDWORKS User Interface: https://store.trimech.com/blog/anatomy-of-the-solidworks-ui
- HawkRidge Systems — User Interface Basics in SOLIDWORKS: https://hawkridgesys.com/blog/user-interface-basics-in-solidworks
- Innova Systems — SOLIDWORKS 'S' Key Shortcut toolbar: https://www.innova-systems.co.uk/using-customising-s-key-shortcut/
- CAD Software Direct — S Keys and Mouse Gestures: https://blog.cadsoftwaredirect.com/improve-your-solidworks-workflow-with-s-keys-and-mouse-gestures/
- RickyJordan — CommandManager & PropertyManager: http://www.rickyjordan.com/2008/08/solidworks-2009-commandmanager-propertymanager.html
- Open WA Pressbooks — Major User Interface Components (Intro to SolidWorks): https://openwa.pressbooks.pub/testmhrtc/chapter/major-user-interface-components/

**FreeCAD current-state (HIGH confidence — verified directly against this checkout, commit 768e237091):**
- `src/Gui/Navigation/SolidWorksNavigationStyle.cpp` — existing SW navigation style (MMB rotate / Ctrl+MMB pan / wheel zoom)
- `src/Gui/Control.h`, `src/Gui/TaskView/TaskDialog.h`, `src/Mod/PartDesign/Gui/TaskFeatureParameters.cpp` — TaskView/PropertyManager analog
- `src/Gui/Tree.cpp`, `src/Gui/TreeView.cpp` — FeatureManager-analog tree (ordered, drag-DnD, F2 rename)
- `src/Mod/PartDesign/App/Body.h`, `src/Mod/PartDesign/Gui/ViewProviderBody.cpp` — Body Tip engine (rollback foundation)
- `src/Gui/Selection/SelectionFilter.cpp`, `BoxSelection.cpp`, `SoFCUnifiedSelection.cpp` — selection filters, box select, preselection highlight
- `src/Gui/OverlayManager.cpp`, `OverlayWidgets.cpp` — canvas-overlay infra for heads-up/confirmation/context toolbars
- `src/Gui/ToolBarManager.cpp`, `src/Gui/CommandManager.h` — command/toolbar backend the ribbon will sit on
- `src/Gui/MainWindow.cpp` (dock placement: tree/property default to right dock) — confirms re-dock-left gap
- `src/Gui/ViewProvider.h` (`doubleClicked()`, `setEdit()`) — in-tree edit entry point

---
*Feature research for: SolidWorks-faithful UI/UX layer on FreeCAD*
*Researched: 2026-06-06*

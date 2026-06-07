# SolidWorks UI Reference (Verified)

**Source:** Deep-research pass (5 angles → 23 sources → 91 claims → 25 adversarially verified, 24 confirmed / 1 refuted), 2026-06-06.
**Confidence:** HIGH. All quotes are from official SOLIDWORKS Help (help.solidworks.com), cross-version-consistent (SW2010–2026) and multiply corroborated. Note: help.solidworks.com returns HTTP 403 to automated fetch; quotes were verified via search-engine extraction + reseller corroboration (Javelin, Hawk Ridge, TriMech, CATI).

> Purpose: ground the SolidWorks-parity requirements in documented, official behavior. Use the **official terminology** below when implementing, and treat the **open questions** as plan-time research.

## Confirmed behaviors (per official Help)

### CommandManager (the "ribbon")
- Context-sensitive tabbed toolbar; clicking a tab below it swaps the shown toolbar (e.g. Sketches tab → Sketch toolbar). Embeds different toolbars by document type (part/assembly/drawing). Tabs are show/hideable; tool buttons customizable (right-click tab → Customize CommandManager).
- **Terminology:** SolidWorks officially calls it a **"toolbar,"** not a "ribbon." Users say "ribbon."
- Source: help.solidworks.com/2024/.../c_commandmanager.htm, c_toolbars.htm, t_displaying_or_hiding_tabs.htm

### PropertyManager
- Slide-in panel on the **left** of the graphics area (its own PropertyManager tab). Opens automatically on qualifying entity/command selection (other cases configurable in Tools > Options > System Options > General).
- Titlebar: **OK = green check** (accept selections, execute, close), **Cancel = red X** (ignore, close), plus **Detailed Preview**, **Help**, and **Keep Visible (pin)** buttons. Same green-check/red-X icons appear in the Confirmation Corner.
- **Selection boxes are PINK when active** (NOT blue). Selecting an item in a box highlights it in the graphics area; the box auto-expands as selections are added. Blue is used only for the **prompt icons** indicating what to pick — it is not a box-state color.
- Source: help.solidworks.com/2025/.../r_pm_overview.htm

### Heads-up View toolbar
- A **transparent** toolbar in **each viewport** with the common view tools (Zoom to Fit, Zoom to Area, Previous View, Section View, View Orientation flyout, Display Style, Hide/Show Items, Edit Appearance, Apply Scene, View Settings).
- Source: help.solidworks.com/2023/.../c_heads_up_view_toolbar.htm

### Context toolbars (on-selection mini-toolbars)
- Appear when you select items in the **graphics area OR the FeatureManager tree**; a subset of frequently-performed actions for that context. Visibility (left-click / right-click / both / neither) is customizable; button *contents* are fixed.
- Source: help.solidworks.com/2023/.../c_toolbars.htm, c_context_toolbars.htm

### Shortcut Bar (the "S" key) — a DISTINCT element
- SolidWorks documents three special productivity toolbars: Heads-up View tools, Context toolbars, and **Shortcut bars**. The Shortcut Bar (invoked by **S**) is its own official element with its own Help topic, separate from heads-up/context toolbars.
- Source: help.solidworks.com/2023/.../c_toolbars.htm; Javelin shortcut-bar blog

### Mouse navigation (defaults)
- **Rotate** = drag with MMB (no modifier). **Pan** = Ctrl + MMB drag (Ctrl **not** required in an active drawing). **Zoom** = scroll wheel. **Roll** = Alt+MMB. **Dolly** = Shift+MMB.
- **Zoom is zoom-to-cursor by default**: wheel zooms toward the pointer; if pointer is outside the graphics area, the model center zooms. (Screen-center zoom is opt-in: View > Modify > Zoom About Screen Center.)
- **Rotate about a clicked entity**: middle-click a vertex/edge/face (highlights magenta), then middle-drag to rotate about it. Disabled while editing a sketch.
- Source: help.solidworks.com/2024/.../r_Middle_Mouse_Button.htm

### Selection: box vs cross (official terminology)
- **Box selection** = drag **left → right**: selects only items **completely within** the box (enclose).
- **Cross selection** = drag **right → left**: selects items **crossing the box boundary IN ADDITION to** those fully enclosed.
- Works in parts, assemblies, drawings. Selection filters change which entity types are selectable.
- Source: help.solidworks.com/2026/.../c_Cross_Selection.htm, 2021/.../c_Box_Selection.htm

### Mouse gestures (corrected)
- Invoked by **right-dragging** in the graphics area; a radial mouse-gesture guide appears mapping drag directions to tools/macros. Right-drag through the highlighted tool icon and release past the tool region to invoke. Works in part/assembly/drawing/sketch.
- **Configurable to 2 / 3 / 4 / 8 / 12 directions; ENABLED by default at 4** (not the "4/8-way" we assumed; 8 is non-default). The 3- and 12-gesture options were added in SW2018.
- Source: help.solidworks.com/2024/.../c_mouse_sestures.htm, 2018/.../t_using_mouse_gestures.htm

## Corrections applied to requirements
1. **PROP-02**: active selection boxes are **pink** (not "blue/pink"); blue = prompt icons only.
2. **NAV-02**: use official terms **"box selection"** (L→R enclose) and **"cross selection"** (R→L crossing **+** enclosed).
3. **CANVAS-06**: gesture directions are **2/3/4/8/12, default 4**; the right-click-vs-gesture cancel mechanism is unverified (see open questions).
4. **NAV-01**: add roll (Alt+MMB) / dolly (Shift+MMB); Ctrl not needed for pan in drawings.

## Missing-from-list SolidWorks elements (added to requirements)
These are documented, commonly-used SW elements our original 21-item set omitted:
- **Task Pane** (right sidebar): Design Library, Appearances/Scenes/Decals (PhotoView), Custom Properties, SOLIDWORKS Resources. A persistent right-side fixture. → **PANE-01**
- **Magnifying Glass** (G key): inspect/select detail without changing zoom. → **CANVAS-08**
- **Reference Triad** (corner orientation triad). FreeCAD already shows an axis cross; folded into **CANVAS-02**.

## Open questions (plan-time, version-pinned verification needed)
These were NOT refuted — they simply weren't covered by a surviving verified claim and must be confirmed against version-pinned official Help during phase planning:
1. Exact ordered default button roster of the Heads-up View toolbar (SW2024/2025), and how the View Orientation flyout enumerates (standard views, view cube, Spacebar dialog).
2. Official FeatureManager interactions: rollback-bar drag (suppress-below vs insert-mid-history), F2 rename, double-click-to-edit, drag-to-reorder, FeatureManager flyout.
3. Precise gesture-vs-right-click distinction (drag threshold/distance; behavior on release inside vs outside the guide). ("Release-inside-cancels" was refuted 1-2.)
4. Confirmation Corner (top-right ✓/✗, D-key to move dialog to cursor), Selection Breadcrumbs (SW2018+), selection filters + F-key bindings, hover pre-selection highlight, Instant3D drag handles — and how newer graphics-area pushbuttons relate to/supersede the legacy confirmation corner.

## Primary sources
- CommandManager: https://help.solidworks.com/2024/english/SolidWorks/sldworks/c_commandmanager.htm
- Toolbars (heads-up / context / shortcut): https://help.solidworks.com/2023/english/SolidWorks/sldworks/c_toolbars.htm
- Heads-up View toolbar: https://help.solidworks.com/2023/english/SolidWorks/sldworks/c_heads_up_view_toolbar.htm
- PropertyManager overview: https://help.solidworks.com/2025/English/SWConnected/swdotworks/r_pm_overview.htm
- Middle mouse button (navigation): https://help.solidworks.com/2024/English/SolidWorks/sldworks/r_Middle_Mouse_Button.htm
- Mouse gestures: https://help.solidworks.com/2024/english/SolidWorks/sldworks/c_mouse_sestures.htm
- Box selection: https://help.solidworks.com/2021/English/SolidWorks/sldworks/c_Box_Selection.htm
- Cross selection: https://help.solidworks.com/2026/English/SolidWorks/sldworks/c_Cross_Selection.htm
- Context toolbars: https://help.solidworks.com/2024/English/SolidWorks/sldworks/c_context_toolbars.htm
- Selection breadcrumbs: https://help.solidworks.com/2021/english/SolidWorks/sldworks/c_selection_breadcrumbs.htm
- Instant3D context toolbar: https://help.solidworks.com/2020/english/SolidWorks/sldworks/c_video_instant3d_context_toolbar.htm
- Appearances Task Pane: https://help.solidworks.com/2025/english/solidworks/sldworks/t_controlling_appearance_task_pane.htm
- Magnifying Glass: https://help.solidworks.com/2025/english/SWConnected/swdotworks/t_Magnifying_Glass.htm
- Reference Triad: https://help.solidworks.com/2025/English/SolidWorks/sldworks/r_reference_triad.htm

---
*Verified UI reference for: SolidWorks-faithful FreeCAD UI · 2026-06-06*

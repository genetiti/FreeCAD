# Phase 4: PropertyManager Panel - Context

**Gathered:** 2026-06-14
**Status:** Ready for planning
**Decision mode:** User delegated all gray-area calls to Claude ("You choose the best options"), mirroring Phase 3. Decisions below are Claude's recommendations, grounded in the codebase scout, the SolidWorks-parity bar, and the LOCKED Phase 1/2/3 precedents (reuse-first, additive `FreeWorksGui` module, palette-only color, transaction-wrapped mutations, no "SolidWorks" identifier). They are LOCKED for planning unless the user revisits.

<domain>
## Phase Boundary

Deliver the **PropertyManager** — a left-docked, SolidWorks-styled panel that slides in when the user starts a feature/sketch command or edits a feature: a **green-✓ accept / red-✗ cancel header**, **collapsible rollout groups**, **pink active selection-reference boxes**, **live-updating fields + live model preview**, **SolidWorks keyboard/focus semantics**, **double-click-a-tree-feature-to-edit-here** (TREE-03's second half), and the **sketch→feature modeling loop** (FLOW-01). It is **non-modal** (geometry stays pickable). Built by **hosting** FreeCAD's existing `Gui::Control` / `TaskView` task-panel machinery in the left `Fw_PropertyManager` dock and restyling it — NOT by replacing it. Requirements: **PROP-01, PROP-02, TREE-03, FLOW-01**.

**Not this phase:** SW icon artwork / exact final colors / fonts (→ Phase 7 theming — only *functional* state colors like the active-box pink and the ✓/✗ accept-cancel semantics are in scope now); selection-model/canvas/box-select behavior beyond filling reference boxes (→ Phase 5); multi-body / assembly panels; any new task-panel backend (we reuse `TaskDialog`/`TaskView`).
</domain>

<decisions>
## Implementation Decisions

### A. Container / hosting strategy (the keystone decision)
- **D-01: Reuse-first — re-host FreeCAD's existing `Gui::Control` / `TaskView` into the left `Fw_PropertyManager` dock and restyle it SW-like.** Do NOT rebuild a task-panel system. `ControlSingleton::showDialog(TaskDialog*)` (Control.h:68) drops the active task dialog into `Control::taskPanel()` — a `Gui::TaskView::TaskView` (Control.h:76) — normally in the right "Tasks" dock. FreeWorks hosts/redirects that `TaskView` in the left `Fw_PropertyManager` dock (the placeholder already exists, `FwLayout.cpp:173` / `FwWorkbench.cpp:117`). This mirrors Phase 3 D-01 (wrap `Gui::TreeWidget`) and Phase 2 (drive the ribbon from the existing command registry). Because every PartDesign/Sketcher feature dialog shows through `Gui::Control`, hosting it left makes existing task panels **work unmodified** inside the new container (SC4) — and makes TREE-03's edit panels land left for free (see D-09).
- **D-02: Achieve the SW presentation via FreeWorks-side hosting/configuration, NOT by editing `src/Gui/Control.cpp` or `src/Gui/TaskView/TaskView.cpp` bodies.** Prefer, in order: (1) host/observe the existing `Control::taskPanel()` `TaskView` widget under the `Fw_PropertyManager` dock (re-parent / observe-the-DOM, mirroring the Phase 3 `mountFeatureManager` find-or-reuse swap), or (2) a thin `FwPropertyManager` container that adopts the running `TaskView`/`TaskDialog`. Any unavoidable shared-file touch gets `// SW-FORK HOOK`.
- **D-03 (PLAN-TIME SPIKE — mirrors the Phase 2 native-ribbon and Phase 3 tree-reuse spikes):** Before the full build, confirm `Gui::Control`'s `TaskView` can be (a) **re-hosted/shown in the LEFT `Fw_PropertyManager` dock** instead of the default right Tasks dock, (b) with the **accept/reject** controls and **collapsible `TaskBox`/`TaskGroup` rollouts** (TaskView.h:67/82/241) intact, (c) **without editing `Control.cpp`/`TaskView.cpp` bodies**. PASS → reuse-and-rehost is the committed engine. FAIL → a thin `FwPropertyManager` container that **adopts the running `TaskView`/`TaskDialog` widgets** (still additive, still reusing the `TaskDialog` machinery — NOT a from-scratch task system, which is the explicit last resort).

### B. Header (green-✓ / red-✗) + collapsible rollouts (PROP-01)
- **D-04: Reuse `TaskView`'s existing structure for the header and rollout groups; do not build a custom one.** `TaskView` already provides `TaskBox`/`TaskGroup` collapsible rollouts (QSint `ActionBox`/`ActionGroup`) and `accept()`/`reject()` (TaskView.h:241) — map **green-✓ → accept()** and **red-✗ → reject()**, positioned in the panel header. Restyle SW-like via palette only (no hex, no `setStyleSheet` color literals — Phase 7 owns final art). The ✓/✗ **semantics + header placement** are in scope this phase; the **glyph art / exact hue** is Phase 7.

### C. Pink selection-reference box (PROP-02)
- **D-05: Build a thin FreeWorks selection-reference affordance that drives/reads the EXISTING `Gui::Selection`, NOT a parallel selection backend.** SW behavior: click into a reference box to **activate** it, pick geometry in the 3D view → the box **fills + the picked element highlights**, the box **auto-expands**, and **clear/remove** work. Implement as a FreeWorks widget wired to `Gui::Selection` (SelectionObserver) and the existing task-panel reference-selection mechanism — never a new selection model (Phase 5 owns selection behavior; this phase only consumes it to fill boxes).
- **D-06: "Pink-when-active" is a FUNCTIONAL state signal (in scope now), distinct from Phase 7 decorative theming.** PROP-02 (and official Help) lock it: **pink = active reference box; blue is reserved for prompt icons.** This is a functional affordance (like Phase 3's `QPalette::Highlight` rollback band), so a **palette-derived active tone** is allowed this phase; the exact final hue/art is Phase 7. Reconciles the "palette-only, no hex" rule — use a derived/role color and note Phase 7 may refine it.

### D. Slide-in + non-modal behavior (PROP-01)
- **D-07: Non-modal / semi-modal — geometry stays pickable while the panel is open and the model previews live.** LOCKED by REQUIREMENTS (a fully-modal PropertyManager that locks the UI is explicitly rejected — it would break selection-box picking). The panel is **event-driven on command/edit start**, reusing the same `Gui::Control` / `signalInEdit` plumbing the Phase-2 `FwRibbonContext` already listens to.
- **D-08: The "slide-in" is a light affordance, not a heavyweight animation (MVP framing).** The must-have is the panel **appearing left-docked when a command/edit starts**; the literal slide is a refinement. Implement a short width/visibility animation (e.g. `QPropertyAnimation`) ONLY if it is low-risk and cross-platform-clean; otherwise an instant reveal is acceptable for the MVP and the slide-FEEL routes to the live checklist. Animation polish must never block the functional panel.

### E. TREE-03 — double-click a tree feature to edit in the PropertyManager
- **D-09: Route the EXISTING `ViewProvider::setEdit()` edit path so its task dialog lands in the LEFT `Fw_PropertyManager` dock — no separate mechanism.** Double-clicking a feature in the `FwFeatureTree` triggers the standard edit entry (the inherited `TreeWidget` double-click → `ViewProviderDocumentObject::doubleClicked` → `setEdit`, ViewProvider.h:646/656). Because that dialog shows via `Gui::Control` (D-01), the left-hosting **automatically** lands feature-edit panels in the PropertyManager — TREE-03's "routes through the existing `setEdit()` path into the left panel" is satisfied by the D-01 hosting itself. Only work: ensure the `FwFeatureTree` double-click reaches `setEdit` (reuse inherited behavior) and that `Control`'s panel is the left-hosted one. (TREE-03's F2-rename half already shipped in 03-02.)

### F. SolidWorks keyboard / focus semantics + transactions (SC4)
- **D-10: Apply SW key semantics at the container level — Enter = accept (✓), Escape = cancel (✗), Tab = advance to the next field / reference box — overriding FreeCAD task-panel defaults where they differ.** Implement as a FreeWorks key handler on the hosted container; do not edit the shared TaskView. Every edit stays transaction-wrapped by reusing the existing `TaskDialog` `openTransaction`/`commitTransaction` discipline (the dialogs already wrap) — clean undo/redo, no `.FCStd` pollution. The precise key map mirrors SW; the live FEEL is a checklist item.

### G. FLOW-01 — sketch → feature modeling loop
- **D-11: On sketch exit, auto-select the just-finished sketch as the profile and switch the ribbon to the Features tab — but DO NOT auto-start a feature command; the user clicks Extrude/Revolve with the profile pre-selected (true SW behavior).** SW exits the sketch, leaves it selected, and lets the user choose the feature; auto-launching a specific feature would guess intent. Reuse the existing `signalResetEdit` (the same signal the Phase-2 `FwRibbonContext` already consumes to restore tabs) to (a) set the finished sketch as the current `Gui::Selection` so a subsequent Extrude/Revolve picks it up as the profile, and (b) let the ribbon context land on the Features tab. No new feature-launch logic.
- **D-12: Plane/face → Sketch entry reuses FreeCAD's existing sketch-creation flow; FreeWorks does not reimplement sketch creation.** It only ensures the entry *feels* SW (plane/face pick → Sketch command → the sketcher edit panel opens in the LEFT PropertyManager via D-01).

### Claude's Discretion
- Exact container seam — re-host the live `Control::taskPanel()` `TaskView` vs. a thin `FwPropertyManager` that adopts it — resolved by the **D-03 spike** during planning/research.
- Slide animation vs. instant reveal (D-08) — animate only if low-risk and cross-platform-clean.
- Precise active-box pink tone and ✓/✗ glyphs — functional/palette-derived now, final art Phase 7.
- The exact key-event override mechanism for D-10 (event filter vs. container `keyPressEvent`).
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents (researcher, planner) MUST read these before planning or implementing.**

### Phase scope & requirements
- `.planning/ROADMAP.md` § "Phase 4: PropertyManager Panel" — goal + 5 success criteria (note **Mode: mvp**; SC1 non-modal slide-in panel, SC2 pink active selection boxes, SC3 double-click→edit, SC4 SW keys + existing PartDesign panels unmodified, SC5 sketch→feature flow)
- `.planning/REQUIREMENTS.md` — **PROP-01** (left slide-in panel: ✓/✗ header, collapsible rollouts, live fields, non-modal, live preview), **PROP-02** (pink active selection-reference boxes; blue reserved for prompt icons), **TREE-03** (double-click→edit + F2; F2 half shipped 03-02), **FLOW-01** (sketch→feature flow). Also REQUIREMENTS § Out-of-Scope: "Fully modal PropertyManager that locks the UI" is REJECTED (non-modal is locked).

### PropertyManager container — the keystone seam (host, don't replace)
- `src/Gui/Control.h:54,68,76,78,103,106` — `ControlSingleton` / `showDialog(TaskDialog*)` / `taskPanel()` → `TaskView` / `showModelView` / `showTaskView` (the existing task-panel host to re-dock left)
- `src/Gui/Control.cpp` — the `Control` dock/show logic (learn from; do NOT edit bodies — D-02)
- `src/Gui/TaskView/TaskView.h:67,82,163,241` — `TaskGroup` / `TaskBox` (collapsible rollouts, QSint ActionBox/ActionGroup) / `TaskView` (the QStackedWidget host) / `accept()`/`reject()` (the ✓/✗ semantics for D-04)
- `src/Gui/TaskView/TaskView.cpp` — TaskView impl (learn from; do NOT edit bodies)
- `src/Gui/TaskView/TaskDialog.h` — `TaskDialog` (the per-command panel contract; already transaction-aware — D-10)

### Edit entry (TREE-03) & command-start signals (PROP-01 / FLOW-01)
- `src/Gui/ViewProvider.h:646,656,669,671` — `EditMode` enum / `setEdit(int)` / `setEditViewer`/`unsetEditViewer` (the edit path TREE-03 routes through)
- `src/Gui/ViewProviderDocumentObject.h` — `doubleClicked()` → `setEdit` (the tree double-click entry)
- `src/Gui/Application.h` — `signalInEdit`/`signalResetEdit` (the event-driven hooks the Phase-2 `FwRibbonContext` already uses; reuse for panel show + sketch→feature handoff)

### Phase 1/2/3 fork seams to mirror
- `src/Gui/FreeWorks/FwLayout.cpp:173` + `src/Gui/FreeWorks/FwWorkbench.cpp:117` — the `Fw_PropertyManager` (left) dock placeholder = the mount target (mirror Phase 3 `mountFeatureManager` find-or-reuse swap)
- `src/Gui/FreeWorks/FwRibbonContext.{h,cpp}` (Phase 2) — the `signalInEdit`/`signalResetEdit` decide-then-act precedent + tab switching (reuse for PROP-01 show + FLOW-01 Features-tab handoff)
- `src/Gui/FreeWorks/FwFeatureTree.{h,cpp}` (Phase 3) — the tree whose double-click drives TREE-03; the mount-swap + leak-grep + headless-test discipline to mirror
- `.planning/phases/02-commandmanager-ribbon/02-04-SUMMARY.md` — `FwRibbonContext` event-driven sketch-tab switching (the FLOW-01 ribbon half)
- `.planning/phases/03-featuremanager-design-tree/03-02-SUMMARY.md` + `03-03-SUMMARY.md` — F2 (TREE-03 first half), `FwSelectionGuard` (the Gui::Selection snapshot/restore pattern reusable for D-05/D-11), dock mount-swap pattern

### Architecture & conventions (anti-patterns to honor)
- `CLAUDE.md` + `.planning/codebase/ARCHITECTURE.md` — App/Gui separation; do NOT modify the document during recompute; mutate via Property setters + transactions; non-modal panel must not break selection picking
- `.planning/codebase/CONVENTIONS.md`, `STRUCTURE.md`, `TESTING.md` — C++20/Qt6.8 naming, FreeWorks lives in `src/Gui/FreeWorks/` (`FreeWorksGui` namespace), headless GTest/QTEST + ctest patterns (mirror Phase 1/2/3); leak-grep (`tools/fw-string-leak-grep.sh`) clean, palette-only color, additive `target_sources`
- `src/Gui/FreeWorks/SPIKE_LIVE_CHECKLIST.md` — where the live-only FEEL items (slide animation, pink-box pick feel, SW key semantics, sketch→feature handoff feel, SC-style daily-user acceptance) get recorded as open obligations (no build tree in env)
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets (reuse, don't rebuild — Phase 2/3 precedent)
- **`Gui::Control` + `Gui::TaskView::TaskView`** (`src/Gui/Control.*`, `src/Gui/TaskView/*`): the entire task-panel system — `showDialog`/`taskPanel()`, collapsible `TaskBox`/`TaskGroup` rollouts, `accept()`/`reject()`. Re-host left per D-01/D-04. Existing PartDesign/Sketcher `TaskDialog`s then run unmodified inside it (SC4).
- **`ViewProvider::setEdit()` + `doubleClicked()`**: the ready-made edit-entry path TREE-03 routes through (D-09) — no new edit mechanism.
- **`Gui::Application::signalInEdit`/`signalResetEdit`**: the event-driven command-start/exit hooks the Phase-2 `FwRibbonContext` already consumes — reuse for PROP-01 panel show and FLOW-01 sketch→feature handoff.
- **`Gui::Selection` + `FwSelectionGuard` (Phase 3)**: the selection API the pink reference boxes (D-05) read/drive and the FLOW-01 auto-profile-select (D-11) uses; the Phase-3 snapshot/restore guard is a reusable pattern.

### Established Patterns (constrain this phase)
- Observe-the-DOM: the panel reflects the live `App::Document`/edit state via `Gui::Control` + selection signals; never caches a divergent model.
- All App-state mutations go through `TaskDialog`'s `openTransaction`/`commitTransaction` — clean undo/redo, no `.FCStd` pollution (D-10).
- FreeWorks code is additive (`target_sources(FreeCADGui PRIVATE ...)`), namespace `FreeWorksGui`, dir `src/Gui/FreeWorks/`, no "SolidWorks" identifier, `// SW-FORK HOOK` for any unavoidable shared touch, palette-only color, leak-grep clean.

### Integration Points
- Mount: replace/host the `Fw_PropertyManager` placeholder in `FwLayout.cpp:173` with the left-hosted `Control` `TaskView` (find-or-reuse swap, mirror Phase 3).
- Command-start / edit / sketch-exit: through `Gui::Control` + `signalInEdit`/`signalResetEdit` (reuse Phase-2 plumbing).
- Reference-box fill + auto-profile-select: through `Gui::Selection` (read/drive only — Phase 5 owns selection behavior).
</code_context>

<specifics>
## Specific Ideas
- The panel must *feel* like SolidWorks (Mode: mvp; the SC-style daily-SW-user acceptance is the bar): left slide-in, pink active reference box that fills on pick and auto-expands, ✓/✗ header, non-modal so geometry stays pickable, live preview.
- Pink = active reference box, blue = prompt icons (official Help) — a functional state distinction to honor now, not deferred to theming.
- The Phase-4 container spike (D-03) is the analog of Phase 2's native-vs-SARibbon and Phase 3's tree-reuse spikes: prove `Control`/`TaskView` re-hosts left (with rollouts + accept/reject) before committing the full build; document the verdict in a `SPIKE.md`.
- Reuse the existing PartDesign/Sketcher task panels verbatim inside the container (SC4) — the win is presentation + placement + the pink box + SW keys, not a new panel backend.
</specifics>

<deferred>
## Deferred Ideas
- **SW icon artwork, exact final colors, fonts:** Phase 7 (Visual Theme). Only functional state colors (active-box pink, ✓/✗ semantics) are in scope now.
- **Selection-model / box-select / canvas accelerators / context toolbars:** Phase 5 (Selection Parity). This phase only consumes selection to fill reference boxes.
- **Multi-body / assembly PropertyManager semantics:** out of scope (single active Body/edit this milestone).
- **Custom per-command PropertyManager layouts beyond what the existing TaskDialogs provide:** out of scope — reuse existing panels; bespoke SW-exact layouts are a future refinement unless a later phase requires them.

### None — discussion stayed within phase scope
</deferred>

---

*Phase: 04-propertymanager-panel*
*Context gathered: 2026-06-14 (user-delegated decisions, Claude-recommended)*

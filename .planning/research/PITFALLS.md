# Pitfalls Research

**Domain:** FreeCAD C++/Qt GUI fork reimplementing a SolidWorks-faithful interface, tracking upstream FreeCAD `main`
**Researched:** 2026-06-06
**Confidence:** HIGH on architecture/upstream (grounded in the codebase maps), MEDIUM on legal (verified against case law but not legal advice), MEDIUM-HIGH on Qt/cross-platform.

> Scope note: This milestone is GUI-layer-only (`src/Gui/*`, `src/Mod/*/Gui/*`). The App/Base layers stay intact. The dangerous mistakes cluster in four areas: **legal asset-cloning**, **upstream merge hell**, **breaking FreeCAD's App/Gui contracts**, and **superficial-but-wrong UX parity**. Each pitfall below names warning signs, prevention, and the phase that owns it.

---

## Critical Pitfalls

### Pitfall 1: Copying SolidWorks proprietary assets (icons, themes, fonts, branding) verbatim

**What goes wrong:**
The team extracts icons/cursors/theme resources from an installed SolidWorks, or traces them pixel-for-pixel, to hit "visual parity" fast. Icons, splash art, and packaged theme files are copyrightable creative works; the "SolidWorks" name and logo are trademarks. Shipping them (even re-saved as PNG) is direct copyright infringement plus trademark/false-affiliation exposure. The PROJECT already flags this as out-of-scope, but the temptation re-appears every time a recreated icon "looks slightly off."

**Why it happens:**
Recreating ~1000+ command icons is genuinely expensive, so a developer "temporarily" drops in the real assets to unblock UI work, intending to replace them later. They never get replaced, or they leak into a commit and into the git history permanently.

**How to avoid:**
- Treat the SolidWorks install as **reference-only, never source**. No SW binary asset enters the repo or build, ever — enforce with a pre-commit hook scanning `Resources/icons` for known hashes / SW metadata, and a CI check.
- Recreate icons as original artwork that conveys the *same function* (a wrench = settings) without copying the specific drawing. Functional iconography (Apple v. Samsung: icons are "functional" because they communicate the action) is the safe zone; the specific artistic rendering is not.
- Keep an `ASSET_PROVENANCE.md` mapping every shipped icon to its creation source (original SVG, commissioned, or permissibly-licensed set like a custom open icon family).
- Never ship the "SolidWorks" wordmark, logo, or splash. Name the product distinctly; do not imply affiliation/endorsement.

**Warning signs:**
- A commit adds binary images with no corresponding source SVG.
- Icons in the repo are 1:1 pixel-identical to SW under diff.
- The word "SolidWorks" appears in any UI string, window title, About box, or installer.
- Designers reference "just grab it from the SW folder."

**Phase to address:**
Visual-theme / icon-set phase (the phase that builds the recreated look-alike asset library). Set the provenance discipline and CI guard at the *start* of that phase, before any icon lands.

---

### Pitfall 2: Mistaking "layout/behavior mimicry is fine" for "everything is fine" — trade-dress overreach

**What goes wrong:**
The team assumes that because look-and-feel/layout cloning is broadly defensible (functional UI elements are generally not protectable; *Google v. Oracle* preserved reimplementation under fair use; *Apple v. Samsung* found UI layout features functional), **nothing** about imitation carries risk. They then go past functional parity into copying distinctive, non-functional brand signatures — exact proprietary color palettes presented *as* SolidWorks, marketing that says "it's SolidWorks," or replicating a distinctive ornamental flourish that has acquired secondary meaning.

**Why it happens:**
"Layout and behavior mimicry is not a legal issue" (true, and in PROJECT.md) gets over-generalized into "imitation has no limits." Trade dress can protect *non-functional, distinctive* presentation; the defense is functionality, not "UIs can't be protected."

**How to avoid:**
- Keep imitation in the **functional** lane: ribbon tabs, tree-with-rollback, sliding property panel, mouse/selection conventions — these are interaction mechanics (functional, safe).
- Do **not** market the product as SolidWorks or as a SolidWorks product; describe it as "familiar to SolidWorks users." Avoid comparative claims that imply endorsement.
- Choose your own distinct default color scheme/accent that *evokes* familiarity without being a pixel-match of SW's branded palette. Parity of *structure* is the goal, not parity of *brand identity*.
- If real money/distribution is involved, get a one-time IP counsel review of the visual theme and marketing copy before public release. This research is not legal advice.

**Warning signs:**
- Marketing copy or README implies the product *is* SolidWorks or is endorsed by Dassault.
- Default theme is described internally as "the SolidWorks colors."
- Anyone says "trade dress doesn't apply to software, so we can copy whatever."

**Phase to address:**
Visual-theme phase for the palette/branding decisions; a release-readiness/legal-review checkpoint phase before first public distribution.

---

### Pitfall 3: Deep edits to `Gui::MainWindow` and shared `src/Gui/*` files create permanent merge hell

**What goes wrong:**
To install the ribbon, FeatureManager, and PropertyManager, the team rewrites `Gui::MainWindow`, `Tree.cpp`, `PropertyEditor.cpp`, `View3DInventorViewer.cpp`, etc. in place. Upstream `main` (a moving target at `1.2.0-dev`) keeps changing those exact files — `Tree.cpp` is ~6,900 lines and `View3DInventorViewer.cpp` ~4,800 lines and both are actively churned. Every upstream sync becomes a multi-day conflict resolution where the team must re-understand upstream's intent inside files they've heavily rewritten. Eventually the fork freezes on an old base because merging is too painful, defeating the "fork from newest internals" decision.

**Why it happens:**
In-place editing is the path of least resistance — it's faster to change a method than to design an extension seam. The cost is invisible until the third or fourth upstream merge.

**How to avoid:**
- **Additive over invasive.** Prefer *new* classes/files (`SwMainWindow` subclass, `SwRibbonBar`, `SwFeatureTree`, `SwPropertyManager`) that compose or subclass upstream, over rewriting upstream bodies. New files never conflict.
- Concentrate unavoidable upstream touch-points into the **smallest possible, well-commented hook sites** — ideally one insertion point per shared file (a factory hook, a virtual, a registration call), tagged with a sentinel comment like `// SW-FORK HOOK` so they are greppable and re-applicable.
- Maintain SW changes as a **rebasable patch series / topic branches on top of an untouched mirror of upstream `main`**, not as commits intermingled into a diverged `main`. Never push SW work to the branch that mirrors upstream — keep that branch byte-identical so new work never starts mid-conflict.
- **Sync small and often** (weekly/biweekly), resolving little conflicts continuously, rather than a quarterly mega-merge. Use `git range-diff` to spot commits upstream has absorbed and `--skip` them.
- Watch upstream for changes to your hook files; ideally upstream the *generic* hook points (extension seams) so FreeCAD itself carries them and you only carry the SW-specific code.
- Pin and document the exact upstream commit you track; treat upstream bumps as deliberate, tested events, not ambient drift.

**Warning signs:**
- A single upstream sync produces conflicts in more than a handful of files, or takes more than a day.
- The fork's `main` has diverged such that `git merge upstream/main` touches your code in dozens of places.
- Developers avoid syncing upstream "because last time was awful" — the fork's base date keeps slipping.
- Hook edits are scattered with no consistent marker, so no one knows where the fork touches upstream.

**Phase to address:**
The foundational "fork scaffolding / extension-seam" phase must come **first**, before any SW UI is built — it establishes the additive architecture, the hook-comment convention, the mirror-branch + patch-series workflow, and a scripted/CI upstream-sync drill. Every later UI phase inherits that discipline.

---

### Pitfall 4: Breaking the App/Gui separation by pushing UI state or logic into the App layer

**What goes wrong:**
A SolidWorks behavior (e.g., the FeatureManager rollback bar, PropertyManager contextual state, selection-filter state) is implemented by adding fields/properties/logic to `App::Document` / `App::DocumentObject`, or by having App code reach into Gui. This corrupts the clean DOM, breaks headless/console operation (`MainCmd.cpp`), pollutes the `.FCStd` file format with GUI-only data, and creates a second, conflict-prone surface against upstream's App layer (which this milestone is supposed to leave untouched).

**Why it happens:**
The SW feature feels like it "belongs to the model" (rollback looks like a model concept), so the developer reaches for the nearest object — which is often an App object — instead of holding the state in a Gui-side controller.

**How to avoid:**
- Keep **all** SW UI/interaction state in the Gui layer (`Gui::Document`, ViewProviders, dedicated Gui controllers). The App layer stays pristine; this is the project's stated constraint.
- Rollback = a *Gui-side visibility/suppression view* over the existing dependency order, not a new App property. Reuse existing suppression/visibility mechanisms rather than inventing App-level state.
- If you genuinely need persisted per-object UI state, store it in Gui-side ViewProvider properties, not App DocumentObject properties — and verify it does not break console-mode load.
- CI gate: run the existing **console/headless** test path; if a Gui-only change breaks `--console` document load/recompute, you've crossed the boundary.

**Warning signs:**
- New properties appear on `App::DocumentObject` subclasses to support a UI panel.
- `#include` of Gui headers appears under `src/App/` or `src/Mod/*/App/`.
- A document saved in the SW fork won't open cleanly in upstream FreeCAD (format polluted).
- Headless tests fail after a "pure UI" change.

**Phase to address:**
FeatureManager/rollback phase and PropertyManager phase — both must explicitly document "state lives in Gui." The scaffolding phase should add the headless-load CI gate that detects boundary violations.

---

### Pitfall 5: Bypassing Property change notification / mutating the document during recompute

**What goes wrong:**
New SW-style commands or ViewProvider code set object state by writing member variables directly (e.g., `obj->Placement = p;`) instead of via Property setters, or they create/delete/modify objects inside `execute()` during a recompute. Per the codebase ARCHITECTURE anti-patterns, this silently breaks undo/redo, dependency tracking, GUI sync, and recompute ordering — and the bugs surface far from the cause (corrupted undo stack, stale tree, objects that won't recompute). This is *especially* easy to introduce when reimplementing the tree and property panel, which are exactly the components that read/write object state.

**Why it happens:**
Direct member assignment compiles and "works" in the happy path; the consequences (broken notifications, corrupt undo) only appear under interaction sequences. Recompute-time mutation feels convenient when a SW command wants to spawn helper objects.

**How to avoid:**
- Always mutate model state through `Property::setValue()` / property setters; never assign object members directly. If internal state must change, call `touch()`.
- Wrap every user-facing edit in `Document::openTransaction()/closeTransaction()` so SW-style undo/redo and the rollback bar behave correctly.
- Never add/remove/modify document objects inside `execute()`; do object creation in the command layer, not during recompute.
- ViewProviders must connect to property-change signals in `attach()` and disconnect in `detach()` — a reimplemented FeatureManager that doesn't will show stale state.

**Warning signs:**
- Undo leaves the document in an inconsistent state, or undo "skips" a change.
- The new tree/property panel shows stale values until a manual refresh.
- Objects fail to recompute or recompute in the wrong order after a SW command.
- Code review finds direct `obj->SomeMember = ...` assignments or `addObject`/`removeObject` calls inside `execute()`.

**Phase to address:**
Every phase that adds commands or touches model state, but most critically the PropertyManager phase (writes properties) and FeatureManager phase (reads/edits/reorders objects, drives transactions/undo).

---

### Pitfall 6: Coin3D `SoNode` ref/unref leaks in custom ViewProviders / viewport widgets

**What goes wrong:**
SW-style selection highlighting, context toolbars, preview glyphs, and navigation overlays add custom Coin3D scene-graph nodes. Coin3D nodes are reference-counted; forgetting `ref()`/`unref()` (or unref'ing too early) leaks memory or causes use-after-free. The codebase already lists memory leaks and OpenGL-state fragility in the Sketcher ViewProvider and `View3DInventorViewer` — the SW work adds *more* custom scene-graph code in exactly this fragile area, compounding the risk over long modeling sessions.

**Why it happens:**
Coin3D's manual refcounting is easy to get subtly wrong, leaks are invisible in short test runs, and the existing viewer code is already complex and full of FIXMEs, so new code inherits unclear ownership conventions.

**How to avoid:**
- Use Coin3D smart pointers / RAII wrappers (`SoRef`-style) for all owned nodes instead of manual `ref`/`unref`.
- Follow the established ViewProvider attach/detach lifecycle: build the subgraph in `attach()`, tear it down in `detach()`; never leave dangling node references.
- Run heavy interaction sessions (create/rotate/select hundreds of times) under a leak detector (ASan/Valgrind) as a periodic check, mirroring how upstream chased the Sketcher leaks.
- Keep SW scene-graph additions in their own nodes/separators so they're easy to audit and don't entangle upstream's OpenGL state blocks.

**Warning signs:**
- RSS memory climbs steadily during a long modeling/selection session (the documented "restart between heavy sessions" smell).
- Crashes on object deletion or workbench switch (premature unref / dangling node).
- ASan/Valgrind reports leaks rooted in `So*` allocations from SW ViewProvider code.

**Phase to address:**
The 3D-viewport interaction phase (SW selection model, highlighting, context toolbars, navigation overlays). Add a leak-check pass to that phase's done-criteria.

---

### Pitfall 7: Superficial UX parity that is "right at a glance, wrong in the feel" (selection model, PropertyManager modality, rollback)

**What goes wrong:**
The ribbon, tree, and side panel *look* like SolidWorks, so the team declares parity — but the **interaction feel** is FreeCAD's, which immediately breaks the "zero relearning" promise that is this project's entire Core Value:
- **Selection model:** SW has specific click/box-select directionality, pre-highlight, selection-filter, and what-gets-selected-on-edge-vs-face semantics. FreeCAD's defaults differ. A SW user's muscle memory mis-selects constantly.
- **PropertyManager modality:** SW's PropertyManager is a modal-ish in-context flow (confirm/cancel, green check, escape behavior, focus capture, tab order through fields). Mapping it onto FreeCAD's Task-panel model superficially leaves wrong focus/Enter/Escape/abort semantics.
- **Rollback:** SW rollback suppresses features *below the bar* and lets you insert at that point. A naive implementation that merely hides or that recomputes incorrectly at the bar position feels wrong and can corrupt edit-in-context flows.

**Why it happens:**
These behaviors are easy to approximate visually and hard to match precisely; the gaps are felt, not seen, so they pass visual QA but fail real SW users. The team that builds it isn't a population of daily SolidWorks users, so the subtle deltas are invisible to them.

**How to avoid:**
- Write **behavioral specs** for each interaction (selection directionality, pre-highlight, filter precedence; PropertyManager Enter/Escape/Tab/focus/confirm-cancel; rollback suppress-below + insert-at-bar) *before* implementing, derived from observed SW behavior — not from FreeCAD defaults.
- Recruit **actual daily SolidWorks users** for parity testing early and repeatedly; "no relearning" can only be validated by them, not by the implementers.
- Treat these three (selection, PropertyManager modality, rollback) as **first-class behavioral features with acceptance tests**, not as styling.
- Reuse FreeCAD's transaction/suppression/Task-panel machinery underneath, but bend the *interaction semantics* to SW on top.

**Warning signs:**
- QA sign-off is based on screenshots/looks, not on SW-user task runs.
- SW testers say "it looks right but feels off" or repeatedly mis-select / hit the wrong key.
- Rollback produces recompute errors or leaves features in an unexpected state.
- Enter/Escape/Tab in the property panel don't do what SW users expect.

**Phase to address:**
Each owns its parity spec + acceptance tests: selection-model phase, PropertyManager phase, FeatureManager/rollback phase. A recurring SW-user usability-testing track should run across all of them.

---

### Pitfall 8: Mouse-navigation / shortcut conventions diverging across Windows, macOS, and Linux

**What goes wrong:**
SW navigation (MMB-rotate, scroll-zoom, modifier-based pan, RMB context) is reproduced and tested only on Windows (the SW-user platform), then ships broken on macOS/Linux: macOS trackpads have no middle button and use gesture/Cmd conventions, modifier keys differ (Cmd vs Ctrl), zoom direction and wheel semantics differ, and right-click/Ctrl-click collide with platform conventions. The result violates the cross-platform requirement and the consistency goal.

**Why it happens:**
The audience is Windows-heavy, so dev/test gravitates to Windows; the per-platform input deltas are easy to forget until a Mac/Linux user reports unusable navigation.

**How to avoid:**
- Build navigation on a **single, explicit input-mapping abstraction** with per-platform bindings, not scattered `if`-checks; define the SW navigation profile once and adapt device/modifier specifics per OS.
- Define explicit macOS substitutes up front (trackpad gestures, Cmd-modifier, two-finger conventions) and a middle-button-less path.
- Make zoom direction, pan modifier, and rotate trigger configurable, defaulting to SW-on-Windows but with platform-correct defaults elsewhere.
- **Test navigation on all three platforms every milestone**, not at the end. CI can't fully test interaction, so schedule manual per-platform navigation checks.

**Warning signs:**
- Navigation code is full of bare Windows assumptions (middle button always present, Ctrl always the modifier).
- No macOS/Linux navigation testing in the milestone's QA.
- Mac users report they can't rotate/pan; zoom feels inverted somewhere.

**Phase to address:**
The mouse-navigation/selection phase — bake the input-abstraction + per-platform profile in from the start, and add tri-platform manual nav testing to that phase's done-criteria.

---

### Pitfall 9: QSS theming that breaks per-platform, and HiDPI/icon scaling failures

**What goes wrong:**
The SW look is chased with heavy global Qt Style Sheets (QSS). QSS swaps the native style for a wrapper style, causing sizing/metrics inconsistencies — notably on macOS, where a simple `background-color` rule can disrupt native widget appearance and metrics, and where scrollbar/edge artifacts appear. Separately, recreated icons supplied only as fixed-size raster PNGs look blurry/wrong on HiDPI (Retina, 150%/200% Windows scaling); the ribbon and tree — dense with icons — are where this is most visible.

**Why it happens:**
QSS is the fast way to restyle; its cross-platform metric side-effects and HiDPI raster pitfalls aren't obvious until tested on a Mac/Retina/scaled display. The team styles on one machine and assumes it travels.

**How to avoid:**
- Prefer **SVG/vector icons** with proper `@2x`/high-DPI handling and Qt's high-DPI pixmap support; avoid shipping single-resolution PNGs for ribbon/tree glyphs.
- Use QSS **surgically and test it on macOS specifically**; where QSS fights native metrics, prefer a custom `QStyle`/`QProxyStyle` or targeted painting over blanket stylesheets. (KDAB's well-known guidance: be wary of QSS for anything beyond light cosmetic tweaks.)
- Validate the full UI at 100% / 150% / 200% scaling and on a Retina Mac before calling a theme done; check ribbon icon crispness, panel metrics, and scrollbar edges.
- Keep theming centralized so per-platform fixes live in one place rather than being patched widget-by-widget.

**Warning signs:**
- Widgets look correct on Windows but cramped/oversized/misaligned on macOS.
- Icons are crisp on the dev machine but blurry on a Retina/scaled display.
- Scrollbar or border artifacts appear only with the stylesheet active.
- Theme tweaks require per-widget QSS patches that multiply over time.

**Phase to address:**
The visual-theme phase (QSS strategy + icon vector pipeline + HiDPI policy). Add per-platform + per-scale visual QA to its done-criteria.

---

### Pitfall 10: Ribbon and docking implemented in ways that fight Qt and upstream

**What goes wrong:**
A SW CommandManager ribbon and SW-style docked FeatureManager/PropertyManager are built either by importing a heavyweight third-party ribbon/docking library (license + maintenance + upstream-conflict surface) or by hacking `QMainWindow`'s menu/toolbar/dock system in `Gui::MainWindow` directly (merge hell, per Pitfall 3). Qt's native docking has quirks (float/restore, save/restore geometry across sessions, tab-merging) that a SW-faithful layout must handle; getting layout persistence wrong means panels don't come back where SW users expect.

**Why it happens:**
There's no built-in Qt ribbon, so teams grab a library or improvise; docking "mostly works" until layout save/restore and multi-monitor/float cases break.

**How to avoid:**
- Build the ribbon as a **self-contained Gui widget** (own files), driven by the existing CommandManager registrations, rather than rewiring `MainWindow`'s menu/toolbar internals in place. Reuse FreeCAD's command metadata so the ribbon and upstream commands stay in sync.
- If using a third-party ribbon/docking lib, vet its license (compatible with FreeCAD's LGPL distribution), its Qt 6.8 support, and its maintenance status — an abandoned UI dependency becomes its own long-term liability.
- Implement and test **layout save/restore** (panel positions, float state, multi-monitor) as an explicit feature; SW users expect their panel layout to persist.
- Keep docking config additive so upstream `QMainWindow`/dock changes don't collide.

**Warning signs:**
- Ribbon/docking work requires large in-place edits to `Gui::MainWindow`.
- Panels don't restore to their previous position/float state across restarts or monitors.
- A third-party UI dependency is unmaintained, GPL-incompatible, or lacks Qt 6.8 support.

**Phase to address:**
Ribbon phase and docking/panel-layout phase. The scaffolding phase should decide the ribbon/docking implementation strategy (custom vs library) up front with the license/maintenance check done.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Drop real SolidWorks icons in "temporarily" | Unblocks UI layout instantly | Copyright infringement baked into git history; legal exposure; can't ship | **Never** |
| Edit `Gui::MainWindow`/`Tree.cpp`/`PropertyEditor.cpp` in place | Fastest way to get the panel showing | Every upstream sync becomes multi-day merge hell; fork freezes on old base | Only for a single, marked, minimal hook line — never for bodies of logic |
| Stash SW UI state on `App::DocumentObject` | Nearest object, "feels like model data" | Breaks headless mode, pollutes `.FCStd`, diverges App layer from upstream | **Never** — keep it in Gui |
| Direct `obj->Member = x` instead of `setValue()` | Compiles, works in happy path | Silent undo/redo + dependency + GUI-sync corruption surfacing far from cause | **Never** |
| Manual Coin3D `ref`/`unref` in new code | Matches surrounding legacy style | Leaks/use-after-free over long sessions in already-fragile viewer code | Only with RAII wrappers; raw refcounting never |
| Global QSS for the whole SW theme | Quick SW look on the dev machine | Breaks native metrics on macOS, fights HiDPI, per-widget patch sprawl | Light cosmetic tweaks only; structural styling via QStyle |
| Test navigation/theme only on Windows | Matches the target audience's OS | Ships unusable on macOS/Linux, violates cross-platform requirement | Never as the *only* test surface |
| Quarterly big upstream merge | Less frequent context-switching | One giant unresolvable conflict; base date slips | Never — sync small and often |
| Parity judged by screenshots | Fast sign-off | "Looks right, feels wrong" fails real SW users; Core Value missed | Never for selection/PropertyManager/rollback |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| Upstream FreeCAD `main` | Diverge `main`, merge quarterly, scatter unmarked edits | Mirror branch kept byte-identical; SW work as rebasable patch series; small frequent syncs; `// SW-FORK HOOK` markers; pin tracked commit |
| FreeCAD CommandManager | Reimplement command registry for the ribbon | Drive the ribbon from existing command registrations/metadata; don't duplicate |
| FreeCAD Task panels | Replace wholesale to build PropertyManager | Build PropertyManager *on* the Task-panel/transaction machinery; change interaction semantics, reuse plumbing |
| Coin3D scene graph | Manual ref/unref in selection/highlight overlays | RAII node ownership; build in `attach()`, destroy in `detach()`; isolate SW nodes in own separators |
| Third-party ribbon/docking lib | Adopt without license/Qt-6.8/maintenance check | Verify LGPL-compatibility, Qt 6.8 support, active maintenance before adopting |
| `.FCStd` document format | Write GUI-only state into the App document | Persist UI state in Gui-side ViewProvider properties; keep `.FCStd` upstream-compatible |
| Headless/console mode (`MainCmd`) | Assume Gui changes can't affect it | Run console-load tests as a CI gate against App/Gui boundary violations |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Reusing `Tree.cpp`'s non-culling rendering in a reimplemented FeatureManager | UI sluggish with large models | Lazy/virtualized tree rendering for the SW tree from the start | >1000–5000 objects (documented limit) |
| Coin3D node leaks in SW selection/highlight overlays | RSS climbs over a session; restart "fixes" it | RAII Coin3D ownership; periodic ASan/Valgrind | Long modeling/selection sessions |
| Re-rendering all SW context glyphs/overlays every frame | Frame rate drops with many selectable items | Cull/instance overlays; only redraw on change | Dense scenes, large selections (>10k selectable) |
| QSS reapplied on widget reparenting across the ribbon | UI jank when switching workbenches/tabs | Minimize QSS scope; avoid reparenting-heavy patterns | Frequent ribbon-tab/workbench switching |
| Full recompute on every rollback-bar move | Lag dragging the rollback bar | Suppress-below as a Gui view; recompute only affected range | Large feature histories |

## Security Mistakes

> This is a desktop CAD GUI fork; classic web-security categories largely don't apply. Domain-relevant items:

| Mistake | Risk | Prevention |
|---------|------|------------|
| Shipping SW proprietary assets / trademark | IP infringement, takedown, legal liability (a "security-of-the-project" risk) | Provenance discipline + CI asset guard (Pitfall 1) |
| Inheriting upstream's documented plugin-import / URI-parsing weaknesses unnoticed | Untrusted addon/link execution (per CONCERNS.md) | Track upstream security fixes; don't fork-freeze away from them |
| Letting the fork drift off upstream security patches | Known CVEs/fixes in OCCT/Qt/Python deps never reach the fork | Frequent upstream sync (Pitfall 3) keeps security fixes flowing in |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Selection model is FreeCAD's, not SW's | SW users mis-select constantly; "zero relearning" broken | Spec SW selection directionality/pre-highlight/filters; acceptance-test with SW users |
| PropertyManager modality mismatch (Enter/Escape/Tab/focus/confirm) | Edits committed/aborted unexpectedly; flow feels alien | Spec SW PropertyManager interaction semantics; test focus/key behavior explicitly |
| Rollback that hides instead of suppress-below-and-insert-at | Feature insertion/edit-in-context behaves wrong, recompute errors | Implement true suppress-below + insert-at-bar over FreeCAD suppression |
| Windows-only navigation feel | macOS/Linux users can't rotate/pan/zoom as expected | Per-platform input profiles defaulting to SW conventions |
| Blurry icons / wrong metrics on HiDPI/macOS | UI looks broken/unprofessional, undermines parity | Vector icons + per-platform + per-scale visual QA |
| Panel layout doesn't persist | Users re-arrange panels every launch | Implement and test layout save/restore including float/multi-monitor |

## "Looks Done But Isn't" Checklist

- [ ] **Recreated icon set:** Often missing provenance proof — verify every shipped icon has an original source file and zero SW binaries are in the repo or git history.
- [ ] **Ribbon:** Often missing layout save/restore and HiDPI crispness — verify panels persist across restarts and icons are sharp at 200% and on Retina.
- [ ] **PropertyManager:** Often missing correct Enter/Escape/Tab/focus and transaction wrapping — verify undo/redo and keyboard flow match SW, not FreeCAD.
- [ ] **FeatureManager rollback:** Often missing true suppress-below + insert-at-bar + correct recompute — verify inserting a feature mid-tree behaves like SW and recompute stays clean.
- [ ] **Selection model:** Often missing SW box-select directionality, pre-highlight, and filter precedence — verify with a daily SW user, not screenshots.
- [ ] **Navigation:** Often missing macOS (no middle button) and Linux profiles — verify rotate/pan/zoom on all three OSes.
- [ ] **App/Gui boundary:** Often missing a headless check — verify a SW-fork-saved document opens in upstream FreeCAD and loads in `--console`.
- [ ] **Upstream sync:** Often missing a repeatable drill — verify there's a documented, scripted sync against a pinned upstream commit, and that the last sync took hours not days.
- [ ] **Coin3D overlays:** Often missing leak checks — verify a long selection/modeling session under ASan/Valgrind shows no growth.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| SW assets committed to history | HIGH | Purge from full git history (filter-repo), rotate/replace with originals, audit every release artifact; if already distributed, pull builds |
| Diverged `main` / merge hell | HIGH | Re-establish an untouched upstream mirror branch; reconstruct SW changes as a fresh rebasable patch series on top; re-mark hook sites; adopt small-frequent-sync going forward |
| App/Gui boundary breach | MEDIUM | Move UI state out of App objects into Gui controllers/ViewProvider props; restore `.FCStd` compatibility; add the headless CI gate |
| Notification/recompute corruption | MEDIUM | Replace direct member writes with `setValue()`; wrap edits in transactions; add undo/redo + recompute regression tests |
| Coin3D leaks | MEDIUM | Convert manual ref/unref to RAII wrappers; audit attach/detach; add leak-check to CI for viewport code |
| QSS/HiDPI breakage on a platform | LOW–MEDIUM | Narrow QSS scope or move to QStyle; switch raster icons to SVG; add per-platform/per-scale QA |
| Wrong UX feel discovered late | MEDIUM–HIGH | Write the missing behavioral spec, re-implement interaction semantics, add SW-user acceptance tests — costlier the later it's found |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Proprietary asset copying (1) | Visual-theme / icon-set phase | CI asset-hash guard + provenance file; no "SolidWorks" string in UI |
| Trade-dress overreach (2) | Visual-theme + release-legal-review checkpoint | Marketing/branding review; distinct palette + product name |
| Upstream merge hell (3) | Foundational scaffolding phase (first) | Scripted sync against pinned commit completes in hours; hook sites all marked |
| App/Gui boundary break (4) | Scaffolding (gate) + FeatureManager + PropertyManager phases | Headless `--console` load test + `.FCStd` opens in upstream FreeCAD |
| Notification/recompute bypass (5) | PropertyManager + FeatureManager phases (and any command phase) | Undo/redo + recompute regression tests pass |
| Coin3D leaks (6) | 3D-viewport interaction phase | ASan/Valgrind clean over long session |
| Superficial UX parity (7) | Selection + PropertyManager + rollback phases | SW-user acceptance tests, not screenshots |
| Cross-platform navigation (8) | Mouse-navigation/selection phase | Manual rotate/pan/zoom verified on Win+macOS+Linux |
| QSS/HiDPI theming (9) | Visual-theme phase | Visual QA at 100/150/200% and on Retina/macOS |
| Ribbon/docking vs Qt & upstream (10) | Ribbon + docking/layout phases (strategy set in scaffolding) | Layout save/restore works; minimal `MainWindow` edits; lib license/Qt6.8 vetted |

## Sources

- FreeCAD codebase maps (authoritative for this fork): `.planning/codebase/ARCHITECTURE.md` (App/Gui separation, anti-patterns: direct property access, modify-during-recompute, Coin3D ref/unref), `.planning/codebase/CONCERNS.md` (Tree.cpp scaling, View3DInventorViewer fragility, Sketcher memory leaks, Qt6 migration gaps), `.planning/codebase/TESTING.md`, `.planning/PROJECT.md` (constraints, out-of-scope legal boundary).
- [Google v. Oracle — EFF case summary](https://www.eff.org/cases/oracle-v-google) and [Copyright Lately analysis](https://copyrightlately.com/its-the-end-of-google-v-oracle-and-i-think-i-feel-fine/) — reimplementation/fair-use context (functional interface reuse defensible; verbatim creative-work copying is not).
- [Apple v. Samsung — iPhone UI held functional for trade dress (Stroock)](https://www.stroock.com/news-and-insights/apples-iphone-user-interface-held-functional-for-trade-dress-infringement-but-not-design-patent-purposes) and [Law Journal Newsletters](https://www.lawjournalnewsletters.com/sites/lawjournalnewsletters/2015/07/01/apples-iphone-user-interface-held-functional-for-trade-dress-infringement-but-not-design-patent-purposes/) — functional UI/icons generally not trade-dress-protectable; non-functional distinctive presentation can be.
- [Trade dress protection for digital products (Turley Law)](https://turleylaw.com/blog/trade-dress-protection-websites-digital-products) — distinctiveness/secondary-meaning + non-functionality framework.
- [GitHub Blog — Strategies for friendly fork management](https://github.blog/2022-05-02-friend-zone-strategies-friendly-fork-management/), [Open Energy Transition — Soft Fork Strategy](https://open-energy-transition.github.io/handbook/docs/Engineering/SoftForkStrategy/), [conda-forge — keep your fork in sync](https://conda-forge.org/docs/how-to/basics/fork-sync/) — mirror-branch + frequent-sync + `range-diff/--skip` patterns.
- [KDAB — Say No to Qt Style Sheets](https://www.kdab.com/say-no-to-qt-style-sheets/) and [Qt Style Sheets docs](https://doc.qt.io/qt-6/stylesheet.html) — QSS swaps native style, cross-platform metric side-effects, macOS pitfalls.
- [Qt Forum — stylesheet disturbs appearance on macOS](https://forum.qt.io/topic/109262/simple-background-color-stylesheet-disturbs-appearance-on-macos) — concrete macOS QSS breakage.

> Legal note: The legal pitfalls reflect public case law and general principles; they are **not legal advice**. Before public distribution, have IP counsel review the visual theme and marketing copy.

---
*Pitfalls research for: FreeCAD SolidWorks-style GUI fork*
*Researched: 2026-06-06*

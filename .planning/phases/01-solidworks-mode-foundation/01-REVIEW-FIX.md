---
phase: 01-solidworks-mode-foundation
fixed_at: 2026-06-07T00:00:00Z
review_path: .planning/phases/01-solidworks-mode-foundation/01-REVIEW.md
iteration: 1
findings_in_scope: 10
fixed: 10
skipped: 0
status: all_fixed
---

# Phase 01: Code Review Fix Report

**Source review:** `.planning/phases/01-solidworks-mode-foundation/01-REVIEW.md`
**Iteration:** 1
**Branch:** `solidworks`

**Summary:**
- Findings in scope (Critical + Warning): 10
- Fixed: 10 (4 Critical, 6 Warning)
- Skipped: 0
- Bonus: IN-01 (dead import) fixed opportunistically in the same file as WR-06.

All fixes were applied in an isolated git worktree, each committed atomically with
normal pre-commit hooks (no `--no-verify`), then fast-forwarded onto `solidworks`.
The three fork guards (`fw-string-leak-grep.sh`, `fw-provenance-guard.sh`,
`fw-sync-upstream.sh`) were re-run after the changes and all pass.

## Fixed Issues

### CR-01: Dock shell never appears on first activation
**Files modified:** `src/Gui/FreeWorks/FwWorkbench.cpp`, `src/Gui/FreeWorks/FwLayout.cpp`, `src/Gui/FreeWorks/FwLayout.h`
**Commit:** `5fed81a291`
**Applied fix:** Moved `FwLayout::install()` from `activated()` (which the framework
runs AFTER `DockWindowManager::setup()` consumes the dock items) into
`setupDockWindows()` (which `Workbench::activate()` runs immediately before
`setup()`, confirmed at `src/Gui/Workbench.cpp:463-465`). The placeholder widgets
now exist under their `Fw_*` names when `setup()` looks them up, so the three
permanent docks appear on first activation. `applyDefault()` stays in `activated()`.

### CR-02: All placeholder docks shared objectName "FwPlaceholder"
**Files modified:** `src/Gui/FreeWorks/FwLayout.cpp` (committed with CR-01)
**Commit:** `5fed81a291`
**Applied fix:** `makePlaceholder()` now takes a per-dock `objectName` (its `Fw_*`
dock name) and a `windowTitle`. `ensureDock()` passes the dock name as the
objectName so each QDockWidget gets a unique, restorable identity (fixes
`QMainWindow::saveState()/restoreState()` round-trip) and a rendered title.
Bundled with CR-01 because both changes live in the same two helper functions and
must land together to keep the commit compilable.

### CR-03: gtest called protected `setupDockWindows()`
**Files modified:** `tests/src/Gui/FwWorkbench.cpp`
**Commit:** `22c0e3b21e`
**Applied fix:** Added `FwWorkbenchAccessor : public FwWorkbench` that re-publishes
the protected `setupDockWindows()` via a `using`-declaration (the project's
documented convention), and switched the test to instantiate the accessor. No
production visibility was widened.

### CR-04: Upstream-sync guard pinned to a fork planning commit
**Files modified:** `tools/fw-sync-upstream.sh`
**Commit:** `efd04aa25b`
**Applied fix:** Re-pointed `DEFAULT_PIN` from `fa185a2b00` (a `.planning/`-only
fork commit) to `768e237091` — upstream PR #30001, the parent of the first fork
commit (`chore: track .planning`) and the merge-base of `solidworks` with upstream
`main`. Verified the new base has no `.planning/` and no `src/Gui/FreeWorks/`
trees and is an ancestor of HEAD. Added a step-0 self-check that fails loudly if
the pin is not an ancestor of HEAD or carries fork artifacts, and documented the
re-baselining procedure. The offline drill now correctly diffs against a real
upstream base (reports `src/Gui/CMakeLists.txt` as the sole marked shared edit)
and exits 0.

### WR-01: `FwLayout::install()` ignored its only parameter
**Files modified:** `src/Gui/FreeWorks/FwLayout.h`, `src/Gui/FreeWorks/FwLayout.cpp`, `src/Gui/FreeWorks/FwWorkbench.cpp` (committed with CR-01)
**Commit:** `5fed81a291`
**Applied fix:** Dropped the unused `QMainWindow*` parameter (it was `Q_UNUSED`);
`install()` now operates only through the `DockWindowManager` singleton. Call site
and forward declaration updated. Bundled with CR-01 because the signature change
and the call-site change are mutually dependent.

### WR-02: `FwTheme::apply()` was dead code
**Files modified:** `src/Gui/FreeWorks/FwWorkbench.cpp`
**Commit:** `b124200fbb`
**Applied fix:** Wired the no-op `FwTheme::apply()` call into
`FwWorkbench::activated()` so the theming seam is live and discoverable now.

### WR-03: Nav-default mac key comment overstated behavior
**Files modified:** `src/Gui/FreeWorks/FwNavigationDefault.cpp`
**Commit:** `45cecf78b3`
**Applied fix:** Downgraded the comment to state plainly that the `FwMacNavProfile`
write only records intent and that nothing in this phase reads it; the actual
MMB-emulation binding lands in a later phase. No behavior change. (Chose the
comment-accuracy option over wiring real behavior, since the consuming code is
explicitly out of this phase's scope.)

### WR-04: `<QMainWindow>`/`<QLabel>` only included under `!_PreComp_`
**Files modified:** `src/Gui/FreeWorks/FwLayout.cpp` (committed with CR-01)
**Commit:** `5fed81a291`
**Applied fix:** Include `<QLabel>` unconditionally rather than relying on the PCH
transitive include. `<QMainWindow>` is no longer needed since WR-01 removed the
parameter. Bundled with CR-01 (same file/region).

### WR-05: Provenance guard `is_listed` used unanchored substring match
**Files modified:** `tools/fw-provenance-guard.sh`
**Commit:** `d922e2be81`
**Applied fix:** Anchored the match to a real Markdown table cell
(`^\| \`path\` \|`) and regex-escaped the path. Verified the real icon is still
accepted (exit 0) and the unlisted self-test image is still rejected (exit 1).

### WR-06: Stale "produced in Plan 04" asset comments
**Files modified:** `src/Gui/FreeWorks/Resources/FreeWorks.qrc`, `src/Gui/FreeWorks/InitGui.py`
**Commit:** `7d090ad706`
**Applied fix:** Updated both comments to reflect that
`icons/FreeWorksWorkbench.svg` and its `ASSET_PROVENANCE.md` row already exist.
Also removed the unused `import FreeCAD as App` (IN-01) in the same file.

## Notes on commit grouping

CR-01, CR-02, WR-01, and WR-04 were committed together (`5fed81a291`) because they
all modify the same two functions (`makePlaceholder`/`ensureDock`) and the
`install()` signature, and splitting them would produce intermediate commits that
do not compile — violating the CLAUDE.md rule "each commit must compile cleanly
with previous commits." Every other finding was committed individually.

## Deferred (Info findings, out of scope)

IN-02 (defensive null guard), IN-03 (centralize dock-name constants), IN-04
(verify tabbing behavior visually after CR-01 — a runtime check, not a code fix),
and IN-05 (find/pipefail masking) were left for a follow-up; they are
non-blocking and were outside the Critical+Warning scope. IN-01 was fixed because
it was trivially adjacent to WR-06.

## Verification performed
- Tier 1 (re-read) on every edit.
- Tier 2: `bash -n` on both modified shell scripts; `python3 ast.parse` on
  `InitGui.py`; both fork shell guards executed end-to-end (including the
  provenance self-test) and the offline upstream-sync drill (exit 0).
- C++ TUs cannot be compiled in this environment (delegated to CI); changes were
  reasoned against the cited upstream signatures (`Workbench.cpp:463-465`,
  `DockWindowManager.h` `registerDockWindow`/`findRegisteredDockWindow`,
  `MainWindow.cpp:623,640` objectName/windowTitle convention).

---

_Fixed: 2026-06-07_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_

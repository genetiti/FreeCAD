---
phase: 01-solidworks-mode-foundation
reviewed: 2026-06-07T00:00:00Z
depth: standard
files_reviewed: 22
files_reviewed_list:
  - src/Gui/FreeWorks/FwWorkbench.h
  - src/Gui/FreeWorks/FwWorkbench.cpp
  - src/Gui/FreeWorks/FwLayout.h
  - src/Gui/FreeWorks/FwLayout.cpp
  - src/Gui/FreeWorks/FwNavigationDefault.h
  - src/Gui/FreeWorks/FwNavigationDefault.cpp
  - src/Gui/FreeWorks/FwTheme.h
  - src/Gui/FreeWorks/FwTheme.cpp
  - src/Gui/FreeWorks/PreCompiled.h
  - src/Gui/FreeWorks/PreCompiled.cpp
  - src/Gui/FreeWorks/InitGui.py
  - src/Gui/FreeWorks/CMakeLists.txt
  - src/Gui/FreeWorks/Resources/FreeWorks.qrc
  - src/Gui/CMakeLists.txt
  - tests/src/Gui/FwWorkbench.cpp
  - tests/src/Gui/CMakeLists.txt
  - .github/workflows/CI_primary.yml
  - .github/workflows/sub_fwHeadlessCompat.yml
  - .github/workflows/sub_fwForkGuards.yml
  - tools/fw-provenance-guard.sh
  - tools/fw-string-leak-grep.sh
  - tools/fw-sync-upstream.sh
findings:
  critical: 4
  warning: 6
  info: 5
  total: 15
status: issues_found
---

# Phase 01: Code Review Report

**Reviewed:** 2026-06-07
**Depth:** standard
**Files Reviewed:** 22
**Status:** issues_found

## Summary

The FreeWorks submodule is structurally additive and respects App/Gui separation
in principle (no App-layer includes in the layout path, nav default written through
the parameter system, single marked `add_subdirectory` in `src/Gui/CMakeLists.txt`).
The fork guards (`fw-string-leak-grep.sh`, `fw-provenance-guard.sh`) were executed
against the working tree and pass; `Gui::SolidWorksNavigationStyle` was verified to
be a genuine upstream type, so the D-03 exception is legitimate.

However, the core deliverable — "mount the dock shell on workbench activation" —
does not work as written, and the only test target will not compile. Four
blocker-class defects were found by tracing the activation call chain into the
upstream `Workbench` / `DockWindowManager` code that this module depends on:

1. The dock placeholders are registered (`FwLayout::install`) *after* the framework
   has already consumed the dock-window items, so the docks do not appear on first
   activation.
2. All three placeholder widgets share the objectName `"FwPlaceholder"`, which the
   framework propagates to the QDockWidget objectName — breaking Qt layout
   save/restore (which requires unique objectNames) and producing blank dock titles.
3. The gtest calls a `protected` member from a non-friend, non-derived context — a
   guaranteed compile error in `Gui_tests_run`.
4. The `fw-sync-upstream.sh` merge-discipline guard pins to a *fork planning commit*
   rather than a pristine upstream base, so it cannot detect un-marked shared-file
   edits introduced at or before that pin.

The grouped findings follow. Line numbers refer to the reviewed source.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Dock shell never appears on first activation (registration runs after consumption)

**File:** `src/Gui/FreeWorks/FwWorkbench.cpp:44-55`, `src/Gui/FreeWorks/FwLayout.cpp:65-83`
**Issue:**
The framework consumes the dock-window items inside `Workbench::activate()`
(`src/Gui/Workbench.cpp:463-465`):

```cpp
DockWindowItems* dw = setupDockWindows();
WorkbenchManipulator::changeDockWindows(dw);
DockWindowManager::instance()->setup(dw);   // looks up _dockWindows by name HERE
```

`DockWindowManager::setup()` (`src/Gui/DockWindowManager.cpp:530-532`) only creates a
dock widget when a widget is already registered under that name in `_dockWindows`:

```cpp
QMap<...>::Iterator jt = d->_dockWindows.find(it.name);
if (jt != d->_dockWindows.end()) { dw = addDockWindow(...); }
```

But the FreeWorks placeholders are registered by `FwLayout::install()`, which is
called from `FwWorkbench::activated()`. The activation sequence in
`Gui::Application` is: `WorkbenchManager::activate()` → `wb->activate()`
(`WorkbenchManager.cpp:119`, runs `setup()`) **then later** `newWb->activated()`
(`Application.cpp:2006`, runs `install()`). So at the moment `setup()` looks up
`Fw_FeatureManager` / `Fw_PropertyManager` / `Fw_TaskPane`, none are registered yet;
no docks are created. The placeholders are registered immediately afterward but
`setup()` is not re-run until the next workbench switch. Net effect: the three
permanent FreeWorks docks **do not appear on first activation** — exactly the
deliverable this phase claims.

**Fix:** Register the placeholder widgets before the framework consumes the dock
items. Do it in the `FwWorkbench` constructor or at the top of `setupDockWindows()`
(which runs inside `activate()` before `setup()`), not in `activated()`:

```cpp
Gui::DockWindowItems* FwWorkbench::setupDockWindows() const
{
    // Ensure the permanent Fw_* dock names are backed by widgets BEFORE the
    // framework's DockWindowManager::setup() consumes the returned items.
    FwLayout::install(Gui::getMainWindow());

    auto* root = new Gui::DockWindowItems();
    root->addDockWidget("Fw_FeatureManager", ...);
    ...
    return root;
}
```
Then drop the `FwLayout::install()` call from `activated()`.

### CR-02: All placeholder docks share objectName "FwPlaceholder" — breaks Qt layout save/restore

**File:** `src/Gui/FreeWorks/FwLayout.cpp:44-51`
**Issue:**
`makePlaceholder()` sets the same objectName on every placeholder:

```cpp
label->setObjectName(QStringLiteral("FwPlaceholder"));
```

`DockWindowManager::addDockWindow()` copies the *widget's* objectName onto the
QDockWidget (`src/Gui/DockWindowManager.cpp:290`: `dw->setObjectName(QString::fromUtf8(name))`,
where `name` is `jt.value()->objectName()` from `setup()` line 532). All three
FreeWorks QDockWidgets therefore end up with the identical objectName
`"FwPlaceholder"`. `QMainWindow::saveState()/restoreState()` key dock layout by
objectName and require it to be **unique per dock** — duplicates produce
`QMainWindow::saveState(): 'objectName' not unique` warnings and make the saved
geometry non-restorable. This directly defeats stated goal OQ-2 ("permanent dock
names onto which later phases swap content without re-layout"): the layout cannot
round-trip. Additionally, no `windowTitle` is set, so `dw->setWindowTitle(widget->windowTitle())`
(line 291) gives all three docks empty titles.

The upstream convention (`src/Gui/MainWindow.cpp:623,640`) sets a distinct
objectName and windowTitle per registered widget.

**Fix:** Give each placeholder a unique objectName matching its dock name, plus a
window title:

```cpp
QWidget* makePlaceholder(const char* objectName, const QString& title, const QString& text)
{
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setObjectName(QString::fromUtf8(objectName));
    label->setWindowTitle(title);
    return label;
}
// ensureDock(manager, "Fw_FeatureManager", "Fw_FeatureManager",
//            QObject::tr("FeatureManager"), QObject::tr("FeatureManager (Phase 3)"));
```

### CR-03: gtest calls protected `setupDockWindows()` — `Gui_tests_run` will not compile

**File:** `tests/src/Gui/FwWorkbench.cpp:79-82`, `src/Gui/FreeWorks/FwWorkbench.h:60-68`
**Issue:**
`setupDockWindows()` is declared `protected` in `FwWorkbench` (and in the base
`Gui::Workbench`). The test does:

```cpp
FreeWorksGui::FwWorkbench workbench;
std::unique_ptr<Gui::DockWindowItems> docks(workbench.setupDockWindows());
```

`FwWorkbenchTest` neither derives from `FwWorkbench` nor is declared a `friend`, so
this is an access violation: `error: 'setupDockWindows' is a protected member`. The
`FwWorkbench.cpp` test will fail to compile, taking the whole `Gui_tests_run`
target down with it. (The other two tests call the public static
`FwNavigationDefault::applyDefault()` and are fine.) This test was evidently never
built.

**Fix:** Per the project convention ("create a subclass that exposes protected
members"), add a test shim instead of widening production visibility:

```cpp
class FwWorkbenchAccessor: public FreeWorksGui::FwWorkbench {
public:
    using FreeWorksGui::FwWorkbench::setupDockWindows;  // expose for test
};
...
FwWorkbenchAccessor workbench;
std::unique_ptr<Gui::DockWindowItems> docks(workbench.setupDockWindows());
```

### CR-04: Upstream-sync guard pins to a fork planning commit, not a pristine upstream base

**File:** `tools/fw-sync-upstream.sh:55`, `.github/workflows/sub_fwForkGuards.yml:101-104`
**Issue:**
```bash
DEFAULT_PIN="fa185a2b007d0e0778113fb5e7843a0de32e1527"
```
This commit is `docs(01): create phase plan` — a fork-internal commit that touches
only `.planning/` files (verified: `git show --stat` lists only `.planning/...`).
It is NOT a pristine upstream FreeCAD base. The guard's load-bearing assertion
(step 4) diffs shared files with `git diff --name-only "${PINNED_SHA}" -- src/`
(line 165), which only surfaces `src/` edits made *after* this planning commit.
Any unmarked shared-file edit introduced in a commit at or before the pin is
invisible to the guard — defeating the entire purpose of threat T-01-12 (catch
un-marked edits to shared files). The drill therefore gives false assurance: it
reports "PASS — all shared-file touches are SW-FORK HOOK-greppable" while being
structurally unable to see edits older than the pin.

The script's own comment (lines 51-55) claims the pin is "the fork's pre-FreeWorks
upstream base," which contradicts the actual commit content.

**Fix:** Set `DEFAULT_PIN` to a real upstream FreeCAD commit SHA that predates all
fork changes (the merge-base of the `solidworks` branch with upstream `main`), and
add a self-check that the pin is reachable on the upstream remote and is an
ancestor of `HEAD`. Document the re-baselining procedure when bumping it.

## Warnings

### WR-01: `FwLayout::install()` ignores its only parameter — dead API surface

**File:** `src/Gui/FreeWorks/FwLayout.cpp:65-69`, `src/Gui/FreeWorks/FwLayout.h:46`
**Issue:**
`install(QMainWindow* mainWindow)` immediately does `Q_UNUSED(mainWindow)` and
operates entirely on `DockWindowManager::instance()`. The parameter is never used.
The header documents it as the install target, but the implementation discards it.
This is misleading: a caller passing a non-main window (or nullptr) gets identical
behavior, and the "observe-the-DOM via the public getter" intent is not actually
exercised. The call site already computes `Gui::getMainWindow()` only to feed it
into a function that throws it away.

**Fix:** Either remove the parameter (preferred, since the manager is a singleton):
`static void install();` — or actually use it (e.g. pass it to
`addDockWindow`/parenting). If kept for future use, leave a TODO explaining why.

### WR-02: `FwTheme::apply()` is dead code — declared, exported, never called

**File:** `src/Gui/FreeWorks/FwTheme.cpp:31-39`, `src/Gui/FreeWorks/FwTheme.h:43`
**Issue:**
`FwTheme::apply()` is a no-op that is never invoked anywhere in the module —
`FwWorkbench::activated()` calls `FwLayout::install()` and
`FwNavigationDefault::applyDefault()` but not `FwTheme::apply()`. The header states
the hook exists "so the activation seam has a stable place to call into," yet the
seam does not call it. As written it is unreachable code that will silently rot;
when Phase 7 wires theming it may be forgotten because nothing references it today.

**Fix:** Either call it from `FwWorkbench::activated()` now (a no-op call is cheap
and proves the seam), or remove the class until Phase 7 actually needs it. A
declared-but-never-called exported stub is dead surface area.

### WR-03: Nav-default writes a second mac-only key that no consumer reads

**File:** `src/Gui/FreeWorks/FwNavigationDefault.cpp:57-74`
**Issue:**
On macOS the code writes `FwMacNavProfile = "ModifierEmulatedMMB"` into the View
preference group. Nothing in this phase (or anywhere in the reviewed code) reads
`FwMacNavProfile`; it is documented as pointing at `MACOS_NAV_PROFILE.md`. Writing
a preference key that no code consumes means the "modifier-emulated MMB profile" is
not actually applied — the comment claims it "selects the modifier-emulated-MMB
substitute profile," but it only records a string. On a Mac with no 3-button mouse,
`Gui::SolidWorksNavigationStyle` (which the same function sets) has no MMB emulation,
so the default is non-functional on the exact platform this branch targets. At
minimum the comment overstates what the code does.

**Fix:** Either wire `FwMacNavProfile` to a real navigation-style selection (e.g.
set `NavigationStyle` to a style that emulates MMB via modifiers on macOS), or
downgrade the comment to state plainly that this only records intent and the
behavior lands in a later phase. Do not imply a behavior that is not implemented.

### WR-04: `FwLayout.cpp` includes `<QMainWindow>` only when `_PreComp_` is undefined, but uses the type unconditionally

**File:** `src/Gui/FreeWorks/FwLayout.cpp:27-30`, `:65`
**Issue:**
```cpp
#ifndef _PreComp_
#include <QLabel>
#include <QMainWindow>
#endif
```
`FwLayout::install(QMainWindow* mainWindow)` references `QMainWindow` in its
signature. This compiles only because `PreCompiled.h` pulls in `Gui/QtAll.h`
(which transitively includes the Qt widgets). If the PCH ever stops including
`QMainWindow` (or a non-PCH build path is used where `_PreComp_` is defined but
`QtAll.h` is trimmed), this translation unit breaks. Relying on a transitive PCH
include for a type in your own public signature is fragile. The header forward-
declares `class QMainWindow;` but the `.cpp` needs the full type for nothing (it
only stores the pointer, and even that is unused — see WR-01).

**Fix:** Include `<QMainWindow>` (and `<QLabel>`) unconditionally in the `.cpp`, or
drop the parameter per WR-01 so the full type is not needed.

### WR-05: Provenance guard's `is_listed` uses unanchored substring match

**File:** `tools/fw-provenance-guard.sh:65-68`
**Issue:**
```bash
is_listed() { grep -qF "\`${rel}\`" "${LEDGER}"; }
```
This matches the back-ticked path as a substring anywhere in the ledger, not as a
full table cell. A future ledger row such as
`` `src/Gui/FreeWorks/Resources/icons/foo.svg` `` would also satisfy the check for a
*different* asset whose relative path happens to be a back-ticked substring inside a
longer back-ticked string (e.g. if a path were embedded in a prose sentence between
backticks). More practically, it does not verify the row is a real table row, so a
path mentioned in the "How to add" prose example would falsely satisfy the guard.

**Fix:** Anchor the match to a table cell, e.g. require a leading `| ` and trailing
` |` around the back-ticked path: `grep -qE "^\| \`${rel}\` \|" "${LEDGER}"` (after
escaping `rel` for regex), or parse the table column explicitly.

### WR-06: `.qrc` reservation comment is stale — icon already exists but doc says it is produced "in Plan 04"

**File:** `src/Gui/FreeWorks/Resources/FreeWorks.qrc:4-7`, `src/Gui/FreeWorks/InitGui.py:40-42`
**Issue:**
The `.qrc` comment and the `InitGui.py` comment both say the icon binary and its
provenance row "are produced in Plan 04 / this manifest reserves the resource path."
But `src/Gui/FreeWorks/Resources/icons/FreeWorksWorkbench.svg` already exists and is
already listed in `ASSET_PROVENANCE.md`. A `.qrc` that references a missing file
fails the Qt resource compile; here it happens to exist, so the stale comment is
merely misleading — but if a reader trusts the comment they may assume the build
will fail without the icon and waste time, or delete the "placeholder" path.

**Fix:** Update both comments to reflect that the icon and its provenance row now
exist. Keep documentation in sync with the tree to avoid contradictory guidance.

## Info

### IN-01: Unused `import FreeCAD as App` in InitGui.py

**File:** `src/Gui/FreeWorks/InitGui.py:27`
**Issue:** `App` is imported but never referenced (only `Gui` is used). Dead import.
**Fix:** Remove `import FreeCAD as App` (keep `import FreeCADGui as Gui`).

### IN-02: `FwNavigationDefault::applyDefault()` writes via SetASCII without a parameter-group existence guard

**File:** `src/Gui/FreeWorks/FwNavigationDefault.cpp:39-45`
**Issue:** `GetParameterGroupByPath()` is assumed non-null and dereferenced via
`hGrp->GetASCII(...)`. In normal operation this group always exists, but other
FreeWorks code paths (e.g. tests, headless) call into the parameter system; a
defensive check matches the null-tolerance shown in `FwLayout` (which guards
`manager == nullptr`).
**Fix:** Optional: assert/guard `if (!hGrp) return;` for symmetry with FwLayout.

### IN-03: Magic dock-name strings duplicated across production and tests

**File:** `src/Gui/FreeWorks/FwWorkbench.cpp:85-93`, `src/Gui/FreeWorks/FwLayout.cpp:80-82`, `tests/src/Gui/FwWorkbench.cpp:24-26`
**Issue:** The three `Fw_*` dock-name string literals are repeated verbatim in
`FwWorkbench::setupDockWindows()`, `FwLayout::install()`, and the test. A typo in
one location silently desyncs registration from the dock-items list (and would
manifest as a missing dock rather than a compile error).
**Fix:** Define the three names once (e.g. `constexpr const char*` in a shared
FreeWorks header) and reference them from all three sites.

### IN-04: `tabify` uses a function-local `static bool` — order/once-only coupling

**File:** observed dependency `src/Gui/DockWindowManager.cpp:558` (not FreeWorks-owned)
**Issue:** Not a FreeWorks defect, but the FreeWorks docks rely on
`tabifyDockWidgets()`'s one-shot `static bool tabify` to tabify `Fw_TaskPane`
(requested as `VisibleTabbed`). If another workbench triggers the one-shot first,
the FreeWorks tabbing request may be skipped. Flagged as a behavioral dependency to
verify once CR-01 is fixed and the docks actually instantiate.
**Fix:** None required in this module; verify FreeWorks tabbing visually after CR-01.

### IN-05: `set -euo pipefail` + `find | sort` masks `find` failure in provenance guard

**File:** `tools/fw-provenance-guard.sh:106`
**Issue:** `done < <(find ... | sort -z)` — under `pipefail` the pipeline's exit
status is `sort`'s, so a `find` failure (e.g. permission error) is swallowed and the
guard reports "0 unlisted" instead of erroring. Low impact for a fixed in-repo path.
**Fix:** Optional: capture find output to a temp file and check its status, or use
`find ... -print0 | sort -z` with a `PIPESTATUS` check.

---

_Reviewed: 2026-06-07_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_

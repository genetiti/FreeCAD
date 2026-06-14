---
status: complete
phase: 02-commandmanager-ribbon
source: [02-VERIFICATION.md]
started: 2026-06-13T23:30:00Z
updated: 2026-06-14T00:30:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Live ribbon visual
expected: In FreeWorks mode a tabbed ribbon (Features/Sketch/Evaluate) is mounted in the top toolbar area, replacing menus+toolbars; buttons are large icon-over-label (32px); reads as SolidWorks at a glance.
result: pass

### 2. Flyout split-button popup interaction
expected: Clicking the arrow on a `*_Comp*` split-button (e.g. a dress-up/primitive group) opens a MenuButtonPopup dropdown listing related commands; selecting one fires it; the primary click fires the default command.
result: pass

### 3. Live menu bar + stock toolbar hide/restore across activate/deactivate
expected: Activating FreeWorks mode reversibly hides the menu bar + stock toolbars; switching to a different workbench fully restores the prior chrome (including the macOS native menu bar).
result: pass

### 4. Live sketch enter → Sketch tab → exit → prior tab restored (RIBBON-02; closes SPIKE item-5)
expected: From a non-Sketch tab, entering a sketch auto-activates the Sketch tab; exiting restores the previously-active tab (observed after-tab index == before-tab index). Also re-confirms the `"SketcherGui::ViewProviderSketch"` type-name literal still matches upstream.
result: pass

### 5. True app-restart persistence
expected: Select a non-default tab (e.g. Evaluate), close FreeCAD, relaunch in FreeWorks mode — the ribbon restores the saved tab.
result: pass

## Summary

total: 5
passed: 5
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None recorded — all five are live-GUI-only verification items deferred under the
project's no-build-tree convention (consistent with Phase 1's TRIOS_LAUNCH_CHECKLIST.md
and Plan 02-01's SPIKE_LIVE_CHECKLIST.md). Item 4 closes the SPIKE_LIVE_CHECKLIST.md
item-5 open obligation. None are implementation gaps — every success criterion is
source-verified per 02-VERIFICATION.md.

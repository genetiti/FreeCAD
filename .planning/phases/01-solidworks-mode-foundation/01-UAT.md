---
status: testing
phase: 01-solidworks-mode-foundation
source: [01-VERIFICATION.md]
started: 2026-06-07
updated: 2026-06-07
---

## Current Test

number: 1
name: Tri-OS GUI launch & SolidWorks-style dock shell mounts correctly
expected: |
  On each of Windows, macOS, and Linux: build FreeCAD with the FreeWorks module
  linked, launch the GUI, select "FreeWorks" in the workbench selector, and confirm
  the Fw_FeatureManager + Fw_PropertyManager docks mount on the LEFT, Fw_TaskPane on
  the RIGHT, the top toolbar area is left free for the Phase 2 ribbon, no crash on
  activation or workbench switch, and the SolidWorks navigation default is active on
  a fresh profile. Walk src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md.
awaiting: user response

## Tests

### 1. Tri-OS GUI launch & SolidWorks-style dock shell mounts correctly
expected: Build + launch on Windows, macOS, Linux; FreeWorks workbench selectable; Fw_FeatureManager/Fw_PropertyManager dock left, Fw_TaskPane right, top ribbon area reserved; no crash on activation/switch; nav default active on fresh profile; per-OS CI matrix (sub_buildWindows / sub_buildUbuntu / sub_buildPixi) + sub_fwHeadlessCompat + sub_fwForkGuards all green. Drive via src/Gui/FreeWorks/TRIOS_LAUNCH_CHECKLIST.md.
result: [pending]

### 2. macOS real-hardware navigation feel (no-middle-button / trackpad)
expected: On real Apple hardware, walk MACOS_NAV_PROFILE.md §4 spike checklist — Option+left-drag rotate, +Command pan, scroll zoom-to-cursor, +Control roll, +Shift dolly behave with reference-CAD parity; Control-click still opens context menu (no secondary-click regression); switching to Gui::GestureNavigationStyle works and the choice persists across relaunch (no-clobber). Record any gesture conflicts / modifier-ergonomics issues.
result: [pending]

## Summary

total: 2
passed: 0
issues: 0
pending: 2
skipped: 0
blocked: 0

## Gaps

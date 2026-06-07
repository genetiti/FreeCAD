<!-- SPDX-License-Identifier: LGPL-2.1-or-later -->

# FreeWorks macOS Navigation Profile (OQ-3)

> Scope: the macOS no-middle-button / trackpad substitute for the FreeWorks
> default 3D-viewport navigation. On Windows and Linux the FreeWorks default is
> the reference-CAD mouse model on a physical 3-button mouse (rotate = MMB drag,
> pan = Ctrl+MMB, zoom = scroll-wheel zoom-to-cursor, roll = Alt+MMB,
> dolly = Shift+MMB, middle-click-entity then middle-drag rotates about it).
> Most Macs ship without a physical middle button, and the reference-CAD nav
> style has **no built-in middle-button emulation**, so macOS needs an explicit,
> documented substitute. This file is the single source of truth for that
> substitute; `FwNavigationDefault::applyDefault()` records the selected profile
> in the `FwMacNavProfile` preference key.

---

## 1. Default macOS profile — Modifier-emulated MMB

On macOS, FreeWorks keeps the **same `NavigationStyle` default** as every other
platform (`Gui::SolidWorksNavigationStyle`) so the button *semantics* are
identical. The only difference is how the physical middle button is produced:
the middle button is **emulated with a modifier + left-drag**, preserving the
reference-CAD chord layering (Ctrl / Option / Shift then modify the emulated MMB).

`FwNavigationDefault` writes `FwMacNavProfile = "ModifierEmulatedMMB"` when the
key is unset (no-clobber, mirroring the `NavigationStyle` default discipline).

### Chosen modifier chord

| Action | macOS chord (emulated MMB) | Rationale |
|--------|----------------------------|-----------|
| **Rotate** (MMB drag) | **⌥ Option + left-drag** | Option is free in the FreeCAD viewport and reachable with the left thumb while the left hand drives the trackpad/mouse. Avoids the macOS-reserved **Control + click = secondary (right) click** conflict. |
| **Pan** (Ctrl + MMB) | **⌥ Option + ⌘ Command + left-drag** | Adds Command (the platform-idiomatic "primary" modifier) on top of the emulated MMB. Command stands in for the desktop Ctrl so the literal Control key stays reserved for secondary-click. |
| **Zoom-to-cursor** | **Scroll / two-finger swipe** (unchanged) | Wheel/trackpad zoom-to-cursor already works without a middle button — identical to desktop. |
| **Roll** (Alt + MMB) | **⌃ Control + ⌥ Option + left-drag** | Control is acceptable here because it is combined with Option + drag (a drag gesture, not a click), so it does not collide with Control-**click** secondary-click. |
| **Dolly** (Shift + MMB) | **⇧ Shift + ⌥ Option + left-drag** | Shift layers cleanly onto the emulated MMB exactly as Shift+MMB does on desktop. |
| **Rotate about entity** | **Option + left-click an entity, then Option + left-drag** | Reproduces "middle-click-entity then middle-drag rotates about it" via the emulated MMB. |

> The modifier chord above (Option + left-drag as the MMB emulator) is the value
> the user **confirms at the blocking Mac-hardware checkpoint** (Plan 02 Task 3).
> If real-hardware testing finds a more ergonomic chord, update this table and
> the chord becomes the new documented default.

---

## 2. One-click alternative — `Gui::GestureNavigationStyle` (native trackpad)

For users who prefer native macOS trackpad gestures over an emulated middle
button, FreeWorks exposes the upstream **`Gui::GestureNavigationStyle`** as a
one-click alternative. It is already registered upstream and selectable without
any FreeWorks-specific code.

**Where the user toggles it** (either path, no restart required):

- **Preferences:** `FreeCAD → Preferences → Display → Navigation → 3D Navigation`
  → choose **"Gesture"**. This writes the same `NavigationStyle` key that
  `FwNavigationDefault` defaults, so the user's choice is then preserved (the
  FreeWorks default never clobbers an explicit selection — note A2).
- **Viewport context menu:** right-click the 3D view → **Navigation styles** →
  **Gesture**.

Selecting Gesture makes `FwMacNavProfile` informational only; the active style is
whatever `NavigationStyle` holds.

---

## 3. Full mapping table — trackpad and 2-button-mouse Macs

| Action | Modifier-emulated MMB (default) | `Gui::GestureNavigationStyle` (alternative) |
|--------|----------------------------------|----------------------------------------------|
| **Rotate** | ⌥ Option + left-drag | One-finger drag on empty space (tap-and-drag) |
| **Pan** | ⌥ Option + ⌘ Command + left-drag | Two-finger drag |
| **Zoom (to cursor)** | Scroll / two-finger swipe | Pinch, or two-finger swipe |
| **Roll** | ⌃ Control + ⌥ Option + left-drag | ⌥ Option + rotate gesture (where supported) |
| **Dolly** | ⇧ Shift + ⌥ Option + left-drag | ⇧ Shift + two-finger drag |
| **Rotate about entity** | Option + left-click entity, then Option + left-drag | Tap entity, then one-finger drag |
| **Select** | Left-click / tap | Left-click / tap |
| **Context menu** | ⌃ Control + click, or right-click (2-button mouse) | ⌃ Control + click, or right-click |

For a **2-button mouse on macOS** (no middle button) the emulated-MMB column
applies unchanged; the right button gives the normal context menu and the scroll
wheel gives zoom-to-cursor.

---

## 4. Mac-hardware spike checklist

These items **cannot be validated in CI** and must be checked on real Apple
hardware (both a trackpad-only MacBook and a Mac with a 2-button mouse). This is
the agenda for the blocking checkpoint (Plan 02 Task 3).

- [ ] **Rotate feel** — Option + left-drag rotates smoothly with reference-CAD
      parity; no stutter, no accidental selection start, correct rotation pivot.
- [ ] **Pan feel** — Option + Command + left-drag pans without triggering window
      moves, Mission Control, or app switching.
- [ ] **Zoom-to-cursor** — two-finger swipe / scroll zooms toward the cursor (not
      screen center); direction matches the user's "natural scrolling" setting
      expectation, or is correctly inverted via the existing `InvertZoom` pref.
- [ ] **Roll** — Control + Option + left-drag rolls and does **not** fire a
      secondary (right) click; confirm Control-**click** still opens the context
      menu (no regression).
- [ ] **Dolly** — Shift + Option + left-drag dollies as expected.
- [ ] **Rotate-about-entity** — Option-click an entity then Option-drag rotates
      about that entity's point.
- [ ] **Gesture conflicts** — verify no collision with macOS system gestures:
      Mission Control (three/four-finger swipe), Launchpad (pinch), App Exposé,
      Notification Center (two-finger from right edge), and Spaces switching.
- [ ] **Modifier ergonomics** — the Option-based chords are comfortable for a
      sustained modeling session on a trackpad; document fatigue or reach issues
      and propose an alternate chord if needed.
- [ ] **Gesture-style alternative** — switch to `Gui::GestureNavigationStyle` via
      both documented toggle paths; confirm native trackpad rotate/pan/zoom work
      and that the choice persists (FreeWorks default does not overwrite it).
- [ ] **No-clobber** — pick a non-default nav style, relaunch + reactivate
      FreeWorks, confirm the chosen style is preserved (note A2).
- [ ] **2-button mouse path** — repeat the rotate/pan/zoom/roll/dolly checks with
      a 2-button USB mouse plugged into the Mac.

Record results (pass / fail / chord change) against each item; any chord change
is reflected back into §1 and §3 before the checkpoint is approved.

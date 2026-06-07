// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeWorks contributors                            *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <App/Application.h>
#include <Base/Parameter.h>

#include "FwNavigationDefault.h"

using namespace FreeWorksGui;

void FwNavigationDefault::applyDefault()
{
    // The navigation style lives in the same upstream preference group the core
    // viewport reads (analog: src/Gui/View3DSettings.cpp NavigationStyle branch,
    // key "NavigationStyle", default CADNavigationStyle).
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");

    // Read the current value with an empty default so an absent key is
    // distinguishable from a deliberately-chosen one. Note A2: only apply the
    // FreeWorks default when the key is unset — never clobber a user's choice.
    const std::string current = hGrp->GetASCII("NavigationStyle", "");
    if (current.empty()) {
        // Reference-CAD mouse model: rotate = MMB drag, pan = Ctrl+MMB,
        // zoom = scroll-wheel zoom-to-cursor, roll = Alt+MMB, dolly = Shift+MMB,
        // middle-click-entity then middle-drag rotates about it. The style ships
        // and is registered upstream; this is a preference default, not new code.
        // The upstream nav-style type-name string below is the sole permitted
        // reference-CAD identifier in the FreeWorks module (D-03 upstream-type
        // exception); the leak grep allow-lists exactly this token.
        hGrp->SetASCII("NavigationStyle", "Gui::SolidWorksNavigationStyle");  // SW-FORK HOOK
    }

#if defined(__APPLE__)
    // macOS lacks a default 3-button mouse, and the reference-CAD style has no
    // built-in middle-button emulation. Keep the same NavigationStyle default
    // above (so button semantics are identical to Windows/Linux) and additionally
    // select the modifier-emulated-MMB substitute profile. This is recorded in a
    // FreeWorks-namespaced marker so the substitute is observable/round-trippable
    // and so MACOS_NAV_PROFILE.md is the single source of truth for the mapping.
    // The native-trackpad alternative ("Gui::GestureNavigationStyle") remains one
    // click away via the standard navigation-style selector. Only set the marker
    // when unset, mirroring the no-clobber discipline above.
    const std::string macProfile = hGrp->GetASCII("FwMacNavProfile", "");
    if (macProfile.empty()) {
        // Values: "ModifierEmulatedMMB" (default) | "GestureNavigationStyle".
        // See src/Gui/FreeWorks/MACOS_NAV_PROFILE.md for the full chord mapping
        // and the Mac-hardware spike checklist.
        hGrp->SetASCII("FwMacNavProfile", "ModifierEmulatedMMB");  // SW-FORK HOOK
    }
#endif
}

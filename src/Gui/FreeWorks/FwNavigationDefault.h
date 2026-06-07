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

#pragma once

#include "PreCompiled.h"

namespace FreeWorksGui
{

/**
 * Applies the FreeWorks default 3D-viewport navigation profile.
 *
 * FreeWorks navigates with reference-CAD mouse conventions out of the box:
 * rotate = MMB drag, pan = Ctrl+MMB, zoom = scroll-wheel zoom-to-cursor,
 * roll = Alt+MMB, dolly = Shift+MMB, and middle-clicking an entity then
 * middle-dragging rotates about it. This is NOT new navigation code: the
 * navigation style already ships and is registered upstream. NAV-01 is a
 * preference default only — the existing "NavigationStyle" preference key is
 * defaulted to the upstream type-name string when (and only when) the user has
 * not already chosen a navigation style.
 *
 * On macOS, which lacks a default 3-button mouse, the same default is applied
 * together with a modifier-emulated middle-button profile that preserves the
 * button semantics above. A native-trackpad alternative remains one click away.
 * See MACOS_NAV_PROFILE.md for the full mapping and the Mac-hardware spike.
 */
class FwNavigationDefault
{
public:
    /**
     * Default the "NavigationStyle" preference key to the reference-CAD style
     * when it is unset. An existing user-chosen value is never overwritten
     * (Runtime State Inventory note A2). Idempotent and safe to call on every
     * workbench activation.
     */
    static void applyDefault();
};

}  // namespace FreeWorksGui

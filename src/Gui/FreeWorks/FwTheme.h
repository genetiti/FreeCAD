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
 * Minimal theme hook for the FreeWorks mode.
 *
 * This is intentionally a no-op stub at the Walking Skeleton stage. The full
 * reference-CAD-style visual theme (recreated look-alike icons, QSS colour
 * scheme, fonts and layout polish) is owned by Phase 7. The hook exists now only
 * so the activation seam has a stable place to call into once theming lands.
 */
class FreeWorksGuiExport FwTheme
{
public:
    /// Apply the FreeWorks theme. No-op placeholder; full QSS/SVG theme is Phase 7.
    static void apply();
};

}  // namespace FreeWorksGui

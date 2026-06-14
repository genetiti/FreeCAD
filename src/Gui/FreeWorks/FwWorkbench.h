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

#include <Gui/Workbench.h>

#include "PreCompiled.h"

namespace FreeWorksGui
{

/**
 * The FreeWorks workbench.
 *
 * Subclasses Gui::StdWorkbench (not the bare Gui::Workbench) so the future
 * FreeWorks ribbon inherits the standard command set. On activation it mounts a
 * coherent reference-CAD-style dock shell via FwLayout::install(). The permanent
 * Fw_* dock names registered in setupDockWindows() are the seam onto which later
 * phases (FeatureManager tree, PropertyManager, Task Pane) swap real content
 * without re-layout.
 *
 * The class carries no trademarked identifier (naming decision D-03); only the
 * label "FreeWorks" (D-01) and the Fw / FreeWorksGui naming (D-02) are used.
 */
// Compiled directly into FreeCADGui and instantiated only through FreeCAD's type
// system, so no cross-library export visibility is required on this class.
class FwWorkbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    FwWorkbench();
    ~FwWorkbench() override;

    /** Install the FreeWorks dock shell, mount the ribbon + hide stock chrome,
     * then run the standard activation. */
    void activated() override;

    /** Restore the stock menu bar + toolbars and unmount the ribbon so other
     * workbenches keep their chrome (reversible, FreeWorks-scoped — D-11). */
    void deactivated() override;

protected:
    /** Defer to the StdWorkbench menus (the FreeWorks ribbon arrives in Phase 2). */
    Gui::MenuItem* setupMenuBar() const override;
    /** Defer to the StdWorkbench toolbars (the FreeWorks ribbon arrives in Phase 2). */
    Gui::ToolBarItem* setupToolBars() const override;
    /** Defer to the StdWorkbench command bars (placeholder for Phase 2). */
    Gui::ToolBarItem* setupCommandBars() const override;
    /** Register the permanent Fw_* dock names; leave the top toolbar area free. */
    Gui::DockWindowItems* setupDockWindows() const override;
};

}  // namespace FreeWorksGui

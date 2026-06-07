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

#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>

#include "FwWorkbench.h"
#include "FwLayout.h"

using namespace FreeWorksGui;

/// Register the FreeWorks workbench in FreeCAD's type system, deriving from the
/// standard workbench so the Std command set is inherited.
TYPESYSTEM_SOURCE(FreeWorksGui::FwWorkbench, Gui::StdWorkbench)  // NOLINT

FwWorkbench::FwWorkbench() = default;

FwWorkbench::~FwWorkbench() = default;

void FwWorkbench::activated()
{
    // Mount the coherent FreeWorks dock geometry, then run the standard
    // activation (which sets up menus, toolbars and dock windows).
    FwLayout::install(Gui::getMainWindow());
    StdWorkbench::activated();
}

Gui::MenuItem* FwWorkbench::setupMenuBar() const
{
    // Phase 2 (the FreeWorks ribbon) replaces this. For now reuse the standard
    // menu bar so the shell stays usable.
    return StdWorkbench::setupMenuBar();
}

Gui::ToolBarItem* FwWorkbench::setupToolBars() const
{
    // Phase 2 (the FreeWorks ribbon) replaces this; defer to the standard set.
    return StdWorkbench::setupToolBars();
}

Gui::ToolBarItem* FwWorkbench::setupCommandBars() const
{
    // Placeholder for Phase 2; defer to the standard command bars.
    return StdWorkbench::setupCommandBars();
}

Gui::DockWindowItems* FwWorkbench::setupDockWindows() const
{
    auto* root = new Gui::DockWindowItems();

    // PERMANENT FreeWorks dock NAMES (OQ-2): later phases swap the content behind
    // these names, never the names themselves. The FeatureManager and
    // PropertyManager live on the left like the reference CAD UI; the Task Pane is
    // reserved on the right. The TOP toolbar area is deliberately left free for the
    // Phase 2 ribbon. The upstream Std_* dock blocks are NOT touched.
    root->addDockWidget("Fw_FeatureManager",
                        Qt::LeftDockWidgetArea,
                        Gui::DockWindowOption::Visible);
    root->addDockWidget("Fw_PropertyManager",
                        Qt::LeftDockWidgetArea,
                        Gui::DockWindowOption::Visible);
    root->addDockWidget("Fw_TaskPane",
                        Qt::RightDockWidgetArea,
                        Gui::DockWindowOption::VisibleTabbed);

    return root;
}

# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeWorks contributors                             *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Registration of the FreeWorks mode workbench in FreeCAD's selector."""

import FreeCAD as App
import FreeCADGui as Gui


class FreeWorksWorkbench(Gui.Workbench):
    """FreeWorks mode workbench.

    Registers the C++ FreeWorksGui::FwWorkbench (compiled into FreeCADGui) with
    FreeCAD's workbench selector. The label is "FreeWorks" (decision D-01); no
    trademarked wordmark appears in any user-facing string (decision D-03).
    """

    def __init__(self):
        # The workbench icon is recreated-original art bundled in the FreeWorks
        # Qt resource (its provenance ledger row is added in Plan 04).
        self.__class__.Icon = ":/FreeWorks/icons/FreeWorksWorkbench.svg"
        self.__class__.MenuText = "FreeWorks"
        self.__class__.ToolTip = "FreeWorks mode"

    def GetClassName(self):
        # Maps to the C++ workbench class registered via the FreeCAD type system.
        return "FreeWorksGui::FwWorkbench"


Gui.addWorkbench(FreeWorksWorkbench())

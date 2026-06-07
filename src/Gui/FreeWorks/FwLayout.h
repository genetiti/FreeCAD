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

class QMainWindow;

namespace FreeWorksGui
{

/**
 * Installer for the FreeWorks dock shell.
 *
 * install() backs each permanent Fw_* dock name with a labeled placeholder
 * widget and arranges the docks into the coherent reference-CAD left/right geometry
 * using only the public getMainWindow() getter and public Qt dock APIs. It never
 * edits MainWindow.cpp and pulls in no App-layer header (observe-the-DOM).
 */
class FreeWorksGuiExport FwLayout
{
public:
    /// Mount the FreeWorks placeholder dock shell into the given main window.
    static void install(QMainWindow* mainWindow);
};

}  // namespace FreeWorksGui

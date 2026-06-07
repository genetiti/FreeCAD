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

#include <FCConfig.h>
#include <FCGlobal.h>

// The FreeWorks module compiles directly into the FreeCADGui shared library
// (its sources are added to the FreeCADGui target by src/Gui/FreeWorks/CMakeLists.txt).
// Therefore its exported symbols use the FreeCADGui export visibility. The
// FreeWorksGuiExport alias keeps the module self-describing while remaining
// part of FreeCADGui's export set.
#ifndef FreeWorksGuiExport
#  define FreeWorksGuiExport GuiExport
#endif

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// STL
#include <string>

// Qt Toolkit
#include <Gui/QtAll.h>

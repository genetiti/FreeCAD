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

#include <string>
#include <vector>

#include <QString>
#include <QTabWidget>

namespace FreeWorksGui
{

/**
 * Minimal native ribbon shell: a QTabWidget whose pages host QToolBar panels of
 * large icon-over-label buttons built from real command IDs.
 *
 * This is the Wave-0 spike surface only: it proves the command -> large labeled
 * button primitive and the native split-button (flyout) seam on native Qt 6.8,
 * so the native-vs-SARibbon decision can be made before the full build. The
 * curated declarative map, panel titles, auto-derive, unique panel objectNames,
 * overflow and persistence are Plan 02-02+; keep this minimal but real.
 *
 * Compiled directly into FreeCADGui and used in-process, so no cross-library
 * export macro is required (mirrors FwWorkbench).
 */
class FwRibbon: public QTabWidget
{
    Q_OBJECT

public:
    explicit FwRibbon(QWidget* parent = nullptr);

    /// Number of tab pages currently built.
    int tabCount() const;

    /**
     * Build one tab page named @p tabName hosting a QToolBar panel with one large
     * icon-over-label button per resolvable command ID in @p ids. An ID that
     * Gui::CommandManager::getCommandByName() returns nullptr for is silently
     * skipped (D-08 omit-missing). A *_Comp* group ID flows through the same
     * Gui::Command::addTo() path and yields a native split-button (MenuButtonPopup)
     * with no hand-rolled QMenu.
     */
    void addTabFromCommandIds(const QString& tabName, const std::vector<std::string>& ids);
};

}  // namespace FreeWorksGui

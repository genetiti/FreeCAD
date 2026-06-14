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

#include <QStringList>

namespace FreeWorksGui
{

/**
 * Installer for the FreeWorks dock shell + ribbon command surface.
 *
 * install() backs each permanent Fw_* dock name with a uniquely-named, titled
 * placeholder widget via the public Gui::DockWindowManager singleton and public
 * Qt dock APIs. It never edits MainWindow.cpp and pulls in no App-layer header
 * (observe-the-DOM). It is called from FwWorkbench::setupDockWindows() so the
 * widgets exist before the framework's DockWindowManager::setup() consumes them.
 *
 * mountRibbon()/unmountRibbon() and hideStockChrome()/restoreStockChrome() wire
 * the FwRibbon into the live main window as the single command surface (D-11):
 * the ribbon is WRAPPED in a real Gui::ToolBar/QToolBar (objectName
 * Fw_RibbonToolBar) added via Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea,
 * wrapper). A real QToolBar is a genuine QMainWindow::saveState() participant — a
 * bare QTabWidget child of a ToolBarAreaWidget is NOT (REVIEW concerns 3 & 4).
 * All operations go through public Gui singletons; MainWindow.cpp is never edited.
 */
class FreeWorksGuiExport FwLayout
{
public:
    /// Register the FreeWorks placeholder docks under their permanent Fw_* names.
    static void install();

    /**
     * Build the FwRibbon for the active workbench, wrap it in a real QToolBar
     * (objectName @c Fw_RibbonToolBar, non-movable/non-floatable) and add it via
     * @c Gui::getMainWindow()->addToolBar(Qt::TopToolBarArea, wrapper). Idempotent:
     * if a toolbar named @c Fw_RibbonToolBar already exists on the main window the
     * call reuses it (no duplicate wrapper on re-activation). No-op if the main
     * window is not yet available.
     */
    static void mountRibbon();

    /// Remove the Fw_RibbonToolBar wrapper from the main window (predictable
    /// teardown). No-op if nothing is mounted.
    static void unmountRibbon();

    /**
     * Hide FreeCAD's stock menu bar + toolbars so the ribbon is the sole command
     * surface (D-11). Reversible and FreeWorks-scoped: the set of toolbar names
     * hidden and the menu-bar visibility + (macOS) native-menu setting are
     * SNAPSHOTTED so restoreStockChrome() restores EXACTLY the pre-hide state.
     */
    static void hideStockChrome();

    /// Restore the stock menu bar + toolbars to the snapshot captured by
    /// hideStockChrome() (Pitfall 3 + macOS native-menu MEDIUM). No-op if no
    /// snapshot is held.
    static void restoreStockChrome();

private:
    /// Stable objectName of the real QToolBar that wraps the ribbon.
    static const char* ribbonToolBarObjectName();

    // --- chrome snapshot (FreeWorks-scoped, reversible) ---------------------
    /// Toolbar names hidden by the last hideStockChrome(); restored verbatim.
    static QStringList s_hiddenToolBars;
    /// Whether a chrome snapshot is currently held (guards double hide/restore).
    static bool s_chromeHidden;
    /// menuBar()->isVisible() captured before hide.
    static bool s_menuBarWasVisible;
    /// menuBar()->isNativeMenuBar() captured before hide (macOS-relevant).
    static bool s_menuBarWasNative;
};

}  // namespace FreeWorksGui

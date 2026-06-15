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

#include <memory>

#include <QStringList>

namespace FreeWorksGui
{

class FwRibbonContext;
class FwPropertyReveal;

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
    /// teardown), AND release the FwRibbonContext switcher so its edit-signal
    /// subscriptions do not leak past FreeWorks mode (RESEARCH Pattern 3 / threat
    /// T-02-09). No-op if nothing is mounted.
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

    /**
     * Left-dock the managed PropertyManager dock — the existing "Tasks" TaskView
     * container (D-03 reuse-and-rehost) — into Qt::LeftDockWidgetArea (PROP-01).
     *
     * The host is resolved ROBUSTLY from @c Gui::Control().taskPanel() by walking UP to
     * its parent QDockWidget, then branching on
     * @c getMainWindow()->dockWidgetArea(dock) (R2-F1 — @c addDockWindow CANNOT move an
     * already-docked panel, DockWindowManager.cpp:256-258):
     *   - already left  -> no-op (idempotent);
     *   - non-left dock -> re-dock the EXISTING container left via
     *     @c Gui::getMainWindow()->addDockWidget(Qt::LeftDockWidgetArea, dock)
     *     (the came-from-right-dock case);
     *   - never-docked  -> resolve the registered Tasks TaskView via
     *     @c findRegisteredDockWindow("Std_TaskView") and CREATE the dock left via
     *     @c addDockWindow("Tasks", taskView, Qt::LeftDockWidgetArea).
     * The "Tasks" container objectName + inner-widget objectName are preserved so
     * @c getDockWindow("Tasks") / @c Control().taskPanel() keep resolving (R2-F3) and
     * a saved layout round-trips.
     *
     * The reveal + deferred-cancellable-teardown consumer (s_propertyReveal, owned
     * SEPARATELY from s_ribbonContext) is (re)bound here; binding/re-mounting CANCELS
     * any pending teardown scheduled by a prior unmountPropertyManager(). No-op if the
     * main window / DockWindowManager are not yet available.
     */
    static void mountPropertyManager();

    /**
     * SCHEDULE the true-exit teardown of the PropertyManager chrome (the A4
     * deferred-cancellable policy) — does NOT tear down inline. Because an edit-time
     * workbench switch fires deactivated() mid-edit BEFORE signalInEdit arms anything
     * (R3-ROOT/R4-BLOCKER), a synchronous teardown (or one gated on a pre-checked flag
     * still false at that instant) would move the "Tasks" dock back right (R2-F2) and
     * strip the chrome. Instead this posts a cancellable QTimer::singleShot(0) teardown
     * and stores its pending handle on s_propertyReveal; the teardown only runs on the
     * NEXT event-loop turn if signalInEdit / re-mount did not cancel it first. No-op if
     * nothing is mounted.
     */
    static void unmountPropertyManager();

private:
    /// Stable objectName of the real QToolBar that wraps the ribbon.
    static const char* ribbonToolBarObjectName();

    // --- contextual tab switcher (RIBBON-02, Plan 02-04) --------------------
    /// Bind (or rebind) the single FwRibbonContext switcher to @p ribbon and start
    /// its edit-signal subscription. Owns exactly ONE context for the FreeWorks-mode
    /// lifetime, so re-activation refreshes the binding instead of adding a duplicate
    /// subscription (pairs with the idempotent mount). Called from mountRibbon().
    static void bindRibbonContext(class FwRibbon* ribbon);

    /// The single context switcher bound to the mounted ribbon. Held for the
    /// FreeWorks-mode lifetime and reset on unmountRibbon() so its scoped
    /// fastsignals connections are released on teardown (no signal fires into a
    /// torn-down ribbon — REVIEW MEDIUM).
    static std::unique_ptr<FwRibbonContext> s_ribbonContext;

    // --- PropertyManager reveal + deferred-cancellable teardown (PROP-01) -----
    /// The single reveal/teardown consumer for the left-docked "Tasks"
    /// PropertyManager. Owned SEPARATELY from s_ribbonContext (which is reset on the
    /// transient deactivated() at unmountRibbon()) so it SURVIVES the edit-time
    /// workbench switch (A4 cross-phase observation). It owns the signalInEdit /
    /// signalResetEdit subscriptions AND the pending-teardown QTimer handle:
    /// signalInEdit cancels the pending teardown and re-asserts the left placement;
    /// unmountPropertyManager() only SCHEDULES a cancellable teardown through it. Reset
    /// only on the deferred true exit (so subscriptions drop before teardown).
    static std::unique_ptr<FwPropertyReveal> s_propertyReveal;

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

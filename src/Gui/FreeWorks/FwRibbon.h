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

#include <list>
#include <string>
#include <utility>
#include <vector>

#include <QString>
#include <QTabWidget>

class QToolBar;
class QToolButton;
class QWidget;

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

    /**
     * Build the full curated ribbon from FwRibbonMap (D-05/D-06): group the rows by
     * tab (first-seen order) then by panel, producing one tab page per tab and one
     * @c QToolBar panel per panel title, with one large icon-over-label button per
     * resolved command ID via Gui::Command::addTo(). A *_Comp* group ID yields a
     * native split-button (MenuButtonPopup) through the same addTo() path. Curated
     * IDs that do not resolve are silently skipped (D-08 omit-missing). If the build
     * yields zero tabs the empty-state widget is shown instead.
     *
     * Clears any previously built tabs first so it is safe to call more than once.
     */
    void buildFromCuratedMap();

    /**
     * Auto-derive a ribbon for an uncurated workbench (D-07) from the LIVE,
     * value-type list returned by Gui::Workbench::getToolbarItems() — each pair is
     * (toolbar/group name, command IDs). One panel @c QToolBar is created per group
     * and one button per resolved command ID; the literal sentinel @c "Separator"
     * (Workbench.cpp toolbar tree) inserts a panel separator instead of a button.
     *
     * This consumes the stable, copied getToolbarItems() value list — NOT the
     * transient setupToolBars() ToolBarItem* tree, which Workbench::activate()
     * consumes and deletes (REVIEW concern 5). All derived panels live under a
     * single "Tools" tab. Yields the empty-state widget when no group is derivable.
     */
    void buildAutoDerived(
        const std::list<std::pair<std::string, std::list<std::string>>>& toolbarGroups);

    /**
     * Select the tab whose label equals @p tabName (no-op if absent). Exposed now so
     * Plan 04's context switcher can drive the active tab on enter/leave edit.
     */
    void setCurrentTab(const QString& tabName);

    /**
     * Persist the SELECTED TAB index to the FreeWorks ribbon ParameterGrp key (D-14).
     *
     * QMainWindow::saveState() round-trips the wrapper QToolBar (Fw_RibbonToolBar)
     * but does NOT cover a QTabWidget's selected tab (REVIEW concern 4), so the
     * current tab is persisted SEPARATELY here via ParameterGrp::SetInt. Called on
     * tab change and at teardown.
     */
    void saveTabState() const;

    /**
     * Restore the selected tab index from the FreeWorks ribbon ParameterGrp key
     * (D-14). Clamped to the valid tab range; a stale/out-of-range index is ignored.
     * Called after a build.
     */
    void restoreTabState();

private:
    /// Lazily (re)build the "More commands…" overflow menu from the CommandManager.
    void rebuildOverflowMenu();

    /// Install the pinned "More commands…" overflow QToolButton as the tab-bar
    /// corner widget (D-12 escape hatch). Idempotent.
    void installOverflowButton();

    /// The pinned "More commands…" overflow button (tab-bar corner widget, D-12).
    QToolButton* m_overflowButton = nullptr;

    /// When true, currentChanged-driven saveTabState() is suppressed: a rebuild
    /// transiently clears+repopulates tabs, and persisting those transient indices
    /// would clobber the user's stored selection before restoreTabState() runs.
    bool m_suppressTabStateSave = false;

    /// Find an existing tab page by its label, or create+append a new one.
    QWidget* tabPageForName(const QString& tabName);

    /// Find an existing panel QToolBar on @p page by panel title, or create one with
    /// the UI-SPEC metrics and the unique objectName Fw_RibbonPanel_<Tab>_<Panel>.
    QToolBar* panelForName(QWidget* page, const QString& tabName, const QString& panelName);

    /// Remove all tab pages (so a rebuild starts clean).
    void clearTabs();

    /// Show the UI-SPEC empty-state page when a build produced no panels.
    void showEmptyState();
};

}  // namespace FreeWorksGui

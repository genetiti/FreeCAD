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

// Include the Qt widget headers this TU uses unconditionally. Do NOT rely on the
// PCH (which transitively pulls in Gui/QtAll.h) to supply them: a non-PCH build
// path, or a future trimming of QtAll.h, would otherwise break this TU (WR-04).
#include <QLabel>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QSize>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <App/Application.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Command.h>

#include "FwRibbon.h"
#include "FwRibbonMap.h"

using namespace FreeWorksGui;

namespace
{
// FreeCAD's toolbar tree uses the literal string "Separator" as the separator
// sentinel inside a toolbar group (Workbench.cpp), NOT an empty command id. The
// auto-derive walk must treat it as a separator, not a (failed) command lookup.
constexpr const char* kSeparatorSentinel = "Separator";

// objectName prefix for every persisted panel QToolBar (Pitfall 4 / D-14 prep):
// QMainWindow::saveState()/restoreState() key on objectName, so each panel needs a
// stable, unique one of the form Fw_RibbonPanel_<Tab>_<Panel>.
QString panelObjectName(const QString& tabName, const QString& panelName)
{
    return QStringLiteral("Fw_RibbonPanel_%1_%2").arg(tabName, panelName);
}

// D-14 separate tab-state store: QMainWindow::saveState() round-trips the wrapper
// QToolBar but NOT a QTabWidget's selected tab (REVIEW concern 4), so the selected
// tab is persisted here under a small FreeWorks ParameterGrp key — NOT a bespoke
// binary format (Don't Hand-Roll). Mirrors how FwNavigationDefault reads a prefs
// group.
constexpr const char* kRibbonParamPath =
    "User parameter:BaseApp/Preferences/FreeWorks/Ribbon";
constexpr const char* kCurrentTabKey = "currentTab";
}  // namespace

FwRibbon::FwRibbon(QWidget* parent)
    : QTabWidget(parent)
{
    // Unique objectName for QMainWindow state persistence (Pitfall 4 / D-14 prep).
    setObjectName(QStringLiteral("Fw_Ribbon"));

    // D-12 escape hatch: a single pinned "More commands…" overflow reaching the
    // uncurated command set, so no command is stranded when the stock chrome is
    // hidden. (The baseline guarantee is that command QAction shortcuts survive
    // menuBar()->hide() — see installOverflowButton().)
    installOverflowButton();

    // D-14: persist the selected tab separately from the QMainWindow toolbar state.
    connect(this, &QTabWidget::currentChanged, this, [this](int) {
        saveTabState();
    });
}

int FwRibbon::tabCount() const
{
    return count();
}

void FwRibbon::addTabFromCommandIds(const QString& tabName, const std::vector<std::string>& ids)
{
    // The tab page hosts a single QToolBar panel for this spike. Plan 02-02 splits
    // a tab into multiple titled panels with unique objectNames; here one panel is
    // enough to prove the command -> large labeled button primitive.
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* panel = new QToolBar(page);
    // UI-SPEC § Spacing Scale: large icon over label (32px). Color comes from the
    // active QPalette/QStyle only — no inline style sheets, no hard-coded hex;
    // Phase 7 owns theming (UI-SPEC § Color).
    panel->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    panel->setIconSize(QSize(32, 32));
    layout->addWidget(panel);
    layout->addStretch();

    Gui::CommandManager& manager = Gui::Application::Instance->commandManager();
    for (const std::string& id : ids) {
        Gui::Command* cmd = manager.getCommandByName(id.c_str());
        if (cmd == nullptr) {
            // D-08 omit-missing: an unresolved ID is silently skipped — no crash,
            // no empty button.
            continue;
        }
        // The single command -> button seam. A *_Comp* group command routes through
        // the same addTo() and yields a native split-button (MenuButtonPopup) via
        // ActionGroup::addTo() (Action.cpp:472-485) — no hand-rolled flyout here.
        cmd->addTo(panel);
    }

    addTab(page, tabName);
}

QWidget* FwRibbon::tabPageForName(const QString& tabName)
{
    for (int i = 0; i < count(); ++i) {
        if (tabText(i) == tabName) {
            return widget(i);
        }
    }

    // A fresh tab page: a vertical layout that stacks its panels left-to-right via
    // an inner row, with a trailing stretch so panels hug the top-left (ribbon
    // body height is owned by the panels' fixed icon metric, not the page).
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    addTab(page, tabName);
    return page;
}

QToolBar*
FwRibbon::panelForName(QWidget* page, const QString& tabName, const QString& panelName)
{
    const QString objName = panelObjectName(tabName, panelName);

    // Reuse an existing panel with this title on this page so repeated rows of the
    // same (tab,panel) accumulate buttons into one QToolBar.
    const QList<QToolBar*> existing = page->findChildren<QToolBar*>();
    for (QToolBar* bar : existing) {
        if (bar->objectName() == objName) {
            return bar;
        }
    }

    auto* panel = new QToolBar(panelName, page);
    // UI-SPEC § Spacing/Typography: large 32px icon over a (word-wrapped, 2-line,
    // never-ellipsis) label. Color comes from the active QPalette/QStyle ONLY —
    // no inline style-sheet, no hard-coded hex; Phase 7 owns theming (UI-SPEC § Color).
    panel->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    panel->setIconSize(QSize(32, 32));
    panel->setMovable(false);
    panel->setFloatable(false);
    // Pitfall 4 / D-14: unique, stable objectName so saveState()/restoreState() key.
    panel->setObjectName(objName);

    // Insert just before the trailing stretch so panels pack left-to-right.
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());
    if (layout != nullptr) {
        layout->insertWidget(layout->count() - 1, panel);
    }
    return panel;
}

void FwRibbon::clearTabs()
{
    while (count() > 0) {
        QWidget* page = widget(0);
        removeTab(0);
        delete page;
    }
}

void FwRibbon::showEmptyState()
{
    clearTabs();

    // UI-SPEC § Copywriting empty-state copy. Should not be reached given the
    // auto-derive fallback, but a build that derived nothing must not be a blank tab.
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* heading = new QLabel(tr("No commands available for this workbench"), page);
    heading->setAlignment(Qt::AlignCenter);
    heading->setWordWrap(true);
    auto* body = new QLabel(
        tr("Switch to a modeling workbench, or use the menu/keyboard shortcuts to "
           "run commands."),
        page);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    layout->addStretch();
    layout->addWidget(heading);
    layout->addWidget(body);
    layout->addStretch();
    addTab(page, tr("Commands"));
}

void FwRibbon::buildFromCuratedMap()
{
    m_suppressTabStateSave = true;
    clearTabs();

    Gui::CommandManager& manager = Gui::Application::Instance->commandManager();

    // The rows are pre-ordered (Features -> Sketch -> Evaluate, panel + button order
    // by first-seen); grouping by (tab,panel) via find-or-create preserves it.
    for (const FwRibbonRow& row : FwRibbonMap::rows()) {
        Gui::Command* cmd = manager.getCommandByName(row.commandId);
        if (cmd == nullptr) {
            // D-08 omit-missing: an unresolved curated id is silently skipped at
            // runtime (the headless per-row test is what fails loudly on a typo).
            continue;
        }

        QWidget* page = tabPageForName(QString::fromUtf8(row.tab));
        QToolBar* panel =
            panelForName(page, QString::fromUtf8(row.tab), QString::fromUtf8(row.panel));

        // The single command -> button seam. A *_Comp* group command routes through
        // the same addTo() and yields a native split-button (MenuButtonPopup) via
        // ActionGroup::addTo() (Action.cpp) — no hand-rolled flyout here.
        cmd->addTo(panel);
    }

    if (count() == 0) {
        showEmptyState();
    }

    // D-14: re-apply the persisted selected tab after a (re)build, then re-enable
    // user-driven persistence.
    m_suppressTabStateSave = false;
    restoreTabState();
}

void FwRibbon::buildAutoDerived(
    const std::list<std::pair<std::string, std::list<std::string>>>& toolbarGroups)
{
    m_suppressTabStateSave = true;
    clearTabs();

    Gui::CommandManager& manager = Gui::Application::Instance->commandManager();

    // All auto-derived panels live under one synthetic tab — an uncurated workbench
    // has no curated tab taxonomy, so its toolbar groups become this tab's panels.
    const QString tabName = QStringLiteral("Tools");

    for (const auto& group : toolbarGroups) {
        const QString panelName = QString::fromStdString(group.first);
        QWidget* page = tabPageForName(tabName);
        QToolBar* panel = panelForName(page, tabName, panelName);

        for (const std::string& id : group.second) {
            if (id == kSeparatorSentinel) {
                // The "Separator" literal is a separator, NOT a command lookup
                // (Workbench.cpp toolbar tree) — REVIEW concern 5.
                panel->addSeparator();
                continue;
            }
            Gui::Command* cmd = manager.getCommandByName(id.c_str());
            if (cmd == nullptr) {
                continue;  // D-08 omit-missing.
            }
            cmd->addTo(panel);
        }
    }

    if (count() == 0) {
        showEmptyState();
    }

    // D-14: re-apply the persisted selected tab after a (re)build, then re-enable
    // user-driven persistence.
    m_suppressTabStateSave = false;
    restoreTabState();
}

void FwRibbon::setCurrentTab(const QString& tabName)
{
    for (int i = 0; i < count(); ++i) {
        if (tabText(i) == tabName) {
            setCurrentIndex(i);
            return;
        }
    }
    // Absent tab -> no-op (Plan 04's switcher tolerates a context with no matching tab).
}

void FwRibbon::installOverflowButton()
{
    // Idempotent: only install once.
    if (m_overflowButton != nullptr) {
        return;
    }

    // D-12: a plain QTabWidget has no inherent right-edge slot, so the correct
    // placement for the pinned overflow affordance is the tab-bar CORNER WIDGET
    // (REVIEW MEDIUM — not an ad-hoc absolute position).
    m_overflowButton = new QToolButton(this);
    // UI-SPEC § Copywriting: the label is exactly "More commands…".
    m_overflowButton->setText(tr("More commands…"));
    m_overflowButton->setToolTip(tr("More commands…"));
    // UI-SPEC § Spacing: a small/secondary 16px icon (the menu indicator carries the
    // affordance; color/icon come from the active QStyle/QPalette — no style sheet).
    m_overflowButton->setIconSize(QSize(16, 16));
    m_overflowButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_overflowButton->setPopupMode(QToolButton::InstantPopup);

    auto* menu = new QMenu(m_overflowButton);
    m_overflowButton->setMenu(menu);
    // Build the menu LAZILY on show so it always reflects the currently registered
    // commands (a thin trigger of each command's existing QAction — no new backend).
    connect(menu, &QMenu::aboutToShow, this, &FwRibbon::rebuildOverflowMenu);

    setCornerWidget(m_overflowButton, Qt::TopRightCorner);
}

void FwRibbon::rebuildOverflowMenu()
{
    if (m_overflowButton == nullptr || m_overflowButton->menu() == nullptr) {
        return;
    }
    QMenu* menu = m_overflowButton->menu();
    menu->clear();

    if (Gui::Application::Instance == nullptr) {
        return;
    }
    Gui::CommandManager& manager = Gui::Application::Instance->commandManager();

    // Reach the uncurated command set (Pitfall 5 / D-12). Group getAllCommands() by
    // owning app module so the overflow is browsable; each entry is the command's
    // existing QAction via Gui::Command::addTo() (thin trigger — no new path, so a
    // destructive command keeps its existing downstream confirmation, T-02-DC).
    QMap<QString, QList<Gui::Command*>> byModule;
    for (Gui::Command* cmd : manager.getAllCommands()) {
        if (cmd == nullptr) {
            continue;
        }
        const char* mod = cmd->getAppModuleName();
        const QString moduleName = (mod != nullptr && *mod != '\0')
            ? QString::fromUtf8(mod)
            : tr("Standard");
        byModule[moduleName].append(cmd);
    }

    for (auto it = byModule.constBegin(); it != byModule.constEnd(); ++it) {
        auto* submenu = menu->addMenu(it.key());
        for (Gui::Command* cmd : it.value()) {
            // addTo() reuses the command's existing QAction (with its shortcut),
            // so the overflow is a discoverability surface, not a duplicate trigger.
            cmd->addTo(submenu);
        }
    }
}

void FwRibbon::saveTabState() const
{
    // Suppress the transient currentChanged storm during a rebuild (clear + repopulate)
    // so the user's stored selection is not clobbered before restoreTabState() runs.
    if (m_suppressTabStateSave) {
        return;
    }
    // D-14 separate tab-state store (REVIEW concern 4): persist the selected tab
    // index via ParameterGrp::SetInt — QMainWindow::saveState() does not cover it.
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(kRibbonParamPath);
    hGrp->SetInt(kCurrentTabKey, currentIndex());
}

void FwRibbon::restoreTabState()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(kRibbonParamPath);
    // Default -1 ("unset"); only apply a stored index that is in range for the
    // freshly built tab set (a stale/out-of-range index is ignored).
    const int saved = hGrp->GetInt(kCurrentTabKey, -1);
    if (saved >= 0 && saved < count()) {
        setCurrentIndex(saved);
    }
}

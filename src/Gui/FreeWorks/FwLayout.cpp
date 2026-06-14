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
#include <QMenuBar>
#include <QToolBar>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Workbench.h>
#include <Gui/WorkbenchManager.h>

#include "FwLayout.h"
#include "FwRibbon.h"
#include "FwRibbonContext.h"

using namespace FreeWorksGui;

// --- chrome snapshot statics (FreeWorks-scoped, reversible) -----------------
QStringList FwLayout::s_hiddenToolBars;
bool FwLayout::s_chromeHidden = false;
bool FwLayout::s_menuBarWasVisible = true;
bool FwLayout::s_menuBarWasNative = false;

// --- contextual tab switcher (RIBBON-02, Plan 02-04) ------------------------
// Exactly ONE context switcher for the FreeWorks-mode lifetime. unique_ptr so its
// scoped fastsignals connections are released on reset() at teardown.
std::unique_ptr<FwRibbonContext> FwLayout::s_ribbonContext;

const char* FwLayout::ribbonToolBarObjectName()
{
    // Stable objectName so QMainWindow::saveState()/restoreState() serializes the
    // wrapper toolbar (REVIEW concern 4) and so mountRibbon() can find-or-reuse it
    // (idempotent re-activation — no duplicate ribbon).
    return "Fw_RibbonToolBar";
}

namespace
{

/// Build a labeled placeholder widget for a dock that real content replaces in a
/// later phase. The label states which phase delivers the real widget so the
/// Walking Skeleton shell is self-documenting.
///
/// objectName MUST be unique per dock: DockWindowManager::addDockWindow() copies
/// the widget's objectName onto the QDockWidget, and QMainWindow saveState()/
/// restoreState() key the layout by objectName and require it to be unique — a
/// shared objectName produces "'objectName' not unique" warnings and a layout
/// that cannot round-trip (CR-02). The windowTitle is propagated to the dock
/// title bar.
QWidget* makePlaceholder(const char* objectName, const QString& title, const QString& text)
{
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setObjectName(QString::fromUtf8(objectName));
    label->setWindowTitle(title);
    return label;
}

/// Register a placeholder under a permanent Fw_* dock name if nothing is yet
/// registered there. Registering under the permanent name lets later phases swap
/// the content without re-laying-out the shell. The dock name doubles as the
/// widget objectName so each QDockWidget gets a unique, restorable identity.
void ensureDock(Gui::DockWindowManager* manager,
                const char* name,
                const QString& title,
                const QString& label)
{
    if (manager->findRegisteredDockWindow(name) == nullptr) {
        manager->registerDockWindow(name, makePlaceholder(name, title, label));
    }
}

}  // namespace

void FwLayout::install()
{
    // Observe-the-DOM: operate only through the public DockWindowManager singleton;
    // never edit MainWindow.cpp and never include an App-layer header. (The main
    // window is reachable via Gui::getMainWindow() if a future revision needs it;
    // the manager singleton is sufficient for placeholder registration today, so
    // no QMainWindow handle is taken — WR-01.)
    auto* manager = Gui::DockWindowManager::instance();
    if (manager == nullptr) {
        return;
    }

    // Back each permanent Fw_* dock name with a labeled placeholder. The
    // FwWorkbench::setupDockWindows() override decides where each name docks
    // (FeatureManager + PropertyManager on the left, Task Pane reserved on the
    // right); here we only supply the content widgets. Each placeholder's
    // objectName is its dock name so the saved layout round-trips (CR-02).
    ensureDock(manager,
               "Fw_FeatureManager",
               QObject::tr("FeatureManager"),
               QObject::tr("FeatureManager (Phase 3)"));
    ensureDock(manager,
               "Fw_PropertyManager",
               QObject::tr("PropertyManager"),
               QObject::tr("PropertyManager (Phase 4)"));
    ensureDock(manager,
               "Fw_TaskPane",
               QObject::tr("Task Pane"),
               QObject::tr("Task Pane (Phase 7)"));
}

void FwLayout::mountRibbon()
{
    // Observe-the-DOM: reach the live main window through the public singleton.
    // Gui::getMainWindow() returns MainWindow::getInstance(); MainWindow IS-A
    // QMainWindow, so addToolBar()/toolBarArea()/saveState() are all available
    // (MainWindow.h:398). Never edit MainWindow.cpp.
    Gui::MainWindow* mw = Gui::getMainWindow();
    if (mw == nullptr) {
        return;
    }

    // Idempotent (REVIEW MEDIUM — no duplicate ribbon on reactivation): if the
    // wrapper toolbar already exists on the main window, reuse it. findChild is an
    // objectName lookup, so it matches the wrapper added on a prior activation.
    QToolBar* existing = mw->findChild<QToolBar*>(
        QString::fromUtf8(ribbonToolBarObjectName()));

    // Build the ribbon for the active workbench. Prefer the curated map (D-05/D-06);
    // if it derived nothing (no curated tab for this workbench) fall back to the live
    // value-type toolbar list of the active workbench (D-07). buildFromCuratedMap()
    // installs an empty-state tab when zero curated rows resolved, so we detect the
    // "nothing curated" case by the absence of a real panel QToolBar.
    // WR-02: hold the freshly built ribbon in a unique_ptr so a throw anywhere
    // between construction and the addWidget() ownership transfer cannot leak it
    // (the ribbon has no QObject parent until adopted). release() at each adopt site.
    auto ribbonOwner = std::make_unique<FwRibbon>();
    FwRibbon* ribbon = ribbonOwner.get();
    ribbon->buildFromCuratedMap();
    bool curatedHasPanels = false;
    for (int i = 0; i < ribbon->count(); ++i) {
        if (!ribbon->widget(i)->findChildren<QToolBar*>().isEmpty()) {
            curatedHasPanels = true;
            break;
        }
    }
    if (!curatedHasPanels) {
        Gui::Workbench* wb = Gui::WorkbenchManager::instance() != nullptr
            ? Gui::WorkbenchManager::instance()->active()
            : nullptr;
        if (wb != nullptr) {
            ribbon->buildAutoDerived(wb->getToolbarItems());
        }
    }

    if (existing != nullptr) {
        // Reuse the existing wrapper: swap in the freshly built ribbon as its only
        // widget so re-activation refreshes content without adding a 2nd toolbar.
        for (QObject* child : existing->children()) {
            if (auto* oldRibbon = qobject_cast<FwRibbon*>(child)) {
                oldRibbon->deleteLater();
            }
        }
        existing->clear();
        existing->addWidget(ribbonOwner.release());  // QToolBar takes ownership
        existing->show();
        // Rebind the single context switcher to the freshly built ribbon so the
        // edit-signal subscription drives the CURRENT ribbon (the prior ribbon was
        // scheduled for deletion above; QPointer would otherwise go null).
        bindRibbonContext(ribbon);
        return;
    }

    // WRAP the ribbon in a REAL Gui::ToolBar/QToolBar (ToolBarManager.h:124) so
    // QMainWindow owns a genuine state participant (REVIEW concerns 3 & 4). Do NOT
    // mount by fetching an "area widget" and adding the bare QTabWidget to it: the
    // area-lookup helper takes a widget and RETURNS the area containing it (it is
    // not a TopToolBarArea fetch), and an area-widget child QTabWidget would not
    // round-trip through saveState() (ToolBarAreaWidget.cpp:121). addToolBar() with
    // a real QToolBar is the only seam that persists.
    auto* wrapper = new Gui::ToolBar(mw);
    wrapper->setObjectName(QString::fromUtf8(ribbonToolBarObjectName()));
    wrapper->setWindowTitle(QObject::tr("FreeWorks Ribbon"));
    wrapper->setMovable(false);
    wrapper->setFloatable(false);
    wrapper->addWidget(ribbonOwner.release());  // QToolBar takes ownership

    mw->addToolBar(Qt::TopToolBarArea, wrapper);

    // Bind the contextual tab switcher (RIBBON-02) to the just-mounted ribbon and
    // start its edit-signal subscription so entering a sketch activates the Sketch
    // tab and leaving restores the prior tab (D-09/D-10).
    bindRibbonContext(ribbon);
}

void FwLayout::bindRibbonContext(FwRibbon* ribbon)
{
    // Own exactly ONE context for the FreeWorks-mode lifetime. Constructing it lazily
    // (and reusing it across re-activations) guarantees a single edit-signal
    // subscription — never a duplicate (pairs with the idempotent mount; threat
    // T-02-09). connect() is itself idempotent (it reassigns the scoped connections),
    // so calling it again after a rebind cannot stack subscriptions.
    if (!s_ribbonContext) {
        s_ribbonContext = std::make_unique<FwRibbonContext>();
    }
    s_ribbonContext->setRibbon(ribbon);  // held via QPointer
    s_ribbonContext->connect();
}

void FwLayout::unmountRibbon()
{
    // Release the context switcher FIRST so its scoped fastsignals connections are
    // dropped before the ribbon is torn down — no signal can fire into a
    // half-removed ribbon (REVIEW MEDIUM / threat T-02-09). Resetting the unique_ptr
    // runs ~FwRibbonContext() which disconnect()s the subscriptions.
    s_ribbonContext.reset();

    Gui::MainWindow* mw = Gui::getMainWindow();
    if (mw == nullptr) {
        return;
    }
    QToolBar* wrapper = mw->findChild<QToolBar*>(
        QString::fromUtf8(ribbonToolBarObjectName()));
    if (wrapper != nullptr) {
        // Predictable teardown: detach from the main window so other workbenches do
        // not inherit the ribbon, then schedule deletion of the wrapper + its ribbon.
        mw->removeToolBar(wrapper);
        wrapper->deleteLater();
    }
}

void FwLayout::hideStockChrome()
{
    // Reversible + FreeWorks-scoped (Pitfall 3). Guard against a double-hide that
    // would corrupt the snapshot (e.g. activated() called twice without a restore).
    if (s_chromeHidden) {
        return;
    }
    Gui::MainWindow* mw = Gui::getMainWindow();
    if (mw == nullptr) {
        return;
    }
    auto* tbm = Gui::ToolBarManager::getInstance();
    if (tbm == nullptr) {
        return;
    }

    // SNAPSHOT the toolbar names we are about to hide so restore is exact. Hide
    // every real toolbar EXCEPT our own ribbon wrapper. setState() keys by name
    // (ToolBarManager.cpp:1269), so we record names and replay them on restore.
    s_hiddenToolBars.clear();
    const QString ribbonName = QString::fromUtf8(ribbonToolBarObjectName());
    const QList<QToolBar*> bars = mw->findChildren<QToolBar*>();
    for (QToolBar* tb : bars) {
        const QString name = tb->objectName();
        if (name.isEmpty() || name == ribbonName) {
            continue;
        }
        s_hiddenToolBars.append(name);
    }
    if (!s_hiddenToolBars.isEmpty()) {
        tbm->setState(s_hiddenToolBars, Gui::ToolBarManager::State::ForceHidden);
    }

    // SNAPSHOT the menu-bar state, then hide. Restoring to EXACTLY this snapshot
    // (rather than an unconditional reveal) avoids clobbering a prior
    // fullscreen/native state on restore (REVIEW MEDIUM — macOS native menu bar).
    if (QMenuBar* mb = mw->menuBar()) {
        s_menuBarWasVisible = mb->isVisible();
        s_menuBarWasNative = mb->isNativeMenuBar();
        mb->hide();
    }

    s_chromeHidden = true;
}

void FwLayout::restoreStockChrome()
{
    if (!s_chromeHidden) {
        return;
    }
    Gui::MainWindow* mw = Gui::getMainWindow();
    auto* tbm = Gui::ToolBarManager::getInstance();
    if (mw == nullptr || tbm == nullptr) {
        return;
    }

    // Restore EXACTLY the toolbars we hid (not all toolbars) via the confirmed
    // round-trip partner of ForceHidden (RESEARCH Q3 RESOLVED).
    if (!s_hiddenToolBars.isEmpty()) {
        tbm->setState(s_hiddenToolBars, Gui::ToolBarManager::State::RestoreDefault);
    }
    s_hiddenToolBars.clear();

    // Restore the menu bar to the SNAPSHOT via setVisible(snapshot), NOT a bare
    // unconditional reveal (REVIEW MEDIUM). On macOS the native-menu setting is
    // part of that snapshot and is restored first.
    if (QMenuBar* mb = mw->menuBar()) {
        mb->setNativeMenuBar(s_menuBarWasNative);
        mb->setVisible(s_menuBarWasVisible);
    }

    s_chromeHidden = false;
}

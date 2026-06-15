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
#include <QDockWidget>
#include <QLabel>
#include <QList>
#include <QMenuBar>
#include <QToolBar>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Workbench.h>
#include <Gui/WorkbenchManager.h>

#include "FwFeatureTree.h"
#include "FwLayout.h"
#include "FwPropertyManagerHeader.h"
#include "FwPropertyReveal.h"
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

// --- PropertyManager reveal + deferred-cancellable teardown (PROP-01) --------
// Owned SEPARATELY from s_ribbonContext (which is reset on the transient deactivated()
// by unmountRibbon()) so it SURVIVES the edit-time workbench switch (the A4 verdict).
std::unique_ptr<FwPropertyReveal> FwLayout::s_propertyReveal;

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

/// Mount the real FwFeatureTree under the permanent "Fw_FeatureManager" dock name,
/// REPLACING the Phase-1 placeholder. Mirrors mountRibbon()'s discipline:
///   - idempotent find-or-reuse by the registered-dock lookup, so re-activation never
///     registers a second tree;
///   - WR-02 unique_ptr-until-adopt: the tree has no QObject parent until
///     registerDockWindow() adopts it, so a throw before the adopt site cannot leak it
///     — release() only at the adopt call;
///   - observe-the-DOM: reach the registry only through DockWindowManager (no
///     MainWindow.cpp edit).
/// The dock objectName stays "Fw_FeatureManager" so QMainWindow saveState()/
/// restoreState() round-trips exactly as the placeholder did (TREE-01, D-01).
void mountFeatureManager(Gui::DockWindowManager* manager)
{
    const char* kFeatureManagerDock = "Fw_FeatureManager";

    // Idempotent: if a FwFeatureTree is already registered under this name (a prior
    // activation), reuse it — never add a second tree (threat T-03-12).
    QWidget* existing = manager->findRegisteredDockWindow(kFeatureManagerDock);
    if (qobject_cast<FreeWorksGui::FwFeatureTree*>(existing) != nullptr) {
        return;
    }

    // Build into a unique_ptr (no parent yet); set the objectName to the dock name so
    // the saved layout keys it exactly like the placeholder it replaces. release() only
    // at the registerDockWindow adopt site.
    auto treeOwner =
        std::make_unique<FreeWorksGui::FwFeatureTree>(kFeatureManagerDock, nullptr);
    treeOwner->setObjectName(QString::fromUtf8(kFeatureManagerDock));
    treeOwner->setWindowTitle(QObject::tr("FeatureManager"));

    // If a placeholder (or anything non-tree) was registered first, drop it so the dock
    // name is free for the tree to claim (find-or-reuse semantics).
    if (existing != nullptr) {
        QWidget* old = manager->unregisterDockWindow(kFeatureManagerDock);
        if (old != nullptr) {
            old->deleteLater();
        }
    }

    manager->registerDockWindow(kFeatureManagerDock, treeOwner.release());
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
    // Fw_FeatureManager hosts the REAL FwFeatureTree (Phase 3). The Phase-1 placeholder
    // is replaced by a mounted tree via the find-or-reuse mount (idempotent; same dock
    // objectName so the saved layout round-trips). PropertyManager / Task Pane stay
    // placeholders until their phases.
    mountFeatureManager(manager);
    // Fw_PropertyManager is DELIBERATELY no longer registered here (Plan 04-02,
    // R3-MAJOR3 disposition (i) — "stop contributing"). The left PropertyManager slot
    // is occupied by the re-hosted "Tasks" TaskView that mountPropertyManager() docks
    // left (D-03 reuse-and-rehost). Registering a Fw_PropertyManager placeholder would
    // make DockWindowManager::setup() create a SECOND live left QDockWidget that the
    // mount would then have to remove async (stranding a stale surface). Not creating it
    // at all is the clean fix.
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

namespace
{

/// Resolve the managed "Tasks" PropertyManager dock and place it in the LEFT area by
/// the CORRECT mechanism (R2-F1 — addDockWindow CANNOT move an already-docked panel,
/// DockWindowManager.cpp:256-258). Returns the hosting QDockWidget on success (already
/// left or just re-docked/created left), or nullptr if no host could be resolved.
///
/// The host is resolved ROBUSTLY from Gui::Control().taskPanel() walked UP to its parent
/// QDockWidget — never a hardcoded container-name guess against the registry key (the
/// live container objectName is "Tasks", not the registry key, so such a lookup may
/// return nullptr). getDockWindow("Tasks") / taskPanel() are parent-independent
/// registry/dock-list lookups, so re-docking preserves them (R2-F3). Idempotent:
/// already-left is a no-op.
QDockWidget* placeTasksDockLeft()
{
    Gui::MainWindow* mw = Gui::getMainWindow();
    Gui::DockWindowManager* manager = Gui::DockWindowManager::instance();
    if (mw == nullptr || manager == nullptr) {
        return nullptr;
    }

    // Walk up from the live task panel to its parent QDockWidget (the "Tasks" container).
    Gui::TaskView::TaskView* taskPanel = Gui::Control().taskPanel();
    auto* dock =
        qobject_cast<QDockWidget*>(taskPanel != nullptr ? taskPanel->parentWidget() : nullptr);

    if (dock != nullptr) {
        if (mw->dockWidgetArea(dock) == Qt::LeftDockWidgetArea) {
            return dock;  // already left — idempotent no-op
        }
        // Came-from-right-dock case: re-dock the EXISTING container left DIRECTLY
        // (addDockWindow would NOT move it — DockWindowManager.cpp:256-258).
        mw->addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->show();
        return dock;
    }

    // Never-docked case (FreeWorks mode, F1: setupDockWindows() never returns
    // Std_TaskView, so no live "Tasks" dock exists). Resolve the registered Tasks
    // TaskView widget (objectName "Tasks", registry key "Std_TaskView") and CREATE the
    // dock left; the NEW-dock branch sets the container objectName to "Tasks"
    // (DockWindowManager.cpp:290) so getDockWindow("Tasks") resolves afterwards.
    QWidget* taskView = manager->findRegisteredDockWindow("Std_TaskView");
    if (taskView == nullptr) {
        return nullptr;
    }
    QDockWidget* created = manager->addDockWindow("Tasks", taskView, Qt::LeftDockWidgetArea);
    if (created != nullptr) {
        created->show();
    }
    return created;
}

/// Remove a stale live Fw_PropertyManager dock if one exists (defense-in-depth for the
/// R3-MAJOR3 post-setup state — even though disposition (i) stops contributing it, a
/// restored layout or a future re-registration must never leave a second left surface).
/// Uses the live-dock removal path (removeDockWindow by name), NOT the pre-setup
/// unregisterDockWindow+deleteLater (which only clears registry state and async-deletes
/// via onWidgetDestroyed, stranding the dock).
void removeStalePropertyManagerDock()
{
    Gui::DockWindowManager* manager = Gui::DockWindowManager::instance();
    if (manager == nullptr) {
        return;
    }
    if (manager->getDockWindow("Fw_PropertyManager") != nullptr) {
        // removeDockWindow(name) destroys the QDockWidget container and returns the inner
        // placeholder widget (parented to nullptr); delete it so nothing is stranded.
        QWidget* inner = manager->removeDockWindow("Fw_PropertyManager");
        if (inner != nullptr) {
            inner->deleteLater();
        }
    }
}

}  // namespace

namespace
{
/// Attach the FwPropertyManagerHeader to @p dock as its title-bar band (container-level,
/// WITHOUT reparenting the inner TaskView — D-01/D-02). Idempotent: if a header is
/// already installed, leave it (re-mount must not stack a second band). The header is
/// chrome that must SURVIVE the edit-time WB switch, so it is owned by the dock and only
/// removed when the dock itself is torn down (never on the transient deactivated()).
void attachPropertyManagerHeader(QDockWidget* dock)
{
    if (dock == nullptr) {
        return;
    }
    if (qobject_cast<FreeWorksGui::FwPropertyManagerHeader*>(dock->titleBarWidget()) != nullptr) {
        return;  // already installed — idempotent
    }
    auto* header = new FreeWorksGui::FwPropertyManagerHeader(dock);
    header->setTitle(dock->windowTitle());
    dock->setTitleBarWidget(header);  // dock takes ownership of the title-bar widget
}
}  // namespace

void FwLayout::mountPropertyManager()
{
    // Reachability guard: no-op until the live shell exists.
    if (Gui::getMainWindow() == nullptr || Gui::DockWindowManager::instance() == nullptr) {
        return;
    }

    // Place the managed "Tasks" PropertyManager dock left (idempotent, two-branch — R2-F1).
    QDockWidget* dock = placeTasksDockLeft();

    // Attach the green-✓/red-✗ header band at the container level (idempotent; the inner
    // TaskView is never reparented). The header drives the existing Control accept/reject.
    attachPropertyManagerHeader(dock);

    // R3-MAJOR3: ensure no stale Fw_PropertyManager dock pollutes the single left surface.
    removeStalePropertyManagerDock();

    // (Re)bind the survivable reveal/teardown consumer and CANCEL any pending teardown a
    // prior unmountPropertyManager() scheduled — re-mounting pre-empts the queued
    // singleShot (the A4 deferred-cancellable policy). Owned SEPARATELY from
    // s_ribbonContext so it survives the transient deactivated().
    if (!s_propertyReveal) {
        s_propertyReveal = std::make_unique<FwPropertyReveal>();
    }
    s_propertyReveal->cancel();
    // The signalInEdit handler re-asserts the idempotent left placement when an edit
    // lands (covers the assureWorkbench WB round-trip — signalInEdit fires after
    // startEditing but within the same synchronous turn as deactivated()).
    s_propertyReveal->connect([]() {
        placeTasksDockLeft();
    });
}

void FwLayout::unmountPropertyManager()
{
    // The A4 deferred-cancellable policy: do NOT tear down inline. An edit-time WB switch
    // fires deactivated() mid-edit BEFORE signalInEdit arms anything (Application.cpp:1984
    // before :2006; ViewProvider.cpp:165 before :175), so a synchronous teardown — or one
    // gated on a pre-checked flag still FALSE at that instant (R4-BLOCKER) — would move the
    // "Tasks" dock back right (R2-F2) and strip the chrome (R3-ROOT). Instead SCHEDULE a
    // cancellable QTimer::singleShot(0) teardown; signalInEdit / a re-mount cancel it
    // within the same event-loop turn so the chrome + placement survive the round-trip.
    if (!s_propertyReveal) {
        return;  // nothing mounted
    }
    s_propertyReveal->schedule([]() {
        // True-exit teardown (only runs when a genuine no-edit FreeWorks->stock switch
        // left the pending teardown uncancelled). Drop the subscriptions so no signal
        // fires into a torn-down chrome — disconnect() (NOT reset()), because this
        // callback runs FROM INSIDE s_propertyReveal's own queued lambda: resetting the
        // unique_ptr would destroy the object mid-stack. The consumer object persists for
        // the FreeWorks-mode lifetime; a later mountPropertyManager() re-connects it.
        // Leaving the "Tasks" dock left is harmless under stock workbenches (the optional
        // right-restore is a nicety, not a need).
        if (s_propertyReveal) {
            s_propertyReveal->disconnect();
        }
    });
}

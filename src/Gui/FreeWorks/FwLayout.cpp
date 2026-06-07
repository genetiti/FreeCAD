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

#include <Gui/DockWindowManager.h>

#include "FwLayout.h"

using namespace FreeWorksGui;

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

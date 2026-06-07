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
#include <QSize>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <Gui/Application.h>
#include <Gui/Command.h>

#include "FwRibbon.h"

using namespace FreeWorksGui;

FwRibbon::FwRibbon(QWidget* parent)
    : QTabWidget(parent)
{}

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

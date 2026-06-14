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

#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>

#include "FwSelectionGuard.h"

namespace FreeWorksGui
{

FwSelectionGuard::FwSelectionGuard()
{
    Gui::SelectionSingleton& sel = Gui::Selection();

    // Snapshot the complete selection by-value. The SelObj const char* pointers
    // (Selection.h:323-333) borrow from the live selection and would dangle after
    // clearSelection(), so we copy each into owned std::strings (reviewer cycle-3
    // MEDIUM). x/y/z are kept so the replay round-trips the pick coordinates.
    const std::vector<Gui::SelectionSingleton::SelObj> complete = sel.getCompleteSelection();
    m_selSnapshot.reserve(complete.size());
    for (const Gui::SelectionSingleton::SelObj& s : complete) {
        OwnedSel owned;
        owned.docName = (s.DocName != nullptr) ? s.DocName : std::string();
        owned.objName = (s.FeatName != nullptr) ? s.FeatName : std::string();
        owned.subName = (s.SubName != nullptr) ? s.SubName : std::string();
        owned.x = s.x;
        owned.y = s.y;
        owned.z = s.z;
        m_selSnapshot.push_back(std::move(owned));
    }

    // Snapshot the preselection by-value too. Both clearSelection() and addSelection()
    // DEFAULT to clearPreSelect=true (Selection.h:385/360), so the restore must
    // re-assert the preselection EXPLICITLY or it is silently wiped (reviewer HIGH-B).
    const Gui::SelectionChanges& pre = sel.getPreselection();
    m_hadPreselect = (pre.Type == Gui::SelectionChanges::SetPreselect);
    if (m_hadPreselect) {
        m_preDocName = (pre.pDocName != nullptr) ? pre.pDocName : std::string();
        m_preObjName = (pre.pObjectName != nullptr) ? pre.pObjectName : std::string();
        m_preSubName = (pre.pSubName != nullptr) ? pre.pSubName : std::string();
        m_preX = pre.x;
        m_preY = pre.y;
        m_preZ = pre.z;
    }
}

void FwSelectionGuard::selectOnly(App::DocumentObject* target)
{
    if (target == nullptr) {
        return;
    }
    App::Document* doc = target->getDocument();
    if (doc == nullptr) {
        return;
    }
    // The PartDesign_MoveTip command requires EXACTLY ONE selected feature
    // (CommandBody.cpp:680). Clear, then select only the target.
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(doc->getName(), target->getNameInDocument());
}

FwSelectionGuard::~FwSelectionGuard()
{
    Gui::SelectionSingleton& sel = Gui::Selection();

    // Restore the snapshotted selection. Replay each entry with the
    // addSelection(..., clearPreSelect=false) overload (Selection.h:360) so the replay
    // does NOT clear the preselect we are about to restore (reviewer HIGH-B). The first
    // clearSelection() wipes whatever the fire left behind.
    sel.clearSelection();
    for (const OwnedSel& s : m_selSnapshot) {
        sel.addSelection(s.docName.c_str(),
                         s.objName.empty() ? nullptr : s.objName.c_str(),
                         s.subName.empty() ? nullptr : s.subName.c_str(),
                         s.x,
                         s.y,
                         s.z,
                         /*pickedList=*/nullptr,
                         /*clearPreSelect=*/false);
    }

    // Restore the preselection EXPLICITLY. Because both clear and add defaulted to
    // clearPreSelect=true (Selection.h:385/360), a plain replay would have left the
    // preselect cleared regardless of its prior state — so we re-assert it here.
    if (m_hadPreselect) {
        sel.setPreselect(m_preDocName.c_str(),
                         m_preObjName.empty() ? nullptr : m_preObjName.c_str(),
                         m_preSubName.empty() ? nullptr : m_preSubName.c_str(),
                         m_preX,
                         m_preY,
                         m_preZ);
    }
    else {
        sel.rmvPreselect();
    }
}

}  // namespace FreeWorksGui

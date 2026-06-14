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

namespace App
{
class DocumentObject;
}

namespace FreeWorksGui
{

/**
 * RAII guard around a global-Gui::Selection-mutating command fire.
 *
 * The PartDesign_MoveTip command (CommandBody.cpp:671-700) reads the GLOBAL
 * Gui::Selection — it calls getSelection().getObjectsOfType(Part::Feature) and
 * requires EXACTLY ONE selected feature, then FCMD_OBJ_SHOWs the tip. To drive it
 * from the rollback bar we must therefore (a) snapshot the user's current selection,
 * (b) select ONLY the target feature for the duration of the fire, and (c) restore
 * the user's selection afterwards so the bar leaves no spurious selection behind.
 *
 * CRITICAL (reviewer HIGH-B): both clearSelection() and addSelection() DEFAULT to
 * clearPreSelect=true (Selection.h:385/360). A naive clear+re-add therefore silently
 * WIPES the preselection it claims to preserve. So this guard ALSO snapshots the
 * preselection (getPreselection(), Selection.h:427) and restores BOTH:
 *   - the selection is replayed with the addSelection(..., clearPreSelect=false)
 *     overload (Selection.h:360) so the replay does not clear preselect, then
 *   - the preselection is re-asserted EXPLICITLY: if the snapshot held a live
 *     preselect (SelectionChanges::Type == SetPreselect), call setPreselect(...)
 *     (Selection.h:413); if it held none, call rmvPreselect() (Selection.h:423).
 * The net effect: getCompleteSelection() AND getPreselection() both round-trip.
 *
 * The snapshot stores OWNED by-value copies (std::string doc/object/sub names — the
 * SelObj const char* pointers borrow from the live selection and would dangle after
 * clearSelection, Selection.h:323-333). It uses ONLY the src/Gui/Selection/Selection.h
 * API, so it carries NO PartDesign include and NO module link.
 *
 * Compiled directly into FreeCADGui and used in-process (mirrors FwFeatureTree /
 * FwRibbon), so no cross-library export macro is required.
 */
class FwSelectionGuard
{
public:
    /// Snapshot the complete global selection AND the current preselection on
    /// construction (by-value owned copies; no borrowed const char*).
    FwSelectionGuard();

    /// Restore BOTH the snapshotted selection (replayed with clearPreSelect=false) and
    /// the snapshotted preselection (re-asserted via setPreselect/rmvPreselect) so the
    /// global Gui::Selection round-trips — no spurious selection, no clobbered preselect.
    ~FwSelectionGuard();

    FwSelectionGuard(const FwSelectionGuard&) = delete;
    FwSelectionGuard& operator=(const FwSelectionGuard&) = delete;
    FwSelectionGuard(FwSelectionGuard&&) = delete;
    FwSelectionGuard& operator=(FwSelectionGuard&&) = delete;

    /// Clear the selection and select EXACTLY @p target (the single feature the
    /// PartDesign_MoveTip command requires). No-op if @p target is null.
    void selectOnly(App::DocumentObject* target);

private:
    /// One owned, replay-safe selection entry (no borrowed pointers).
    struct OwnedSel
    {
        std::string docName;
        std::string objName;
        std::string subName;
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    std::vector<OwnedSel> m_selSnapshot;

    // Preselection snapshot (owned). m_hadPreselect records whether a live preselect
    // existed at construction (SelectionChanges::Type == SetPreselect).
    bool m_hadPreselect = false;
    std::string m_preDocName;
    std::string m_preObjName;
    std::string m_preSubName;
    float m_preX = 0.0F;
    float m_preY = 0.0F;
    float m_preZ = 0.0F;
};

}  // namespace FreeWorksGui

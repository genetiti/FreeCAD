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

#include <cstddef>
#include <string>
#include <vector>

namespace App
{
class DocumentObject;
}

namespace FreeWorksGui
{

/**
 * The rollback-bar MECHANISM (TREE-02, D-04/D-05/D-06): the headline reference-CAD
 * slice. It is NOT a QWidget — it is a pure-logic + command-fire mechanism the
 * FwFeatureTree drives. It splits into a PURE decide half (link-free
 * resolveTipTarget) and an act half (fireTipMove / rollToEnd / insertAtBar), exactly
 * the decide-then-act pattern FwRibbonContext established.
 *
 * Decide (PURE, LINK-FREE, headless-testable): resolveTipTarget() takes a bar row
 * position over the Body's ORDERED feature list (read by the caller via
 * getPropertyByName("Group") as App::PropertyLinkList, GroupExtension.h:142) and
 * snaps to the nearest PRECEDING SOLID feature, classified by its
 * getTypeId().getName() type-name STRING (mirroring isSolidFeature SEMANTICS,
 * Body.cpp:176, but NEVER calling the C++-only Body helper — reviewer concern 7).
 * Datums/sketches are skipped. Bar above all rows -> roll-to-base (Tip = None,
 * returns nullptr); bar at the end -> the last solid feature.
 *
 * Act (the REAL Body.Tip move): fireTipMove() drives the EXISTING PartDesign_MoveTip
 * command-ID (CommandBody.cpp:660) under an RAII FwSelectionGuard. The command opens
 * its OWN transaction (CommandBody.cpp:730) and runs updateActive recompute, so the
 * bar adds NO outer FreeWorks openCommand (would double-wrap undo — reviewer concern
 * 5). If the command-ID is unavailable, a Python "obj.Tip = ..." doCommand fallback
 * opens EXACTLY ONE transaction. There is NO PartDesign module header included and NO
 * PartDesign Gui link: the bar references the rollback engine purely by command-ID /
 * Python string + generic App accessors (D-04/A3/Pitfall 1).
 *
 * Insert-at-bar (insertAtBar) reuses the PYTHON-ONLY Body.insertObject (Body.pyi:24 —
 * no command-ID, and it does NOT move Tip, Body.pyi:34) via a doCommand in EXACTLY
 * ONE transaction, then applies an EXPLICIT post-insert Tip policy IN THE SAME
 * transaction: a SOLID insert becomes the tip; a non-solid (sketch/datum) leaves Tip
 * unchanged (reviewer concern 6).
 *
 * The bar holds NO new persisted state: Body.Tip (the existing persisted
 * App::PropertyLink, BodyBase.h:54) is the single source of truth (D-05).
 *
 * Compiled directly into FreeCADGui and used in-process (mirrors FwFeatureTree /
 * FwRibbon), so no cross-library export macro is required.
 */
class FwRollbackBar
{
public:
    /// The drawn band thickness and grab-zone height (UI-SPEC § Spacing). Exposed so
    /// the FwFeatureTree drawRow paint and the grab hit-test share the same metrics.
    static constexpr int kBandThicknessPx = 4;
    static constexpr int kGrabZonePx = 8;

    /// The command-ID contract literal for the REAL Tip move (CommandBody.cpp:660).
    /// The ONLY identifier FreeWorks uses to reach the rollback engine (no module link).
    static constexpr const char* kMoveTipCommandId = "PartDesign_MoveTip";

    FwRollbackBar() = default;
    ~FwRollbackBar() = default;

    /**
     * PURE, LINK-FREE decide half. Given the Body's ORDERED features @p orderedFeatures
     * (Group order) and the bar's row @p positionRow (0 = above the first row -> roll to
     * base; N = below row N-1), return the SOLID feature the Tip should snap to:
     *   - positionRow == 0            -> nullptr (roll-to-base, Tip = None).
     *   - otherwise                   -> the nearest solid feature at index < positionRow
     *                                     (skips datums/sketches by type-name string).
     *   - none precedes               -> nullptr (roll-to-base).
     * Never mutates, never touches a widget, never calls a C++-only Body helper.
     */
    App::DocumentObject* resolveTipTarget(std::size_t positionRow,
                                          const std::vector<App::DocumentObject*>& orderedFeatures)
        const;

    /// Pure solidness predicate: true iff @p obj's type-name STRING is a known solid
    /// PartDesign feature type (link-free; mirrors isSolidFeature semantics by string).
    bool isSolidByTypeName(const App::DocumentObject* obj) const;

    /**
     * Act half: fire the REAL Body.Tip move. Selects ONLY @p targetFeature under an
     * FwSelectionGuard (which restores both the prior selection AND preselection on
     * scope-exit), then invokes the PartDesign_MoveTip command-ID (its own single
     * transaction — NO outer openCommand). If @p targetFeature is null, rolls to base
     * (Tip = None) via the command on the Body. Returns true if a fire path ran.
     */
    bool fireTipMove(App::DocumentObject* body, App::DocumentObject* targetFeature) const;

    /// Reversibility: resolve the last solid feature in the Body's Group and fire the
    /// forward Tip move to it (under the guard, single transaction). Returns true on fire.
    bool rollToEnd(App::DocumentObject* body) const;

    /**
     * Insert @p newFeature at the bar via the PYTHON-ONLY Body.insertObject (after the
     * current tip feature) in EXACTLY ONE transaction, then apply the explicit
     * post-insert Tip policy in the SAME transaction: a SOLID newFeature becomes the
     * tip; a non-solid leaves Tip unchanged. Returns true if the insert path ran.
     */
    bool insertAtBar(App::DocumentObject* body, App::DocumentObject* newFeature) const;

private:
    /// Read the Body's ordered features link-free via getPropertyByName("Group")
    /// (App::PropertyLinkList). Empty if the property is absent.
    std::vector<App::DocumentObject*> readGroup(const App::DocumentObject* body) const;

    /// The current Tip feature of @p body (generic App::PropertyLink read). May be null.
    App::DocumentObject* currentTip(const App::DocumentObject* body) const;
};

}  // namespace FreeWorksGui

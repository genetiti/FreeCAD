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

#include <string>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/Command.h>

#include "FwRollbackBar.h"
#include "FwSelectionGuard.h"

namespace FreeWorksGui
{

namespace
{
// The set of solid PartDesign feature type-name STRINGS the bar may snap the Tip to.
// FreeWorks decides solidness by these STRINGS — never by calling the C++-only Body
// solid-feature helper (Body.cpp:176), which would force a PartDesign link (reviewer
// concern 7). Datum/sketch type names ("Sketcher::SketchObject", "PartDesign::Plane",
// "PartDesign::Line", "PartDesign::Point") are deliberately ABSENT, so the resolver
// skips them and snaps to the preceding solid (Pitfall 5). The list mirrors the test's
// solidFeatureTypeNames() and must be re-validated against upstream in the spike check.
const std::vector<std::string>& solidFeatureTypeNames()
{
    static const std::vector<std::string> names {
        "PartDesign::Pad",
        "PartDesign::Pocket",
        "PartDesign::Revolution",
        "PartDesign::Groove",
        "PartDesign::AdditiveBox",
        "PartDesign::AdditiveCylinder",
        "PartDesign::AdditiveSphere",
        "PartDesign::AdditiveCone",
        "PartDesign::AdditiveTorus",
        "PartDesign::AdditivePrism",
        "PartDesign::AdditiveWedge",
        "PartDesign::SubtractiveBox",
        "PartDesign::SubtractiveCylinder",
        "PartDesign::SubtractiveSphere",
        "PartDesign::SubtractiveCone",
        "PartDesign::SubtractiveTorus",
        "PartDesign::SubtractivePrism",
        "PartDesign::SubtractiveWedge",
        "PartDesign::Hole",
        "PartDesign::Chamfer",
        "PartDesign::Fillet",
        "PartDesign::Draft",
        "PartDesign::Thickness",
        "PartDesign::Mirrored",
        "PartDesign::LinearPattern",
        "PartDesign::PolarPattern",
        "PartDesign::MultiTransform",
        "PartDesign::Boolean",
        "PartDesign::Loft",
        "PartDesign::AdditiveLoft",
        "PartDesign::SubtractiveLoft",
        "PartDesign::Pipe",
        "PartDesign::AdditivePipe",
        "PartDesign::SubtractivePipe",
    };
    return names;
}
}  // namespace

bool FwRollbackBar::isSolidByTypeName(const App::DocumentObject* obj) const
{
    if (obj == nullptr) {
        return false;
    }
    const std::string typeName = obj->getTypeId().getName();
    for (const std::string& solid : solidFeatureTypeNames()) {
        if (typeName == solid) {
            return true;
        }
    }
    return false;
}

App::DocumentObject* FwRollbackBar::resolveTipTarget(
    std::size_t positionRow,
    const std::vector<App::DocumentObject*>& orderedFeatures) const
{
    // positionRow == 0 means the bar sits above the first row -> roll to base (Tip =
    // None, signalled by nullptr). Otherwise snap to the nearest PRECEDING solid feature
    // (index < positionRow), skipping datums/sketches by their type-name string.
    if (positionRow == 0) {
        return nullptr;
    }
    std::size_t start = positionRow;
    if (start > orderedFeatures.size()) {
        start = orderedFeatures.size();  // bar at/after the end -> last solid
    }
    for (std::size_t i = start; i-- > 0;) {
        if (isSolidByTypeName(orderedFeatures[i])) {
            return orderedFeatures[i];
        }
    }
    return nullptr;  // no solid precedes -> roll to base
}

std::vector<App::DocumentObject*> FwRollbackBar::readGroup(const App::DocumentObject* body) const
{
    std::vector<App::DocumentObject*> out;
    if (body == nullptr) {
        return out;
    }
    // Link-free read of the Body's ordered features via the generic GroupExtension
    // "Group" PropertyLinkList (GroupExtension.h:142) — getPropertyByName, no module
    // include.
    const App::Property* prop = body->getPropertyByName("Group");
    const auto* links = dynamic_cast<const App::PropertyLinkList*>(prop);
    if (links == nullptr) {
        return out;
    }
    for (App::DocumentObject* obj : links->getValues()) {
        out.push_back(obj);
    }
    return out;
}

App::DocumentObject* FwRollbackBar::currentTip(const App::DocumentObject* body) const
{
    if (body == nullptr) {
        return nullptr;
    }
    const App::Property* prop = body->getPropertyByName("Tip");
    const auto* tip = dynamic_cast<const App::PropertyLink*>(prop);
    return tip != nullptr ? tip->getValue() : nullptr;
}

bool FwRollbackBar::fireTipMove(App::DocumentObject* body, App::DocumentObject* targetFeature) const
{
    if (body == nullptr) {
        return false;
    }

    // Select ONLY the fire target under the RAII guard: it snapshots the prior selection
    // AND preselection, and restores BOTH on scope-exit (reviewer HIGH-B). The
    // PartDesign_MoveTip command reads the global selection (CommandBody.cpp:674) and
    // requires exactly one selected feature. For roll-to-base (targetFeature == null)
    // the command treats the Body itself as the selection (CommandBody.cpp:732), so we
    // select the Body.
    FwSelectionGuard guard;
    guard.selectOnly(targetFeature != nullptr ? targetFeature : body);

    // Prefer the EXISTING command-ID: it opens its OWN single transaction
    // (CommandBody.cpp:730) and runs updateActive recompute, so we add NO outer
    // FreeWorks transaction wrap (would double-wrap undo — reviewer concern 5). No
    // module link: we reach the command purely by its registered ID.
    Gui::Application* app = Gui::Application::Instance;
    if (app != nullptr) {
        Gui::Command* cmd = app->commandManager().getCommandByName(kMoveTipCommandId);
        if (cmd != nullptr) {
            cmd->invoke(0);
            return true;
        }
    }

    // Python fallback (no command-ID registered): set Tip directly via a doCommand
    // string in EXACTLY ONE transaction (A3 — reference by string, never link).
    App::Document* doc = body->getDocument();
    if (doc == nullptr) {
        return false;
    }
    const std::string bodyCmd = Gui::Command::getObjectCmd(body);
    std::string tipExpr;
    if (targetFeature != nullptr) {
        tipExpr = bodyCmd + ".Tip = " + Gui::Command::getObjectCmd(targetFeature);
    }
    else {
        tipExpr = bodyCmd + ".Tip = None";
    }
    doc->openTransaction("Move tip to selected feature");
    try {
        Base::Interpreter().runString(tipExpr.c_str());
    }
    catch (...) {
        doc->abortTransaction();
        throw;
    }
    doc->commitTransaction();
    doc->recompute();
    return true;
}

bool FwRollbackBar::rollToEnd(App::DocumentObject* body) const
{
    if (body == nullptr) {
        return false;
    }
    // Reversibility (Pitfall 4): resolve the LAST solid feature in the Group and fire
    // the forward Tip move to it (under the guard, single internal transaction).
    const std::vector<App::DocumentObject*> group = readGroup(body);
    App::DocumentObject* lastSolid = nullptr;
    for (App::DocumentObject* obj : group) {
        if (isSolidByTypeName(obj)) {
            lastSolid = obj;
        }
    }
    return fireTipMove(body, lastSolid);
}

bool FwRollbackBar::insertAtBar(App::DocumentObject* body, App::DocumentObject* newFeature) const
{
    if (body == nullptr || newFeature == nullptr) {
        return false;
    }
    App::Document* doc = body->getDocument();
    if (doc == nullptr) {
        return false;
    }

    // insertObject is PYTHON-ONLY (Body.pyi:24 — there is NO command-ID) and does NOT
    // move the Tip (Body.pyi:34). Fire it via a doCommand string and apply the EXPLICIT
    // post-insert Tip policy IN THE SAME transaction (reviewer concern 6).
    App::DocumentObject* tipFeature = currentTip(body);
    const std::string bodyCmd = Gui::Command::getObjectCmd(body);
    const std::string newCmd = Gui::Command::getObjectCmd(newFeature);

    std::string script = bodyCmd + ".insertObject(" + newCmd + ", ";
    if (tipFeature != nullptr) {
        script += Gui::Command::getObjectCmd(tipFeature);
    }
    else {
        script += "None";
    }
    script += ", True)\n";

    // SW-like mid-history insertion: a SOLID insert becomes the new tip; a non-solid
    // (sketch/datum) leaves Tip unchanged. Same transaction, no extra wrap.
    if (isSolidByTypeName(newFeature)) {
        script += bodyCmd + ".Tip = " + newCmd + "\n";
    }

    doc->openTransaction("Insert feature at rollback bar");
    try {
        Base::Interpreter().runString(script.c_str());
    }
    catch (...) {
        doc->abortTransaction();
        throw;
    }
    doc->commitTransaction();
    doc->recompute();
    return true;
}

}  // namespace FreeWorksGui

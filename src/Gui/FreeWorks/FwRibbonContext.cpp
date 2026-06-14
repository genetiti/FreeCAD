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

#include <QString>

// NOTE (RESEARCH Pitfall 2 / threat T-02-08): there is deliberately NO Sketcher
// module header included here. A sketch is identified ONLY by its view-provider
// type-name STRING, so FreeWorks keeps zero compile/link dependency on the Sketcher
// module. Pulling in a Sketcher header to dynamic_cast the view provider would
// break additive/merge discipline — do not add one.
#include <Gui/Application.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "FwRibbon.h"
#include "FwRibbonContext.h"

using namespace FreeWorksGui;

namespace
{
// The documented external type-name contract for a sketch view provider
// (PROPERTY_SOURCE_WITH_EXTENSIONS(SketcherGui::ViewProviderSketch, ...),
// ViewProviderSketch.cpp:562). This is the sole coupling point to Sketcher and it
// is a STRING, not an include (RESEARCH Pitfall 2).
//
// LIVE RE-VALIDATION REQUIRED (REVIEW LOW / VALIDATION.md): a pure unit test cannot
// prove this literal still matches the upstream type name — only the running app
// can (enter a sketch, confirm the Sketch tab activates). If a future upstream
// rename breaks the match, the manual sketch enter/exit validation item is what
// catches it. The curated "Sketch" tab name below must also exist in FwRibbonMap.
constexpr const char* kSketchViewProviderTypeName = "SketcherGui::ViewProviderSketch";

// The curated tab label the context switches to on sketch entry. FwRibbon resolves
// this by label; an absent tab is a tolerated no-op (FwRibbon::setCurrentTab).
constexpr const char* kSketchTabName = "Sketch";
}  // namespace

FwRibbonContext::FwRibbonContext(FwRibbon* ribbon)
    : ribbon_(ribbon)
{}

FwRibbonContext::~FwRibbonContext()
{
    // The scoped_connection members release on destruction, but call disconnect()
    // explicitly so the order is unambiguous and a late signal cannot fire into a
    // half-destroyed context (REVIEW MEDIUM / threat T-02-09).
    disconnect();
}

void FwRibbonContext::setRibbon(FwRibbon* ribbon)
{
    // QPointer tracks destruction: if the ribbon is later torn down, ribbon_ becomes
    // null and the signal handlers below are guarded no-ops (no dangling deref).
    ribbon_ = ribbon;
}

void FwRibbonContext::connect()
{
    Gui::Application* app = Gui::Application::Instance;
    if (app == nullptr) {
        return;
    }

    // Idempotent: reassigning the scoped_connection members disconnects any prior
    // subscription first, so connect() never leaves a duplicate subscription behind
    // (pairs with FwLayout's idempotent mount — threat T-02-09).
    //
    // D-09 REINTERPRETATION (documented): CONTEXT D-09 says the switch is "wired to
    // Gui::Control active-dialog / edit state". RESEARCH Pitfall 1 establishes that
    // Control's active-dialog accessor is a getter with NO change signal, so reading
    // it would force a laggy/racy polling loop (which is why this class never polls
    // it). The application-level edit signals signalInEdit / signalResetEdit ARE the
    // event-driven realization of that edit state (the OverlayManager consumes
    // exactly these — OverlayManager.cpp:406-409). Subscribing to them is therefore
    // faithful to D-09, not a deviation.
    inEditConn_ = app->signalInEdit.connect(
        [this](const Gui::ViewProviderDocumentObject& vp) {
            const std::string typeName = vp.getTypeId().getName();
            const TabAction action = decideOnEnter(isSketchType(typeName),
                                                   ribbon_ ? ribbon_->currentIndex() : 0);
            // The pure core never returns a raw index; the bound layer resolves the
            // curated "Sketch" tab by name. Guarded by QPointer (REVIEW MEDIUM).
            if (action == TabAction::SwitchToSketch && ribbon_) {
                ribbon_->setCurrentTab(QString::fromUtf8(kSketchTabName));
            }
        });

    resetEditConn_ = app->signalResetEdit.connect(
        [this](const Gui::ViewProviderDocumentObject&) {
            // Reset is type-agnostic: decideOnReset() only restores when a
            // sketch-driven switch is currently active (contextActive_), so a reset
            // from leaving a non-sketch edit is a NoOp and cannot force a stale tab
            // (REVIEW MEDIUM).
            const TabAction action = decideOnReset();
            if (action == TabAction::RestorePrevious && ribbon_) {
                ribbon_->setCurrentIndex(previousIndex_);
            }
        });
}

void FwRibbonContext::disconnect()
{
    // scoped_connection::disconnect() is idempotent and safe on an empty connection.
    inEditConn_.disconnect();
    resetEditConn_.disconnect();
}

// --- PURE switch core (no ribbon, no Qt) ------------------------------------

FwRibbonContext::TabAction FwRibbonContext::decideOnEnter(bool isSketch, int currentIndex)
{
    if (!isSketch) {
        // Only sketch entry switches in v1 (D-09 sketch-edit only). Do not touch the
        // state machine: a non-sketch edit must not arm or disarm the context.
        return TabAction::NoOp;
    }
    if (contextActive_) {
        // Repeated/nested sketch enter while already active: do NOT re-stash the
        // remembered tab (it would currently be the Sketch index) — that is the bug
        // REVIEW concern 7 flagged. Leave previousIndex_ as the original pre-sketch
        // tab so reset restores it, not Sketch.
        return TabAction::NoOp;
    }
    // First sketch enter: remember where the user was (D-10), arm the context, and
    // switch (D-09 context wins, even over a manual selection).
    previousIndex_ = currentIndex;
    contextActive_ = true;
    return TabAction::SwitchToSketch;
}

FwRibbonContext::TabAction FwRibbonContext::decideOnReset()
{
    if (!contextActive_) {
        // A stray reset (e.g. leaving a non-sketch edit, or a reset with no matching
        // enter) must never force a stale tab (REVIEW MEDIUM).
        return TabAction::NoOp;
    }
    contextActive_ = false;
    return TabAction::RestorePrevious;
}

bool FwRibbonContext::isSketchType(const std::string& vpTypeName) const
{
    return vpTypeName == kSketchViewProviderTypeName;
}

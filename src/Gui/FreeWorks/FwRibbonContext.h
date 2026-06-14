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

#include <QPointer>

#include <fastsignals/connection.h>

namespace FreeWorksGui
{

class FwRibbon;

/**
 * The single contextual tab switch a reference-CAD user notices most (RIBBON-02 /
 * Success Criterion 4, CONTEXT D-09/D-10): entering a sketch auto-activates the
 * "Sketch" tab (context wins, even over a manual selection), and leaving restores
 * whichever tab was active before entering (clean revert).
 *
 * It is an EVENT-DRIVEN subscriber to the application-level edit signals
 * Gui::Application::Instance->signalInEdit / signalResetEdit (mirrors
 * OverlayManager.cpp:406-409). It does NOT poll Gui::Control::activeDialog() —
 * Control exposes a getter with no change signal, so polling is laggy/racy
 * (RESEARCH Pitfall 1). The edit signals are the correct, documented realization of
 * D-09's "wired to Gui::Control active-dialog / edit state".
 *
 * A sketch is identified ONLY by the view-provider type-name STRING
 * "SketcherGui::ViewProviderSketch" — a documented external type-name contract — so
 * FreeWorks keeps ZERO compile/link dependency on the Sketcher module
 * (RESEARCH Pitfall 2). No <Mod/Sketcher/...> include appears here.
 *
 * The switch decision is factored into a PURE, headless-testable core
 * (decideOnEnter / decideOnReset) that returns a TabAction enum and never
 * references the ribbon or a raw tab index. The ribbon-bound layer resolves
 * TabAction::SwitchToSketch to the curated "Sketch" tab by name and
 * TabAction::RestorePrevious to the remembered index. This keeps the
 * concern-7 state machine unit-testable with no QWidget.
 *
 * The ribbon is held via QPointer<FwRibbon> so an edit signal firing AFTER the
 * ribbon is torn down is a guarded no-op rather than a dangling-pointer
 * dereference (REVIEW MEDIUM). Scoped fastsignals connections are stored and
 * released in disconnect()/the destructor so no subscription leaks past
 * FreeWorks mode (REVIEW MEDIUM / threat T-02-09).
 *
 * Compiled directly into FreeCADGui and used in-process (mirrors FwRibbon /
 * FwWorkbench), so no cross-library export macro is required.
 */
class FwRibbonContext
{
public:
    /**
     * The decision returned by the pure switch core. The pure core NEVER returns a
     * raw tab index (REVIEW MEDIUM); the ribbon-bound layer resolves the action:
     * SwitchToSketch -> setCurrentTab("Sketch"), RestorePrevious ->
     * setCurrentIndex(previousIndex_), NoOp -> do nothing.
     */
    enum class TabAction
    {
        SwitchToSketch,
        NoOp,
        RestorePrevious,
    };

    FwRibbonContext() = default;
    explicit FwRibbonContext(FwRibbon* ribbon);
    ~FwRibbonContext();

    FwRibbonContext(const FwRibbonContext&) = delete;
    FwRibbonContext& operator=(const FwRibbonContext&) = delete;
    FwRibbonContext(FwRibbonContext&&) = delete;
    FwRibbonContext& operator=(FwRibbonContext&&) = delete;

    /// Bind (or rebind) the ribbon this context drives, held weakly via QPointer.
    void setRibbon(FwRibbon* ribbon);

    /**
     * Subscribe to Gui::Application::Instance->signalInEdit / signalResetEdit. The
     * inEdit handler reads vp.getTypeId().getName(), runs isSketchType() +
     * decideOnEnter(), and on SwitchToSketch calls ribbon_->setCurrentTab("Sketch")
     * (only if the ribbon is still alive). The resetEdit handler runs
     * decideOnReset() and on RestorePrevious calls
     * ribbon_->setCurrentIndex(previousIndex_). Idempotent: a second connect()
     * replaces the existing scoped connections so there is never a duplicate
     * subscription (pairs with FwLayout's idempotent mount). No-op if the
     * application singleton is not yet available.
     */
    void connect();

    /// Release the stored scoped connections so no signal fires after teardown
    /// (also runs automatically from the destructor). Idempotent.
    void disconnect();

    // --- PURE, headless-testable switch core (no ribbon, no Qt) --------------

    /**
     * Decide what to do when an object enters edit mode (D-09/D-10, concern 7):
     *  - isSketch && !contextActive_ : stash previousIndex_ = @p currentIndex, set
     *    contextActive_ = true, return SwitchToSketch (context wins, remember).
     *  - isSketch && contextActive_  : return NoOp and do NOT re-stash previousIndex_
     *    (a repeated/nested sketch enter must not overwrite the remembered tab with
     *    the Sketch index — REVIEW concern 7).
     *  - !isSketch                   : return NoOp, leave contextActive_ untouched
     *    (only sketch entry switches in v1 — D-09 sketch-edit only).
     */
    TabAction decideOnEnter(bool isSketch, int currentIndex);

    /**
     * Decide what to do when an object leaves edit mode:
     *  - contextActive_  : clear contextActive_, return RestorePrevious (the bound
     *    layer restores previousIndex_).
     *  - !contextActive_ : return NoOp (a stray reset never forces a stale tab —
     *    REVIEW MEDIUM).
     */
    TabAction decideOnReset();

    /// True iff @p vpTypeName is the sketch view-provider type-name contract literal
    /// "SketcherGui::ViewProviderSketch" (RESEARCH Pitfall 2 — string only, no
    /// Sketcher include).
    bool isSketchType(const std::string& vpTypeName) const;

    // --- state inspection (for tests) ----------------------------------------
    bool isContextActive() const
    {
        return contextActive_;
    }
    int previousIndex() const
    {
        return previousIndex_;
    }

private:
    /// Weak, guarded reference: a signal firing after the ribbon is destroyed sees a
    /// null QPointer and is a no-op (REVIEW MEDIUM — no dangling dereference).
    QPointer<FwRibbon> ribbon_;

    /// Explicit state machine (REVIEW concern 7): true while a sketch-driven context
    /// switch is in effect. Gates both nested enters (NoOp) and stray resets (NoOp).
    bool contextActive_ = false;

    /// The tab index active before the sketch-driven switch; restored on reset.
    int previousIndex_ = 0;

    /// Scoped connections — auto-disconnect on destruction / reassignment so no
    /// subscription leaks past FreeWorks mode (RESEARCH Pattern 3; threat T-02-09).
    fastsignals::scoped_connection inEditConn_;
    fastsignals::scoped_connection resetEditConn_;
};

}  // namespace FreeWorksGui

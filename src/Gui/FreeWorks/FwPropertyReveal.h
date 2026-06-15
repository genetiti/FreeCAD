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

#include <functional>

#include <fastsignals/connection.h>

namespace FreeWorksGui
{

/**
 * The survivable reveal + deferred-cancellable-teardown consumer for the left-docked
 * "Tasks" PropertyManager (PROP-01, the 04-01 A4 verdict).
 *
 * It is owned by FwLayout via a unique_ptr static held SEPARATELY from
 * s_ribbonContext — which is reset on the transient deactivated() (FwLayout.cpp
 * unmountRibbon()) — so this consumer SURVIVES the edit-time workbench switch. It holds
 * two scoped fastsignals subscriptions and a pending-teardown generation token:
 *
 *   - signalInEdit  -> CANCEL any pending teardown (the A4 deferred-cancellable policy)
 *                      and re-assert the idempotent left placement / raise. signalInEdit
 *                      fires only after startEditing (Document.cpp:680-692) but WITHIN
 *                      the same synchronous activateWorkbench turn as deactivated(), so
 *                      it pre-empts the queued QTimer::singleShot(0) teardown
 *                      (R2-F2/R3-ROOT/R4-BLOCKER).
 *   - signalResetEdit -> reserved for the FLOW-01 sketch-exit handoff (Plan 04-04). It
 *                        does NOT re-schedule teardown.
 *
 * unmountPropertyManager() SCHEDULES the teardown through schedule(): it posts a
 * QTimer::singleShot(0) guarded by a generation token. cancel() bumps the token so the
 * already-queued lambda becomes a no-op when it eventually fires — making cancellation
 * race-free without relying on QTimer ownership. The teardown callback (supplied by
 * FwLayout) releases the chrome + drops this consumer's subscriptions and is only run
 * if not cancelled.
 *
 * The reAssert callback (supplied by FwLayout) re-runs the idempotent left placement on
 * signalInEdit. Both callbacks are plain std::function so this class keeps no link to
 * the dock machinery (single-responsibility: lifetime + signal plumbing only).
 */
class FwPropertyReveal
{
public:
    FwPropertyReveal() = default;
    ~FwPropertyReveal();

    FwPropertyReveal(const FwPropertyReveal&) = delete;
    FwPropertyReveal& operator=(const FwPropertyReveal&) = delete;
    FwPropertyReveal(FwPropertyReveal&&) = delete;
    FwPropertyReveal& operator=(FwPropertyReveal&&) = delete;

    /// (Re)subscribe to signalInEdit / signalResetEdit. @p reAssert re-runs the
    /// idempotent left placement when an edit lands. Idempotent: a second connect()
    /// replaces the existing scoped connections (never stacks). No-op if the
    /// application singleton is not yet available.
    void connect(std::function<void()> reAssert);

    /// Release the stored scoped connections (also runs from the destructor). Idempotent.
    void disconnect();

    /// SCHEDULE a cancellable QTimer::singleShot(0) teardown. @p teardown runs on the
    /// next event-loop turn ONLY if cancel() (via signalInEdit / re-mount) has not bumped
    /// the generation token first. Re-scheduling bumps the token first, so a prior
    /// pending teardown is cancelled rather than stacked.
    void schedule(std::function<void()> teardown);

    /// Cancel any pending teardown by invalidating the queued lambda's generation token.
    void cancel();

private:
    /// Re-assert the idempotent left placement on signalInEdit (set by connect()).
    std::function<void()> reAssert_;

    /// Monotonic token: every schedule()/cancel() bumps it so a stale queued teardown
    /// lambda (captured with an older token) becomes a no-op. Race-free cancellation
    /// without depending on QTimer object ownership.
    unsigned long pendingGeneration_ = 0;
    bool teardownPending_ = false;

    fastsignals::scoped_connection inEditConn_;
    fastsignals::scoped_connection resetEditConn_;
};

}  // namespace FreeWorksGui

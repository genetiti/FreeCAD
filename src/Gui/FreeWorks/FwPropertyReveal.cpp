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

#include <QTimer>

#include <Gui/Application.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "FwPropertyReveal.h"

using namespace FreeWorksGui;

FwPropertyReveal::~FwPropertyReveal()
{
    // Invalidate any queued teardown so a singleShot firing after destruction is a
    // no-op, then drop the subscriptions explicitly so the order is unambiguous.
    cancel();
    disconnect();
}

void FwPropertyReveal::connect(std::function<void()> reAssert)
{
    reAssert_ = std::move(reAssert);

    Gui::Application* app = Gui::Application::Instance;
    if (app == nullptr) {
        return;
    }

    // Idempotent: reassigning the scoped_connection members disconnects any prior
    // subscription first, so connect() never leaves a duplicate behind (pairs with
    // FwLayout's idempotent mount).
    //
    // signalInEdit is the event-driven realization of "an edit started" (Pitfall 1:
    // Control().activeDialog() has no change signal — never poll it). It fires AFTER
    // startEditing but WITHIN the same synchronous activateWorkbench turn as the
    // transient deactivated(), so cancelling the pending teardown here pre-empts the
    // queued QTimer::singleShot(0) (R2-F2/R3-ROOT/R4-BLOCKER).
    inEditConn_ = app->signalInEdit.connect(
        [this](const Gui::ViewProviderDocumentObject&) {
            cancel();
            if (reAssert_) {
                reAssert_();
            }
        });

    // signalResetEdit is reserved for the FLOW-01 sketch-exit handoff (Plan 04-04). It
    // does NOT re-schedule teardown — a genuine workbench exit is what schedules it.
    resetEditConn_ = app->signalResetEdit.connect(
        [](const Gui::ViewProviderDocumentObject&) {
            // Intentionally empty in 04-02: the FLOW-01 handler lands here in 04-04.
        });
}

void FwPropertyReveal::disconnect()
{
    inEditConn_.disconnect();
    resetEditConn_.disconnect();
}

void FwPropertyReveal::schedule(std::function<void()> teardown)
{
    // Cancel-then-reschedule: bumping the generation invalidates any earlier queued
    // teardown so re-scheduling never stacks two pending teardowns.
    ++pendingGeneration_;
    const unsigned long myGeneration = pendingGeneration_;
    teardownPending_ = true;

    auto run = std::move(teardown);
    // Post a guarded lambda. If cancel()/another schedule() bumped the generation by the
    // time the loop turn arrives, this lambda is a no-op — race-free cancellation that
    // does not depend on owning/stopping the QTimer object itself.
    QTimer::singleShot(0, [this, myGeneration, run = std::move(run)]() {
        if (myGeneration != pendingGeneration_ || !teardownPending_) {
            return;  // cancelled by signalInEdit / a re-mount before this turn
        }
        teardownPending_ = false;
        if (run) {
            run();
        }
    });
}

void FwPropertyReveal::cancel()
{
    // Bump the generation so the queued lambda (captured with the prior token) becomes a
    // no-op, and clear the pending flag.
    ++pendingGeneration_;
    teardownPending_ = false;
}

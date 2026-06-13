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

#include <span>

namespace FreeWorksGui
{

/**
 * One curated ribbon entry: a single command ID placed in a named panel of a
 * named tab. The table is a flat list of these rows; FwRibbon groups them by
 * @ref tab then @ref panel at build time (preserving first-seen order), so the
 * row order in the table is the visual left-to-right / top-to-bottom order.
 *
 * All three members are compile-time string literals (developer constants),
 * never user input — they are resolved against the live command registry via
 * Gui::CommandManager::getCommandByName(). A *_Comp* @ref commandId is a command
 * group that yields a native split-button (flyout) downstream (Action.cpp).
 *
 * D-08 (omit-missing): an unresolved @ref commandId is silently skipped at
 * RUNTIME, but the headless per-row resolution test treats an unresolved curated
 * row as a FAILURE (the typo guard for these pinned IDs) — silent-omit is runtime
 * robustness, not a license for the test to pass on a misspelled ID.
 */
struct FwRibbonRow
{
    const char* tab;        ///< Curated tab name: "Features", "Sketch", "Evaluate".
    const char* panel;      ///< Reference-CAD-faithful plain-noun panel title.
    const char* commandId;  ///< Real registered command ID (resolved via the registry).
};

/**
 * The curated declarative map for the FreeWorks core loop (D-05/D-06).
 *
 * A thin static accessor over a compile-time row table — no parse, no IO, no
 * resource file (RESEARCH "Curated-Map File Format": a C++ table is the chosen
 * format). Plan 02-02 owns Features/Sketch/Evaluate; later waves may extend the
 * roster, so callers consume the rows through @ref rows() rather than reaching
 * for a fixed-size array.
 */
class FwRibbonMap
{
public:
    /// The curated core-loop rows in visual order (Features -> Sketch -> Evaluate).
    static std::span<const FwRibbonRow> rows();
};

}  // namespace FreeWorksGui

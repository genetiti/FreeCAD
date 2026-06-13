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

#include <array>

#include "FwRibbonMap.h"

using namespace FreeWorksGui;

namespace
{

// The curated core loop (D-05/D-06). Every commandId below is a real registered
// ID read from this checkout (RESEARCH § "Verified core-loop command IDs",
// Open Questions Q1/Q2 RESOLVED) — placed as-is, no execute-time grep. Row order
// is the on-screen order: tab order Features -> Sketch -> Evaluate (RESEARCH Q1
// RESOLVED-WITH-FALLBACK, the reference-CAD CommandManager tab order); within a
// tab, panel order and button order follow first-seen order here.
//
// D-08 STRICT: only real IDs are listed; no FreeCAD-only extras are appended.
// A *_Comp* id is a command group that becomes a native split-button (flyout)
// downstream. Reference-CAD-with-no-FreeCAD-equivalent gaps are noted as `// gap:`
// comments rather than fabricated/unresolvable IDs — there is no swallowing
// allow-list, so the per-row resolution test stays a true typo guard.
constexpr std::array<FwRibbonRow, 56> kRows = {{
    // ---------------------------------------------------------------- Features
    // Owned by PartDesignGui (PartDesign/Gui/Workbench.cpp); the dress-up IDs at
    // Workbench.cpp:177,208; additive/subtractive/pattern groups at 487-526.
    {"Features", "Sketch", "PartDesign_NewSketch"},

    // Additive — leads with the additive primitive flyout, then the solid features.
    {"Features", "Additive", "PartDesign_CompPrimitiveAdditive"},
    {"Features", "Additive", "PartDesign_Pad"},
    {"Features", "Additive", "PartDesign_Revolution"},
    {"Features", "Additive", "PartDesign_AdditiveLoft"},
    {"Features", "Additive", "PartDesign_AdditivePipe"},
    {"Features", "Additive", "PartDesign_AdditiveHelix"},

    // Subtractive — leads with the subtractive primitive flyout.
    {"Features", "Subtractive", "PartDesign_CompPrimitiveSubtractive"},
    {"Features", "Subtractive", "PartDesign_Pocket"},
    {"Features", "Subtractive", "PartDesign_Hole"},
    {"Features", "Subtractive", "PartDesign_Groove"},
    {"Features", "Subtractive", "PartDesign_SubtractiveLoft"},
    {"Features", "Subtractive", "PartDesign_SubtractivePipe"},
    {"Features", "Subtractive", "PartDesign_SubtractiveHelix"},

    // Dress-Up — fillet/chamfer/draft/shell.
    {"Features", "Dress-Up", "PartDesign_Fillet"},
    {"Features", "Dress-Up", "PartDesign_Chamfer"},
    {"Features", "Dress-Up", "PartDesign_Draft"},
    {"Features", "Dress-Up", "PartDesign_Thickness"},

    // Pattern — mirror / linear / circular / multi-transform.
    {"Features", "Pattern", "PartDesign_Mirrored"},
    {"Features", "Pattern", "PartDesign_LinearPattern"},
    {"Features", "Pattern", "PartDesign_PolarPattern"},
    {"Features", "Pattern", "PartDesign_MultiTransform"},

    // Reference — body / binders / clone / boolean.
    {"Features", "Reference", "PartDesign_Body"},
    {"Features", "Reference", "PartDesign_ShapeBinder"},
    {"Features", "Reference", "PartDesign_SubShapeBinder"},
    {"Features", "Reference", "PartDesign_Clone"},
    {"Features", "Reference", "PartDesign_Boolean"},

    // ------------------------------------------------------------------ Sketch
    // Owned by SketcherGui (Sketcher/Gui/Workbench.cpp + CommandConstraints.cpp).
    {"Sketch", "Sketch", "Sketcher_NewSketch"},
    {"Sketch", "Sketch", "Sketcher_EditSketch"},
    {"Sketch", "Sketch", "Sketcher_LeaveSketch"},
    {"Sketch", "Sketch", "Sketcher_MapSketch"},
    {"Sketch", "Sketch", "Sketcher_ValidateSketch"},

    // Geometry — flyouts for the multi-variant tools, then the core primitives.
    {"Sketch", "Geometry", "Sketcher_CompLine"},
    {"Sketch", "Geometry", "Sketcher_CompCreateArc"},
    {"Sketch", "Geometry", "Sketcher_CompCreateConic"},
    {"Sketch", "Geometry", "Sketcher_CompCreateRectangles"},
    {"Sketch", "Geometry", "Sketcher_CompCreateRegularPolygon"},
    {"Sketch", "Geometry", "Sketcher_CreateLine"},
    {"Sketch", "Geometry", "Sketcher_CreatePolyline"},
    {"Sketch", "Geometry", "Sketcher_CreateArc"},
    {"Sketch", "Geometry", "Sketcher_CreateCircle"},
    {"Sketch", "Geometry", "Sketcher_CreateRectangle"},

    // Dimensions — the dimensioning / dimensional-constraint tools.
    {"Sketch", "Dimensions", "Sketcher_Dimension"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainDistance"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainDistanceX"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainDistanceY"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainRadius"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainDiameter"},
    {"Sketch", "Dimensions", "Sketcher_ConstrainAngle"},

    // Relations — geometric (non-dimensional) constraints.
    {"Sketch", "Relations", "Sketcher_ConstrainHorizontal"},
    {"Sketch", "Relations", "Sketcher_ConstrainVertical"},
    {"Sketch", "Relations", "Sketcher_ConstrainParallel"},
    {"Sketch", "Relations", "Sketcher_ConstrainPerpendicular"},
    {"Sketch", "Relations", "Sketcher_ConstrainTangent"},
    {"Sketch", "Relations", "Sketcher_ConstrainEqual"},
    {"Sketch", "Relations", "Sketcher_ConstrainSymmetric"},
    {"Sketch", "Relations", "Sketcher_ConstrainBlock"},

    // ---------------------------------------------------------------- Evaluate
    // NOT all PartDesignGui-owned: Part_CheckGeometry is PartGui (pulled in by
    // PartDesignGui), Std_Measure/Std_MassProperties are MeasureGui, and
    // Materials_Inspect* are MatGui (REVIEW cycle-2 NEW HIGH 2). The per-row test
    // covering these rows is the load-bearing guard that the bootstrap imports
    // all of those owning modules.
    {"Evaluate", "Evaluate", "Part_CheckGeometry"},
    {"Evaluate", "Evaluate", "Std_Measure"},
    {"Evaluate", "Evaluate", "Std_MassProperties"},
    {"Evaluate", "Evaluate", "Materials_InspectMaterial"},
    {"Evaluate", "Evaluate", "Materials_InspectAppearance"},

    // gap: the reference-CAD "Sensor" / "Measure Wall Thickness" tools have no
    // direct 1:1 FreeCAD core command — intentionally NOT placed (no fabricated ID).
    // gap: the reference-CAD "Section View" lives on the heads-up view toolbar, not
    // the CommandManager ribbon — out of scope for the curated core-loop map here.
}};

}  // namespace

std::span<const FwRibbonRow> FwRibbonMap::rows()
{
    return {kRows.data(), kRows.size()};
}

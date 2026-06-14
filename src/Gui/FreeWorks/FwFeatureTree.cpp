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

// Include the Qt headers this TU uses unconditionally rather than relying on the PCH
// (mirrors FwRibbon.cpp WR-04): <QTreeWidget> arrives via the Gui::TreeWidget base,
// but the drawRow signature and the item API are named here explicitly.
#include <QCursor>
#include <QDragMoveEvent>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTreeWidgetItem>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "FwFeatureTree.h"
#include "FwFeatureTreeDelegate.h"

namespace FreeWorksGui
{

namespace
{
// UI-SPEC § Spacing Scale: 22px row height, 16px indent. Pin the metrics now (the
// reference-CAD art / theming is Phase 7); palette/QStyle only, no inline stylesheet.
constexpr int kRowHeight = 22;
constexpr int kIndent = 16;

// The Body type-name contract literal, kept local to the implementation so the
// scope render reads the exact "PartDesign::Body" string here (D-07). It mirrors the
// header constant; both name the same upstream type with no PartDesign link.
constexpr const char* kBodyTypeNameLiteral = "PartDesign::Body";
}  // namespace

FwFeatureTree::FwFeatureTree(const char* name, QWidget* parent)
    : Gui::TreeWidget(name, parent)
{
    applySpikeMetrics();
    connectActiveBodySignals();
}

FwFeatureTree::~FwFeatureTree() = default;

bool FwFeatureTree::isBody(const App::DocumentObject* obj) const
{
    if (obj == nullptr) {
        return false;
    }
    // The whole Body identification: a single type-name string compare against the
    // "PartDesign::Body" literal. No PartDesign include, no module link, no C++-only
    // Body helper (D-07, Pitfall 1).
    return std::string(obj->getTypeId().getName()) == kBodyTypeNameLiteral;
}

void FwFeatureTree::setDocument(Gui::Document* doc)
{
    // The stock TreeWidget already subscribes to the Gui document/object signals in
    // its ctor and auto-populates the per-document DocumentItem + the nested object
    // items. We only pin which document the scope render targets; the items are owned
    // and built by the base (never by us — the item factory is non-virtual, Pitfall 2).
    m_scopedDocument = doc;
}

void FwFeatureTree::setActiveBody(App::DocumentObject* body)
{
    m_activeBody = (body != nullptr && isBody(body)) ? body : nullptr;
}

void FwFeatureTree::connectActiveBodySignals()
{
    Gui::Application* app = Gui::Application::Instance;
    if (app == nullptr) {
        return;
    }
    // Observe-the-DOM only (no App-layer link). On any of these Gui events the active
    // Body may have changed, so re-run the scope render. The active-Body resolution
    // itself stays on the pure isBody()/type-name path.
    app->signalActivatedObject.connect([this](const Gui::ViewProvider& vp) {
        const auto* vpd = dynamic_cast<const Gui::ViewProviderDocumentObject*>(&vp);
        if (vpd != nullptr && isBody(vpd->getObject())) {
            setActiveBody(vpd->getObject());
            scopeToActiveBody();
        }
    });
    app->signalInEdit.connect([this](const Gui::ViewProviderDocumentObject&) {
        scopeToActiveBody();
    });
    app->signalActiveDocument.connect([this](const Gui::Document& doc) {
        setDocument(const_cast<Gui::Document*>(&doc));
        scopeToActiveBody();
    });
}

App::DocumentObject* FwFeatureTree::objectOfItem(QTreeWidgetItem* item) const
{
    // Resolve the underlying App object via the PUBLIC DocumentObjectItem accessor
    // (Tree.h:495) — the same surface the base TreeWidget exposes. A DocumentItem (the
    // per-document top-level row) is not a DocumentObjectItem and has no App object.
    auto* objItem = dynamic_cast<Gui::DocumentObjectItem*>(item);
    if (objItem == nullptr) {
        return nullptr;
    }
    Gui::ViewProviderDocumentObject* vp = objItem->object();
    return vp != nullptr ? vp->getObject() : nullptr;
}

void FwFeatureTree::scopeItemRecursive(QTreeWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        App::DocumentObject* obj = objectOfItem(child);

        if (obj != nullptr && isBody(obj)) {
            // A Body row. Show it iff it is the active Body; hide its whole subtree
            // otherwise. setHidden() is the EXACT public-item API the base TreeWidget
            // uses (Tree.cpp:4574/6070) — scoping is a render, never a model change.
            const bool isActive = (m_activeBody != nullptr && obj == m_activeBody);
            child->setHidden(!isActive);
            // Descend regardless so a freshly-activated Body's previously-hidden
            // children are re-shown (the active subtree stays fully visible).
            if (isActive) {
                child->setHidden(false);
            }
        }
        // Keep descending: Bodies are children of the DocumentItem, and a Body's own
        // children (Origin, features, nested sketches) must stay visible under it.
        scopeItemRecursive(child);
    }
}

void FwFeatureTree::scopeToActiveBody()
{
    // Descend from invisibleRootItem() — the parent of the top-level Gui::DocumentItem
    // rows. The DocumentItem stays visible; only NON-active Body child subtrees are
    // hidden. We never iterate the top-level rows to find a Body (a Body is a CHILD of
    // the DocumentItem, not a top-level row, Tree.cpp:4562), and never touch item
    // construction or write any visibility property for presentation (D-06, Pitfall 2/3).
    if (m_activeBody == nullptr) {
        return;  // nothing active to scope to — leave the stock view untouched.
    }
    scopeItemRecursive(invisibleRootItem());
}

std::vector<App::DocumentObject*> FwFeatureTree::activeBodyGroup() const
{
    std::vector<App::DocumentObject*> out;
    if (m_activeBody == nullptr) {
        return out;
    }
    // Link-free read of the Body's ordered features via the generic GroupExtension
    // "Group" PropertyLinkList (GroupExtension.h:142) — getPropertyByName, no PartDesign
    // include. Solidness classification (by type-name string) is owned by Plan 03-02's
    // delegate / the rollback logic; this spike only proves the link-free read.
    const App::Property* prop = m_activeBody->getPropertyByName("Group");
    const auto* links = dynamic_cast<const App::PropertyLinkList*>(prop);
    if (links == nullptr) {
        return out;
    }
    for (App::DocumentObject* obj : links->getValues()) {
        out.push_back(obj);
    }
    return out;
}

void FwFeatureTree::drawRow(QPainter* painter,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const
{
    // Presentation seam reserved for Plan 03-03's rollback band. For the spike this is
    // a pure pass-through: the inherited rendering is unchanged.
    Gui::TreeWidget::drawRow(painter, option, index);
}

void FwFeatureTree::applySpikeMetrics()
{
    // Pin the UI-SPEC row metrics using the base TreeWidget API + indentation only.
    // NO inline stylesheet, NO hex color — Phase 7 owns theming; color comes from the
    // QPalette / native QStyle the base widget already uses.
    setIconHeight(kRowHeight);
    static_assert(kIndent == 16, "UI-SPEC § Spacing pins the FeatureManager indent to 16px");
    setIndentation(16);  // == kIndent; the UI-SPEC § Spacing 16px indent metric

    // Install the FeatureManager row delegate: the three origin planes render under
    // their reference-CAD display names (Front/Top/Right) display-only, and the
    // below-tip greying hook is palette-driven (Plan 03-03 sets the flag). The delegate
    // is parented to this tree so it resolves rows via itemFromIndex() (the stock
    // item-object path). setItemDelegate takes ownership of the delegate.
    setItemDelegate(new FwFeatureTreeDelegate(this));
}

QString FwFeatureTree::noActiveBodyText()
{
    // UI-SPEC § Copywriting empty state. tr() string, no trademark token.
    return tr("No active Body");
}

bool FwFeatureTree::isEmptyState() const
{
    // The empty state is shown exactly when no Body is active to scope to.
    return m_activeBody == nullptr;
}

void FwFeatureTree::dragMoveEvent(QDragMoveEvent* event)
{
    // Reuse the dependency-aware machinery VERBATIM: run the inherited validation FIRST.
    // Gui::TreeWidget::dragMoveEvent consults the ViewProvider drop/drag gates and sets
    // the event accept/ignore state (Tree.cpp:2348-2458). We never re-implement those
    // gates and never re-call them — we only read the resulting accept/ignore state below.
    Gui::TreeWidget::dragMoveEvent(event);

    if (!event->isAccepted()) {
        // The base IGNORED the drop (invalid target — e.g. child-before-parent). Decorate
        // the rejection: forbidden cursor + suppress the 2px insertion line. No
        // transaction opens (the base already declined) — allow-and-flag is never taken
        // (D-10/D-11/D-12; UI-SPEC § Interaction State Contract: invalid = no line +
        // ForbiddenCursor + event->ignore()).
        setCursor(Qt::ForbiddenCursor);
        setDropIndicatorShown(false);
        return;
    }

    // VALID target: leave the inherited 2px insertion line and the inherited
    // dropEvent/sortDroppedObjects single-transaction Group reorder untouched.
    unsetCursor();
    setDropIndicatorShown(true);
}

}  // namespace FreeWorksGui

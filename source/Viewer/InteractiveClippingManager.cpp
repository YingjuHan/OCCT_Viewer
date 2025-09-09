#include "InteractiveClippingManager.h"

#include <ViewBase/InteractiveClipping.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveClippingManager
// ================================================================
InteractiveClippingManager::InteractiveClippingManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateClipping
// ================================================================
void InteractiveClippingManager::CreateClipping(const ObjectId& theObjId, const int theViewId)
{
    static constexpr int invalidViewId1 = -1;
    static constexpr int invalidViewId2 = -2;

    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewBase::InteractiveClippingOwner) anOwner = new ViewBase::InteractiveClippingOwner(theObjId.c_str());
    Handle(ViewBase::InteractiveClipping)      aPrs    = new ViewBase::InteractiveClipping(anOwner);

    Handle(V3d_View) aView = myWebViewer->FocusView();
    if (theViewId != invalidViewId1 && theViewId != invalidViewId2) {
        const Handle(V3d_Viewer)& aViewer = myWebViewer->GetContext()->CurrentViewer();
        for (const Handle(V3d_View)& aViewIter : aViewer->DefinedViews()) {
            if (aViewIter->View()->Identification() == theViewId) {
                aView = aViewIter;
                break;
            }
        }
    }
    if (aView.IsNull()) {
        Message::SendFail() << "Error: CreateClipping (" << theObjId << ", " << theViewId
                            << ") called with invalid view";
        return;
    }

    aPrs->ViewAffinity()->SetVisible(false);
    aPrs->ViewAffinity()->SetVisible(aView->View()->Identification(), true);
    aView->SetClipPlanes(aPrs->ManagedPlanes());

    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : GetClippingCorner
// ================================================================
Vec3 InteractiveClippingManager::GetClippingCorner(const ObjectId& theObjId, bool theIsRel) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) ? Vec3(aPrs->GetClippingCorner(theIsRel).XYZ()) : Vec3(Graphic3d_Vec3d(0.0));
}

// ================================================================
// Function : SetClippingCorner
// ================================================================
void InteractiveClippingManager::SetClippingCorner(const ObjectId& theObjId, const Vec3& thePnt, bool theIsRel)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetClippingCorner (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetClippingCorner(thePnt.XYZ(), theIsRel);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    } else if (aPrs->ToBeUpdated(true)) {
        myWebViewer->GetContext()->RecomputeSelectionOnly(aPrs);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : GetClippingExtent
// ================================================================
Vec3 InteractiveClippingManager::GetClippingExtent(const ObjectId& theObjId, bool theIsRel) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) ? Vec3(aPrs->GetClippingExtent(theIsRel)) : Vec3(Graphic3d_Vec3d(0.0));
}

// ================================================================
// Function : SetClippingExtent
// ================================================================
void InteractiveClippingManager::SetClippingExtent(const ObjectId& theObjId, const Vec3& theExt, bool theIsRel)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetClippingExtent (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetClippingExtent(theExt.XYZ(), theIsRel);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    } else if (aPrs->ToBeUpdated(true)) {
        myWebViewer->GetContext()->RecomputeSelectionOnly(aPrs);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : GetClippingBox
// ================================================================
BndBox3 InteractiveClippingManager::GetClippingBox(const ObjectId& theObjId, bool theIsRel) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) ? BndBox3(aPrs->GetClippingBox(theIsRel)) : BndBox3();
}

// ================================================================
// Function : SetClippingBox
// ================================================================
void InteractiveClippingManager::SetClippingBox(const ObjectId& theObjId, const BndBox3& theBox, bool theIsRel)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetClippingBox (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetClippingBox(theBox, theIsRel);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    } else if (aPrs->ToBeUpdated(true)) {
        myWebViewer->GetContext()->RecomputeSelectionOnly(aPrs);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : GetNbPlanesXYZ
// ================================================================
Vec3 InteractiveClippingManager::GetNbPlanesXYZ(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) ? Vec3(Graphic3d_Vec3d(aPrs->GetNbPlanesXYZ())) : Vec3(Graphic3d_Vec3d(0.0));
}

// ================================================================
// Function : SetNbPlanesXYZ
// ================================================================
void InteractiveClippingManager::SetNbPlanesXYZ(const ObjectId& theObjId, const Vec3& theVec)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetNbPlanesXYZ (" << theObjId << ") called for non-existing object";
        return;
    }

    try {
        aPrs->SetNbPlanesXYZ(Graphic3d_Vec3i(theVec));
    } catch (const Standard_Failure&) {
        Message::SendFail() << "Error: SetNbPlanesXYZ (" << theObjId << ", " << theVec.x() << " " << theVec.y() << " "
                            << theVec.z() << ") called with invalid numbers";
        return;
    }

    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    } else if (aPrs->ToBeUpdated(true)) {
        myWebViewer->GetContext()->RecomputeSelectionOnly(aPrs);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : IsClippingOn
// ================================================================
bool InteractiveClippingManager::IsClippingOn(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) && aPrs->IsClippingOn();
}

// ================================================================
// Function : SetClippingOn
// ================================================================
void InteractiveClippingManager::SetClippingOn(const ObjectId& theObjId, const bool theToClip)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetClippingOn (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetClippingOn(theToClip);
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : ToFlipClipping
// ================================================================
bool InteractiveClippingManager::ToFlipClipping(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) && aPrs->ToFlipClipping();
}

// ================================================================
// Function : SetFlipClipping
// ================================================================
void InteractiveClippingManager::SetFlipClipping(const ObjectId& theObjId, const bool theToFlip)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetFlipClipping (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetFlipClipping(theToFlip);

    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    } else if (aPrs->ToBeUpdated(true)) {
        myWebViewer->GetContext()->RecomputeSelectionOnly(aPrs);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : IsCappingOn
// ================================================================
bool InteractiveClippingManager::IsCappingOn(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) && aPrs->IsCappingOn();
}

// ================================================================
// Function : SetCappingOn
// ================================================================
void InteractiveClippingManager::SetCappingOn(const ObjectId& theObjId, const bool theToUse)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetCappingOn (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetCappingOn(theToUse);
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : ToUseObjectMaterial
// ================================================================
bool InteractiveClippingManager::ToUseObjectMaterial(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) && aPrs->ToUseObjectMaterial();
}

// ================================================================
// Function : SetUseObjectMaterial
// ================================================================
void InteractiveClippingManager::SetUseObjectMaterial(const ObjectId& theObjId, const bool theToUse)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetUseObjectMaterial (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetUseObjectMaterial(theToUse);
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : IsShaded
// ================================================================
bool InteractiveClippingManager::IsShaded(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    return FindAisObject(aPrs, theObjId) && aPrs->DatumDisplayMode() == Prs3d_DM_Shaded;
}

// ================================================================
// Function : SetShaded
// ================================================================
void InteractiveClippingManager::SetShaded(const ObjectId& theObjId, const bool theToShade)
{
    Handle(ViewBase::InteractiveClipping) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetShaded (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetDatumDisplayMode(theToShade ? Prs3d_DM_Shaded : Prs3d_DM_WireFrame);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    }
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveClippingManager)
{
    emscripten::class_<InteractiveClippingManager, emscripten::base<IManager>>("InteractiveClippingManager")
        .function("createClipping", &InteractiveClippingManager::CreateClipping)
        .function("getClippingCorner", &InteractiveClippingManager::GetClippingCorner)
        .function("setClippingCorner", &InteractiveClippingManager::SetClippingCorner)
        .function("getClippingExtent", &InteractiveClippingManager::GetClippingExtent)
        .function("setClippingExtent", &InteractiveClippingManager::SetClippingExtent)
        .function("getClippingBox", &InteractiveClippingManager::GetClippingBox)
        .function("setClippingBox", &InteractiveClippingManager::SetClippingBox)
        .function("getNbPlanesXYZ", &InteractiveClippingManager::GetNbPlanesXYZ)
        .function("setNbPlanesXYZ", &InteractiveClippingManager::SetNbPlanesXYZ)
        .function("isClippingOn", &InteractiveClippingManager::IsClippingOn)
        .function("setClippingOn", &InteractiveClippingManager::SetClippingOn)
        .function("toFlipClipping", &InteractiveClippingManager::ToFlipClipping)
        .function("setFlipClipping", &InteractiveClippingManager::SetFlipClipping)
        .function("isCappingOn", &InteractiveClippingManager::IsCappingOn)
        .function("setCappingOn", &InteractiveClippingManager::SetCappingOn)
        .function("toUseObjectMaterial", &InteractiveClippingManager::ToUseObjectMaterial)
        .function("setUseObjectMaterial", &InteractiveClippingManager::SetUseObjectMaterial)
        .function("isShaded", &InteractiveClippingManager::IsShaded)
        .function("setShaded", &InteractiveClippingManager::SetShaded);
}
#endif

} // namespace


#include "InteractiveDimensionManager.h"

#include <ViewDim/InteractiveAngleDimension.h>
#include <ViewDim/InteractiveLengthDimension.h>
#include <ViewDim/InteractiveRadiusDimension.h>
#include <ViewDim/InteractiveDimensionOwner.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>
#include <gce_MakePln.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveDimensionManager
// ================================================================
InteractiveDimensionManager::InteractiveDimensionManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateLengthDimension
// ================================================================
void InteractiveDimensionManager::CreateLengthDimension(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewDim::InteractiveLengthDimension) aPrs = new ViewDim::InteractiveLengthDimension(
        new ViewDim::InteractiveDimensionOwner(theObjId.c_str()));
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : CreateRadiusDimension
// ================================================================
void InteractiveDimensionManager::CreateRadiusDimension(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewDim::InteractiveRadiusDimension) aPrs = new ViewDim::InteractiveRadiusDimension(
        new ViewDim::InteractiveDimensionOwner(theObjId.c_str()));
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : CreateAngleDimension
// ================================================================
void InteractiveDimensionManager::CreateAngleDimension(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewDim::InteractiveAngleDimension) aPrs = new ViewDim::InteractiveAngleDimension(
        new ViewDim::InteractiveDimensionOwner(theObjId.c_str()));
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : BuildLengthDimension
// ================================================================
bool InteractiveDimensionManager::BuildLengthDimension(const ObjectId& theObjId,
                                                       const Vec3&     thePnt1,
                                                       const Vec3&     thePnt2,
                                                       const Ax2&      thePln)
{
    Handle(ViewDim::InteractiveLengthDimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        CreateLengthDimension(theObjId);
        FindAisObject(aPrs, theObjId);
    }
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: BuildLengthDimension (" << theObjId << ") called for non-existing object";
        return false;
    }

    if (thePnt1.XYZ().IsEqual(thePnt2.XYZ(), Precision::Confusion())) {
        CreateLengthDimension(theObjId);
        FindAisObject(aPrs, theObjId);
        myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
        ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
        return false;
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);

    gp_Pln aPln(thePnt1.XYZ(), gp::DZ());
    try {
        if (thePln.HasLocation() && thePln.HasDirection()) {
            aPln = gp_Pln(thePln.GetAx2());
        } else if (thePln.HasLocation() && !thePln.HasDirection() && !thePnt1.IsEqual(thePnt2)
                   && !thePnt1.IsEqual(thePln.location) && !thePnt2.IsEqual(thePln.location)) {
            aPln = gce_MakePln(thePnt1.XYZ(), thePnt2.XYZ(), thePln.location.XYZ());
        } else if (!thePln.HasLocation() && thePln.HasDirection()) {
            aPln = gp_Pln(thePnt1.XYZ(), thePln.direction.XYZ());
        } else if (gp_Vec(thePnt1.XYZ(), thePnt2.XYZ()).IsParallel(aPln.Axis().Direction(), Precision::Angular())) {
            aPln = gp_Pln(thePnt1.XYZ(), gp::DY());
        }
        aPrs->SetMeasuredGeometry(thePnt1.XYZ(), thePnt2.XYZ(), aPln);
    } catch (const Standard_Failure& anEx) {
        Message::SendFail() << "Error: BuildLengthDimension (" << theObjId << ") cannot be constructed:" << anEx;
        return false;
    }
    return aPrs->IsValid();
}

// ================================================================
// Function : BuildRadiusDimension
// ================================================================
bool InteractiveDimensionManager::BuildRadiusDimension(const ObjectId& theObjId,
                                                       const Ax2&      theAx2,
                                                       const double    theRadius,
                                                       const Vec3&     theAnchor)
{
    Handle(ViewDim::InteractiveRadiusDimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        CreateRadiusDimension(theObjId);
        FindAisObject(aPrs, theObjId);
    }
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: BuildRadiusDimension (" << theObjId << ") called for non-existing object";
        return false;
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);

    if (!theAx2.HasLocation() || !theAx2.HasDirection() || Abs(theRadius) < Precision::Confusion()) {
        CreateRadiusDimension(theObjId);
        FindAisObject(aPrs, theObjId);
        myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
        ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
        return false;
    }

    gp_Circ aCirc(theAx2.GetAx2(), theRadius);
    aPrs->SetMeasuredGeometry(aCirc, theAnchor.XYZ(), !theAnchor.IsInfinite());
    return aPrs->IsValid();
}

// ================================================================
// Function : BuildAngleDimension
// ================================================================
bool InteractiveDimensionManager::BuildAngleDimension(const ObjectId& theObjId,
                                                      const Vec3&     thePnt1,
                                                      const Vec3&     thePnt2,
                                                      const Vec3&     thePnt3)
{
    Handle(ViewDim::InteractiveAngleDimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        CreateAngleDimension(theObjId);
        FindAisObject(aPrs, theObjId);
    }
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: BuildAngleDimension (" << theObjId << ") called for non-existing object";
        return false;
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);

    if (thePnt1.IsInfinite() || thePnt2.IsInfinite() || thePnt3.IsInfinite()) {
        CreateAngleDimension(theObjId);
        FindAisObject(aPrs, theObjId);
        myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
        ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
        return false;
    }

    aPrs->SetMeasuredGeometry(thePnt1.XYZ(), thePnt2.XYZ(), thePnt3.XYZ());
    return aPrs->IsValid();
}

// ================================================================
// Function : GetDimensionParams
// ================================================================
DimensionParams InteractiveDimensionManager::GetDimensionParams(const ObjectId& theObjId) const
{
    DimensionParams aParams;

    Handle(PrsDim_Dimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetDimensionParams (" << theObjId << ") called for non-existing object";
        return aParams;
    }

    aParams.InitFromPresentation(aPrs);
    return aParams;
}

// ================================================================
// Function : SetDimensionParams
// ================================================================
void InteractiveDimensionManager::SetDimensionParams(const ObjectId& theObjId, const DimensionParams& theParams)
{
    Handle(PrsDim_Dimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetDimensionParams (" << theObjId << ") called for non-existing object";
        return;
    }

    theParams.UpdatePresentation(aPrs);

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : GetDimensionTextPosition
// ================================================================
Vec3 InteractiveDimensionManager::GetDimensionTextPosition(const ObjectId& theObjId) const
{
    Handle(PrsDim_Dimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: GetDimensionTextPosition (" << theObjId << ") called for non-existing object";
        return Vec3::InvalidPoint();
    }

    return Vec3(aPrs->GetTextPosition().XYZ());
}

// ================================================================
// Function : SetDimensionTextPosition
// ================================================================
void InteractiveDimensionManager::SetDimensionTextPosition(const ObjectId& theObjId, const Vec3& thePnt)
{
    Handle(PrsDim_Dimension) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetDimensionTextPosition (" << theObjId << ") called for non-existing object";
        return;
    }

    aPrs->SetTextPosition(thePnt.XYZ());
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveDimensionManager)
{
    emscripten::class_<InteractiveDimensionManager, emscripten::base<IManager>>("InteractiveDimensionManager")
        .function("createLengthDimension", &InteractiveDimensionManager::CreateLengthDimension)
        .function("createRadiusDimension", &InteractiveDimensionManager::CreateRadiusDimension)
        .function("createAngleDimension", &InteractiveDimensionManager::CreateAngleDimension)
        .function("buildLengthDimension", &InteractiveDimensionManager::BuildLengthDimension)
        .function("buildRadiusDimension", &InteractiveDimensionManager::BuildRadiusDimension)
        .function("buildAngleDimension", &InteractiveDimensionManager::BuildAngleDimension)
        .function("getDimensionParams", &InteractiveDimensionManager::GetDimensionParams)
        .function("setDimensionParams", &InteractiveDimensionManager::SetDimensionParams)
        .function("getDimensionTextPosition", &InteractiveDimensionManager::GetDimensionTextPosition)
        .function("setDimensionTextPosition", &InteractiveDimensionManager::SetDimensionTextPosition);
}
#endif

} // namespace


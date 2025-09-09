#include "InteractiveTextLabelManager.h"

#include <ViewBase/InteractiveTextLabel.h>
#include <ViewBase/InteractiveTextLabelOwner.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveTextLabelManager
// ================================================================
InteractiveTextLabelManager::InteractiveTextLabelManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateTextLabel
// ================================================================
void InteractiveTextLabelManager::CreateTextLabel(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewBase::InteractiveTextLabel) aPrs =
        new ViewBase::InteractiveTextLabel(new ViewBase::InteractiveTextLabelOwner(theObjId.c_str()),
                                            myWebViewer->GetContext()->DefaultDrawer()->TextAspect());
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : GetLabelParams
// ================================================================
TextLabelParams InteractiveTextLabelManager::GetLabelParams(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveTextLabel) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        return TextLabelParams();
    }

    TextLabelParams aParams;
    aParams.text              = TCollection_AsciiString(aPrs->Text()).ToCString();
    aParams.position          = Vec3(aPrs->Position().XYZ());
    aParams.attachPoint       = Vec3(aPrs->AttachPoint().XYZ());
    aParams.attachMarker      = (int)aPrs->AttachMarker();
    aParams.attachMarkerScale = (int)aPrs->AttachPointAspect(true)->MarkerScale();
    aParams.pointPadding      = aPrs->PointPadding();
    aParams.toDrawRectangle   = aPrs->ToDrawRectangle();
    aParams.rectanglePadding  = aPrs->RectanglePadding();
    return aParams;
}

// ================================================================
// Function : SetLabelParams
// ================================================================
void InteractiveTextLabelManager::SetLabelParams(const ObjectId& theObjId, const TextLabelParams& theParams)
{
    Handle(ViewBase::InteractiveTextLabel) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetLabelParams (" << theObjId << ") called for non-existing object";
        return;
    } else if (theParams.attachMarker < Aspect_TOM_EMPTY || theParams.attachMarker >= Aspect_TOM_USERDEFINED) {
        Message::SendFail() << "Error: SetLabelParams (" << theObjId << ") invalid attachMarker type '"
                            << theParams.attachMarker << "'";
        return;
    }

    if (!aPrs->Position().IsEqual(theParams.position.XYZ(), Precision::Confusion())) {
        aPrs->SetPosition(theParams.position.XYZ());
        aPrs->SetToUpdate();
    }
    aPrs->SetAttachPoint(theParams.attachPoint.XYZ());

    const TCollection_ExtendedString aText(theParams.text.c_str(), true);
    if (!aPrs->Text().IsEqual(aText)) {
        aPrs->SetText(aText);
        aPrs->SetToUpdate();
    }
    aPrs->SetAttachMarker((Aspect_TypeOfMarker)theParams.attachMarker);
    aPrs->AttachPointAspect(false)->SetMarkerScale((float)theParams.attachMarkerScale);
    aPrs->AttachPointAspect(true)->SetMarkerScale((float)theParams.attachMarkerScale);
    aPrs->SetDrawRectangle(theParams.toDrawRectangle);
    aPrs->SetPointPadding(theParams.pointPadding);
    aPrs->SetRectanglePadding(theParams.rectanglePadding);

    if (theParams.icon.empty()) {
        aPrs->SetIcon(Handle(ViewBase::ScalableTexture2D)());
    } else if (aPrs->Icon().IsNull() || !aPrs->Icon()->ImagePath().IsEqual(theParams.icon.c_str())) {
        Handle(ViewBase::ScalablePixMap)    aPixmap  = new ViewBase::ScalablePixMap(theParams.icon.c_str());
        Handle(ViewBase::ScalableTexture2D) aTexture = new ViewBase::ScalableTexture2D(aPixmap);
        aPrs->SetIcon(aTexture);
    }

    aPrs->SynchronizeAspects();
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveTextLabelManager)
{
    emscripten::value_object<TextLabelParams>("TextLabelParams")
        .field("text", &TextLabelParams::text)
        .field("icon", &TextLabelParams::icon)
        .field("position", &TextLabelParams::position)
        .field("attachPoint", &TextLabelParams::attachPoint)
        .field("attachMarker", &TextLabelParams::attachMarker)
        .field("attachMarkerScale", &TextLabelParams::attachMarkerScale)
        .field("pointPadding", &TextLabelParams::pointPadding)
        .field("rectanglePadding", &TextLabelParams::rectanglePadding)
        .field("toDrawRectangle", &TextLabelParams::toDrawRectangle);

    emscripten::class_<InteractiveTextLabelManager, emscripten::base<IManager>>("InteractiveTextLabelManager")
        .function("createTextLabel", &InteractiveTextLabelManager::CreateTextLabel)
        .function("getLabelParams", &InteractiveTextLabelManager::GetLabelParams)
        .function("setLabelParams", &InteractiveTextLabelManager::SetLabelParams);
}
#endif

} // namespace


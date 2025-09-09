#include "InteractiveViewCubeManager.h"

#include <ViewBase/InteractiveViewCube.h>
#include <ViewBase/InteractiveViewCubeOwner.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveViewCubeManager
// ================================================================
InteractiveViewCubeManager::InteractiveViewCubeManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateViewCube
// ================================================================
void InteractiveViewCubeManager::CreateViewCube(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewBase::InteractiveViewCube) aPrs =
        new ViewBase::InteractiveViewCube(new ViewBase::InteractiveViewCubeOwner(theObjId.c_str()),
                                           myWebViewer->DefaultTextStyle());
    aPrs->SetViewAnimation(myWebViewer->ViewAnimation());
    aPrs->SetFixedAnimationLoop(false);
    aPrs->SetAutoStartAnimation(true);
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : GetViewCubeBaseSize
// ================================================================
float InteractiveViewCubeManager::GetViewCubeBaseSize(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveViewCube) aPrs;
    return FindAisObject(aPrs, theObjId) ? (float)aPrs->Size() : 0.0f;
}

// ================================================================
// Function : SetViewCubeBaseSize
// ================================================================
void InteractiveViewCubeManager::SetViewCubeBaseSize(const ObjectId& theObjId, const float theSize)
{
    Handle(ViewBase::InteractiveViewCube) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetViewCubeBaseSize (" << theObjId << ") called for non-existing object";
        return;
    } else if ((float)aPrs->Size() == theSize) {
        return;
    }

    aPrs->SetSize(theSize);
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// ================================================================
// Function : GetBoxSideLabel
// ================================================================
std::string InteractiveViewCubeManager::GetBoxSideLabel(const ObjectId& theObjId, const int theSide) const
{
    Handle(ViewBase::InteractiveViewCube) aPrs;
    return aPrs->IsBoxSide((V3d_TypeOfOrientation)theSide) && FindAisObject(aPrs, theObjId)
             ? std::string(aPrs->BoxSideLabel((V3d_TypeOfOrientation)theSide).ToCString())
             : std::string();
}

// ================================================================
// Function : SetBoxSideLabel
// ================================================================
void InteractiveViewCubeManager::SetBoxSideLabel(const ObjectId&    theObjId,
                                                 const int          theSide,
                                                 const std::string& theLabel)
{
    Handle(ViewBase::InteractiveViewCube) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetBoxSideLabel (" << theObjId << ", " << theSide
                            << ") called for non-existing object";
        return;
    } else if (!aPrs->IsBoxSide((V3d_TypeOfOrientation)theSide)) {
        Message::SendFail() << "Error: SetBoxSideLabel (" << theObjId << ", " << theSide
                            << ") called with invalid cube side";
        return;
    } else if (aPrs->BoxSideLabel((V3d_TypeOfOrientation)theSide).IsEqual(theLabel.c_str())) {
        return;
    }

    aPrs->SetBoxSideLabel((V3d_TypeOfOrientation)theSide, theLabel.c_str());
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveViewCubeManager)
{
    emscripten::class_<InteractiveViewCubeManager, emscripten::base<IManager>>("InteractiveViewCubeManager")
        .function("createViewCube", &InteractiveViewCubeManager::CreateViewCube)
        .function("getViewCubeBaseSize", &InteractiveViewCubeManager::GetViewCubeBaseSize)
        .function("setViewCubeBaseSize", &InteractiveViewCubeManager::SetViewCubeBaseSize)
        .function("getBoxSideLabel", &InteractiveViewCubeManager::GetBoxSideLabel)
        .function("setBoxSideLabel", &InteractiveViewCubeManager::SetBoxSideLabel);
}
#endif

} // namespace


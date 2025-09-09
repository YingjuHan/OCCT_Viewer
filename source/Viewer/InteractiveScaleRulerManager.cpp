#include "InteractiveScaleRulerManager.h"

#include <ViewBase/InteractiveScaleRuler.h>
#include <ViewBase/InteractiveScaleRulerOwner.h>

#include "InteractiveContext.h"
#include "WasmViewer.h"

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveScaleRulerManager
// ================================================================
InteractiveScaleRulerManager::InteractiveScaleRulerManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : CreateScaleRuler
// ================================================================
void InteractiveScaleRulerManager::CreateScaleRuler(const ObjectId& theObjId)
{
    InteractiveContext::RemoveObject(myWebViewer->GetContext(), theObjId);

    Handle(ViewBase::InteractiveScaleRuler) aPrs =
        new ViewBase::InteractiveScaleRuler(new ViewBase::InteractiveScaleRulerOwner(theObjId.c_str()),
                                             myWebViewer->DefaultTextStyle());
    aPrs->OnPixelScaleChange(aPrs, myWebViewer->DevicePixelRatio());
    myWebViewer->GetContext()->Display(aPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
}

// ================================================================
// Function : GetBaseSize
// ================================================================
Vec2 InteractiveScaleRulerManager::GetBaseSize(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveScaleRuler) aPrs;
    return FindAisObject(aPrs, theObjId) ? Vec2(Graphic3d_Vec2d(aPrs->BaseSize())) : Vec2();
}

// ================================================================
// Function : SetBaseSize
// ================================================================
void InteractiveScaleRulerManager::SetBaseSize(const ObjectId& theObjId, const Vec2& theSize)
{
    Handle(ViewBase::InteractiveScaleRuler) aPrs;
    if (!FindAisObject(aPrs, theObjId)) {
        Message::SendFail() << "Error: SetBaseSize (" << theObjId << ") called for non-existing object";
        return;
    } else if (aPrs->BaseSize() == Graphic3d_Vec2(theSize)) {
        return;
    }

    aPrs->SetBaseSize(Graphic3d_Vec2(theSize));
    if (aPrs->ToBeUpdated()) {
        myWebViewer->GetContext()->Redisplay(aPrs, false);
    }

    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(aPrs);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveScaleRulerManager)
{
    emscripten::class_<InteractiveScaleRulerManager, emscripten::base<IManager>>("InteractiveScaleRulerManager")
        .function("createScaleRuler", &InteractiveScaleRulerManager::CreateScaleRuler)
        .function("getBaseSize", &InteractiveScaleRulerManager::GetBaseSize)
        .function("setBaseSize", &InteractiveScaleRulerManager::SetBaseSize);
}
#endif

} // namespace


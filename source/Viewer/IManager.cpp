#include "IManager.h"

#include "WasmViewer.h"

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : IdsMapToString
// ================================================================
void IManager::IdsMapToString(std::string& theListStr, const TColStd_PackedMapOfInteger& theMap)
{
    for (TColStd_PackedMapOfInteger::Iterator anIndexIter(theMap); anIndexIter.More(); anIndexIter.Next()) {
        if (!theListStr.empty()) {
            theListStr += THE_LIST_SEPARATOR;
        }
        theListStr += std::to_string(anIndexIter.Key());
    }
}

// ================================================================
// Function : IdsMapFromString
// ================================================================
Handle(TColStd_HPackedMapOfInteger) IManager::IdsMapFromString(const std::string& theListStr)
{
    if (theListStr.empty()) {
        return Handle(TColStd_HPackedMapOfInteger)();
    }

    Handle(TColStd_HPackedMapOfInteger) aList = new TColStd_HPackedMapOfInteger();

    std::stringstream aStream(theListStr);
    std::string       aStrId;
    while (std::getline(aStream, aStrId, THE_LIST_SEPARATOR)) {
        int anElemId = std::atoi(aStrId.c_str());
        aList->ChangeMap().Add(anElemId);
    }
    return aList;
}

// ================================================================
// Function : IManager
// ================================================================
IManager::IManager(WasmViewer* theViewer) : myWebViewer(theViewer)
{
    //
}

// ================================================================
// Function : FindPrsOwner
// ================================================================
Handle(ViewBase::InteractiveOwner) IManager::FindPrsOwner(const ObjectId& theObjId) const
{
    return ViewBase::InteractiveOwner::FindPrsOwnerById(*myWebViewer->GetContext(), theObjId.c_str());
}

// ================================================================
// Function : FindAisObject
// ================================================================
Handle(AIS_InteractiveObject) IManager::FindAisObject(const ObjectId& theObjId) const
{
    return ViewBase::IInteractiveObject::FindAisObjectById(*myWebViewer->GetContext(), theObjId.c_str());
}

// ================================================================
// Function : findView
// ================================================================
Handle(V3d_View) IManager::findView(int theViewId) const
{
    if (theViewId == -1) {
        return myWebViewer->FocusView();
    }

    const Handle(V3d_Viewer)& aViewer = myWebViewer->GetContext()->CurrentViewer();
    for (const Handle(V3d_View)& aViewIter : aViewer->DefinedViews()) {
        if (aViewIter->View()->Identification() == theViewId) {
            return aViewIter;
        }
    }
    return Handle(V3d_View)();
}

// ================================================================
// Function : invalidateViewer
// ================================================================
void IManager::invalidateViewer(const Handle(AIS_InteractiveObject)& theObj)
{
    myWebViewer->GetContext()->MainPrsMgr()->ClearImmediateDraw();
    ViewBase::IInteractiveObject::InvalidateViewer(theObj);
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(IManager)
{
    emscripten::class_<IManager>("IManager");
}
#endif

} // namespace

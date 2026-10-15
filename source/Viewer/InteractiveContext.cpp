#include "InteractiveContext.h"

#include "WasmViewer.h"
#include "Utils/ViewCorner.h"

#include <algorithm>
#include <iomanip>
#include <unordered_set>
#include <sstream>
#include <vector>

#include <ViewShape/InteractiveShape.h>

#include <RWShapeConv/ShapeLoader.h>

#include <AIS_InteractiveContext.hxx>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : InteractiveContext
// ================================================================
InteractiveContext::InteractiveContext(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : generateId
// ================================================================
ObjectId InteractiveContext::generateId(const ObjectId& theBase) const
{
    return GenerateId(myWebViewer->GetContext(), theBase);
}

// ================================================================
// Function : GenerateId
// ================================================================
ObjectId InteractiveContext::GenerateId(const Handle(AIS_InteractiveContext)& theCtx, const ObjectId& theBase)
{
    NCollection_Map<TCollection_AsciiString, TCollection_AsciiString> aFilter;
    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*theCtx); anOwnIter.More(); anOwnIter.Next()) {
        const ViewBase::InteractiveOwner* anId = anOwnIter.Current();
        if (!anId->Id().IsEmpty()) {
            aFilter.Add(anId->Id());
        }
    }

    TCollection_AsciiString aBaseName(theBase.c_str());
    if (aBaseName.IsEmpty()) {
        aBaseName = "object";
    } else if (!aFilter.Contains(aBaseName)) {
        return theBase;
    }

    for (int aTry = 1; aTry < IntegerLast(); ++aTry) {
        TCollection_AsciiString aNewName = aBaseName + "_" + aTry;
        if (!aFilter.Contains(aNewName)) {
            return aNewName.ToCString();
        }
    }
    return ObjectId();
}

// ================================================================
// Function : IsAlive
// ================================================================
bool InteractiveContext::IsAlive(const ObjectId& theObjId) const
{
    return !FindPrsOwner(theObjId).IsNull();
}

// ================================================================
// Function : GetType
// ================================================================
std::string InteractiveContext::GetType(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (ViewBase::IInteractiveObject* anIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get())) {
        return anIObj->TypeName();
    } else if (!anObj.IsNull()) {
        return anObj->DynamicType()->Name();
    }
    return "undefined";
}

// ================================================================
// Function : IsLoaded
// ================================================================
bool InteractiveContext::IsLoaded(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
    return !anOwner.IsNull() && anOwner->IsLoaded();
}

// ================================================================
// Function : IsEnabled
// ================================================================
bool InteractiveContext::IsEnabled(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject)  anAisObj = FindAisObject(theObjId);
    ViewBase::IInteractiveObject* aPrs     = dynamic_cast<ViewBase::IInteractiveObject*>(anAisObj.get());
    return aPrs != nullptr && aPrs->IsEnabled();
}

// ================================================================
// Function : SetEnabled
// ================================================================
void InteractiveContext::SetEnabled(const ObjectId& theObjId, const bool theToEnable)
{
    Handle(AIS_InteractiveObject)  anObj = FindAisObject(theObjId);
    ViewBase::IInteractiveObject* aPrs  = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get());
    if (aPrs == nullptr) {
        Message::SendFail() << "Error: InteractiveContext::SetEnabled (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    if (aPrs->IsEnabled() == theToEnable) {
        return;
    }

    aPrs->SetEnabled(theToEnable);
    if (anObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed) {
        if (anObj->ToBeUpdated()) {
            myWebViewer->GetContext()->Redisplay(anObj, false);
        }
        invalidateViewer(anObj);
    }
}

// ================================================================
// Function : IsVisible
// ================================================================
bool InteractiveContext::IsVisible(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    return !anObj.IsNull() && anObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed;
}

// ================================================================
// Function : SetVisible
// ================================================================
void InteractiveContext::SetVisible(const ObjectId& theObjId, const bool theToShow)
{
    PrsMgr_DisplayStatus          aNewStatus = theToShow ? PrsMgr_DisplayStatus_Displayed : PrsMgr_DisplayStatus_Erased;
    Handle(AIS_InteractiveObject) anObj      = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetVisible (" << theObjId
                            << ") called for non-existing object";
        return;
    } else if (anObj->DisplayStatus() == aNewStatus) {
        return;
    }

    if (theToShow) {
        const bool toBeUpdated = anObj->ToBeUpdated(true);
        myWebViewer->GetContext()->Display(anObj, false);
        if (toBeUpdated) {
            // force selection update if hidden presentation was invalidated
            myWebViewer->GetContext()->RecomputeSelectionOnly(anObj);
        }
    } else {
        myWebViewer->GetContext()->Erase(anObj, false);
    }
    for (const Handle(PrsMgr_PresentableObject)& aChildIter : anObj->Children()) {
        Handle(AIS_InteractiveObject) aChildObj = Handle(AIS_InteractiveObject)::DownCast(aChildIter);
        if (theToShow) {
            myWebViewer->GetContext()->Display(aChildObj, false);
        } else {
            myWebViewer->GetContext()->Erase(aChildObj, false);
        }
    }

    myWebViewer->Invalidate2dObjects();
    invalidateViewer(anObj);
}

// ================================================================
// Function : SetVisibleMany
// ================================================================
void InteractiveContext::SetVisibleMany(const std::string& theObjIds, const bool theToShow)
{
    if (theObjIds.empty()) {
        return;
    }

    std::istringstream aStream(theObjIds);
    std::string        anId;
    while (std::getline(aStream, anId, THE_LIST_SEPARATOR)) {
        if (anId.empty()) {
            continue;
        }
        SetVisible(anId, theToShow);
    }
}

// ================================================================
// Function : SetVisibleOnly
// ================================================================
void InteractiveContext::SetVisibleOnly(const std::string& theObjIds)
{
    std::unordered_set<std::string> aVisibleIds;
    if (!theObjIds.empty()) {
        std::istringstream aStream(theObjIds);
        std::string        anId;
        while (std::getline(aStream, anId, THE_LIST_SEPARATOR)) {
            if (!anId.empty()) {
                aVisibleIds.insert(anId);
            }
        }
    }

    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (const ViewBase::InteractiveOwner* anId = dynamic_cast<ViewBase::InteractiveOwner*>(
                anObj->GetOwner().get())) {
            if (anId->Id().IsEmpty()) {
                continue;
            }
            const std::string aStrId = anId->Id().ToCString();
            SetVisible(aStrId, aVisibleIds.find(aStrId) != aVisibleIds.end());
        }
    }
}

// ================================================================
// Function : IsVisibleInView
// ================================================================
bool InteractiveContext::IsVisibleInView(const ObjectId& theObjId, const int theViewId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::IsVisibleInView (" << theObjId << ", " << theViewId
                            << ") called for non-existing object";
        return false;
    }

    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::IsVisibleInView (" << theObjId << ", " << theViewId
                            << ") called for non-existing view";
        return false;
    }

    if (anObj->DisplayStatus() != PrsMgr_DisplayStatus_Displayed) {
        return false;
    }

    Handle(Graphic3d_ViewAffinity) anAffinity = myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(
        anObj);
    return anAffinity->IsVisible(aView->View()->Identification());
}

// ================================================================
// Function : GetVisibleInView
// ================================================================
int InteractiveContext::GetVisibleInView(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::GetVisibleInView (" << theObjId
                            << ") called for non-existing object";
        return UNDEFINED_VIEW_ID;
    }

    Handle(Graphic3d_ViewAffinity) anAffinity = myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(
        anObj);
    int aNbVisible = 0;
    int aViewId    = -1;
    for (const Handle(V3d_View)& aViewIter : myWebViewer->MainView()->Viewer()->DefinedViews()) {
        if (anAffinity->IsVisible(aViewIter->View()->Identification())) {
            ++aNbVisible;
            if (aNbVisible == 1) {
                aViewId = aViewIter->View()->Identification();
            }
        }
    }
    return aNbVisible == 1 ? aViewId : UNDEFINED_VIEW_ID;
}

// ================================================================
// Function : SetVisibleInView
// ================================================================
void InteractiveContext::SetVisibleInView(const ObjectId& theObjId, const int theViewId)
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetVisibleInView (" << theObjId << ", " << theViewId
                            << ") called for non-existing object";
        return;
    }

    Handle(Graphic3d_ViewAffinity) anAffinity = myWebViewer->MainView()->Viewer()->StructureManager()->ObjectAffinity(
        anObj);
    if (theViewId == UNDEFINED_VIEW_ID) {
        for (const Handle(V3d_View)& aViewIter : myWebViewer->MainView()->Viewer()->DefinedViews()) {
            if (anObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed
                && !anAffinity->IsVisible(aViewIter->View()->Identification())) {
                aViewIter->Invalidate();
            }
            myWebViewer->GetContext()->SetViewAffinity(anObj, aViewIter, true);
        }

        // force visible state for all non-existing Views as well
        anAffinity->SetVisible(true);
        return;
    }

    Handle(V3d_View) aView = findView(theViewId);
    if (aView.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetVisibleInView (" << theObjId << ", " << theViewId
                            << ") called for non-existing view";
        return;
    }

    for (const Handle(V3d_View)& aViewIter : myWebViewer->MainView()->Viewer()->DefinedViews()) {
        if (aViewIter == aView) {
            continue;
        }

        if (anObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed
            && anAffinity->IsVisible(aViewIter->View()->Identification())) {
            aViewIter->Invalidate();
        }
        myWebViewer->GetContext()->SetViewAffinity(anObj, aViewIter, false);
    }

    if (anObj->DisplayStatus() == PrsMgr_DisplayStatus_Displayed
        && anAffinity->IsVisible(aView->View()->Identification())) {
        aView->Invalidate();
    }
    myWebViewer->GetContext()->SetViewAffinity(anObj, aView, true);

    // force invisible state for all non-existing Views
    // (warning, unsafe - should be done in sync with View creation code)
    anAffinity->SetVisible(false);
    anAffinity->SetVisible(aView->View()->Identification(), true);
}

// ================================================================
// Function : GetViewCorner
// ================================================================
int InteractiveContext::GetViewCorner(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::GetViewCorner (" << theObjId
                            << ") called for non-existing object";
        return UNDEFINED_VIEW_ID;
    }

    const Handle(Graphic3d_TransformPers)& aPers = anObj->TransformPersistence();
    if (aPers.IsNull() || !aPers->IsTrihedronOr2d()) {
        return static_cast<int>(ViewCorner::FREE3D);
    }
    return (int)Viewer::occToViewCorner(aPers->Corner2d());
}

// ================================================================
// Function : SetViewCorner
// ================================================================
void InteractiveContext::SetViewCorner(const ObjectId& theObjId, int theCorner)
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetVisibleInView (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    Handle(Graphic3d_TransformPers) aPers   = anObj->TransformPersistence();
    Aspect_TypeOfTriedronPosition   aCorner = occFromViewCorner((ViewCorner)theCorner);
    if (theCorner == static_cast<int>(ViewCorner::FREE3D)) {
        if (!aPers.IsNull()) {
            anObj->SetTransformPersistence(Handle(Graphic3d_TransformPers)());
            invalidateViewer(anObj);
        }
        return;
    } else if (theCorner < 0 || theCorner > static_cast<int>(ViewCorner::RIGHT_LOWER)) {
        Message::SendFail() << "Error: InteractiveContext::SetViewCorner() is called with invalid corner '" << theCorner
                            << "'";
        return;
    }

    if (aPers.IsNull() || !aPers->IsTrihedronOr2d()) {
        Graphic3d_Vec2d                anOffset;
        ViewBase::IInteractiveObject* anIIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get());
        if (anIIObj != nullptr) {
            anOffset = Graphic3d_Vec2d(anIIObj->Get2dViewOffset()) * anIIObj->DevicePixelRatio();
        }
        aPers = new Graphic3d_TransformPers(Graphic3d_TMF_2d, aCorner, Graphic3d_Vec2i(anOffset));
        anObj->SetTransformPersistence(aPers);
        invalidateViewer(anObj);
    } else if (aPers->Corner2d() != aCorner) {
        aPers->SetCorner2d(aCorner);
        invalidateViewer(anObj);
    }
}

// ================================================================
// Function : GetViewCornerOffset
// ================================================================
Vec2 InteractiveContext::GetViewCornerOffset(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::GetViewCornerOffset (" << theObjId
                            << ") called for non-existing object";
        return Vec2();
    }

    const Handle(Graphic3d_TransformPers)& aPers = anObj->TransformPersistence();
    return !aPers.IsNull() && aPers->IsTrihedronOr2d() ? Vec2(Graphic3d_Vec2d(aPers->Offset2d())) : Vec2();
}

// ================================================================
// Function : SetViewCornerOffset
// ================================================================
void InteractiveContext::SetViewCornerOffset(const ObjectId& theObjId, const Vec2& theOffset)
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetViewCornerOffset (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    Graphic3d_Vec2d                aNewOffset(theOffset);
    ViewBase::IInteractiveObject* anIIObj = dynamic_cast<ViewBase::IInteractiveObject*>(anObj.get());
    if (anIIObj != nullptr) {
        anIIObj->Set2dViewOffset(Graphic3d_Vec2i(aNewOffset));
        aNewOffset = aNewOffset * anIIObj->DevicePixelRatio();
    }

    Handle(Graphic3d_TransformPers) aPers = anObj->TransformPersistence();
    if (!aPers.IsNull() && aPers->IsTrihedronOr2d()) {
        aPers->SetOffset2d(Graphic3d_Vec2i(aNewOffset));
        invalidateViewer(anObj);
    }
}

// ================================================================
// Function : GetMinMax
// ================================================================
BndBox3 InteractiveContext::GetMinMax(const ObjectId& theObjId) const
{
    Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
    if (anObj.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::GetMinMax (" << theObjId
                            << ") called for non-existing object";
        return BndBox3();
    }

    Bnd_Box aBox;
    anObj->BoundingBox(aBox);
    return BndBox3(aBox);
}

// ================================================================
// Function : GetPriorityModifier
// ================================================================
int InteractiveContext::GetPriorityModifier(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
    if (anOwner.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::GetPriorityModifier (" << theObjId
                            << ") called for non-existing object";
        return 0;
    }
    return anOwner->PriorityModifier();
}

// ================================================================
// Function : SetPriorityModifier
// ================================================================
void InteractiveContext::SetPriorityModifier(const ObjectId& theObjId, const int theModifier)
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
    if (anOwner.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetPriorityModifier (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    if (anOwner->PriorityModifier() == theModifier) {
        return;
    }

    anOwner->SetPriorityModifier(theModifier);
    if (anOwner->HasSelectable()) {
        anOwner->Selectable()->SetToUpdate();
    }
}

// ================================================================
// Function : IsInfiniteState
// ================================================================
bool InteractiveContext::IsInfiniteState(const ObjectId& theObjId) const
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
    if (anOwner.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::IsInfiniteState (" << theObjId
                            << ") called for non-existing object";
        return false;
    }
    return anOwner->IsInfiniteState();
}

// ================================================================
// Function : SetInfiniteState
// ================================================================
void InteractiveContext::SetInfiniteState(const ObjectId& theObjId, const bool theIsInf)
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
    if (anOwner.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::SetInfiniteState (" << theObjId
                            << ") called for non-existing object";
        return;
    }

    if (anOwner->IsInfiniteState() == theIsInf) {
        return;
    }

    anOwner->SetInfiniteState(theIsInf);
    if (anOwner->HasSelectable()) {
        anOwner->Selectable()->SetToUpdate();
    }
}

// ================================================================
// Function : removeObject
// ================================================================
bool InteractiveContext::removeObject(const ObjectId& theObjId)
{
    const bool isRemoved = RemoveObject(myWebViewer->GetContext(), theObjId);
    if (isRemoved) {
        myWebViewer->Invalidate2dObjects();
    }
    return isRemoved;
}

// ================================================================
// Function : RemoveObject
// ================================================================
bool InteractiveContext::RemoveObject(const Handle(AIS_InteractiveContext)& theCtx, const ObjectId& theObjId)
{
    Handle(AIS_InteractiveObject) anObj = ViewBase::IInteractiveObject::FindAisObjectById(*theCtx, theObjId.c_str());
    if (anObj.IsNull()) {
        // @c Message::SendFail() << "Error: Remove (" << theObjId << ") called for non-existing object";
        return false;
    }

    return RemoveObject(theCtx, anObj);
}

// ================================================================
// Function : RemoveAll
// ================================================================
bool InteractiveContext::RemoveAll()
{
    AIS_ListOfInteractive anObjList;
    myWebViewer->GetContext()->ObjectsInside(anObjList);
    int aNbRemoved = 0;
    for (const Handle(AIS_InteractiveObject)& anObjIter : anObjList) {
        if (dynamic_cast<ViewBase::InteractiveOwner*>(anObjIter->GetOwner().get()) != nullptr) {
            ++aNbRemoved;
            myWebViewer->GetContext()->Remove(anObjIter, false);
        }
    }

    if (aNbRemoved >= 0) {
        myWebViewer->Invalidate2dObjects();
        myWebViewer->View()->Viewer()->Invalidate();
    }
    return aNbRemoved > 0;
}

// ================================================================
// Function : GetObjectsInside
// ================================================================
std::string InteractiveContext::GetObjectsInside(const std::string& theIdPrefix) const
{
    std::string aList;
    const bool  hasNamePrefix = !theIdPrefix.empty() && theIdPrefix != "*";

    const TCollection_AsciiString aNamePrefix(theIdPrefix.c_str());
    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (const ViewBase::InteractiveOwner* anId = dynamic_cast<ViewBase::InteractiveOwner*>(
                anObj->GetOwner().get())) {
            if (hasNamePrefix && !anId->Id().StartsWith(aNamePrefix)) {
                continue;
            }

            if (!aList.empty()) {
                aList += "\n";
            }
            aList += anId->Id().ToCString();
        }
    }
    return aList;
}

namespace {
std::string jsonEscape(const std::string& theStr)
{
    std::ostringstream aRes;
    for (const unsigned char aCh : theStr) {
        switch (aCh) {
            case '\\': aRes << "\\\\"; break;
            case '"': aRes << "\\\""; break;
            case '\b': aRes << "\\b"; break;
            case '\f': aRes << "\\f"; break;
            case '\n': aRes << "\\n"; break;
            case '\r': aRes << "\\r"; break;
            case '\t': aRes << "\\t"; break;
            default: {
                if (aCh < 0x20) {
                    aRes << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)aCh << std::dec;
                } else {
                    aRes << (char)aCh;
                }
                break;
            }
        }
    }
    return aRes.str();
}
} // namespace

// ================================================================
// Function : GetSceneGraphJson
// ================================================================
std::string InteractiveContext::GetSceneGraphJson(const std::string& theIdPrefix) const
{
    std::vector<std::string> anIds;
    const bool               hasNamePrefix = !theIdPrefix.empty() && theIdPrefix != "*";
    const TCollection_AsciiString aNamePrefix(theIdPrefix.c_str());

    for (AIS_DataMapOfIOStatus::Iterator anObjIter = myWebViewer->GetContext()->ObjectIterator(); anObjIter.More();
         anObjIter.Next()) {
        const Handle(AIS_InteractiveObject)& anObj = anObjIter.Key();
        if (const ViewBase::InteractiveOwner* anId = dynamic_cast<ViewBase::InteractiveOwner*>(
                anObj->GetOwner().get())) {
            if (anId->Id().IsEmpty()) {
                continue;
            }
            if (hasNamePrefix && !anId->Id().StartsWith(aNamePrefix)) {
                continue;
            }
            anIds.emplace_back(anId->Id().ToCString());
        }
    }

    std::sort(anIds.begin(), anIds.end());
    anIds.erase(std::unique(anIds.begin(), anIds.end()), anIds.end());

    std::ostringstream aJson;
    aJson << std::setprecision(17);

    aJson << "{\"specVersion\":\"0.1\",\"rootNodeId\":\"root\",\"nodes\":[";

    // Root node
    aJson << "{\"id\":\"root\",\"name\":\"root\",\"type\":\"Root\",\"parentId\":null,\"children\":[";
    for (size_t anIdIter = 0; anIdIter < anIds.size(); ++anIdIter) {
        if (anIdIter != 0) {
            aJson << ",";
        }
        aJson << "\"" << jsonEscape(anIds[anIdIter]) << "\"";
    }
    aJson << "],\"bbox\":null}";

    // Object nodes (flat tree for now: root -> objects)
    for (const std::string& anObjId : anIds) {
        const std::string aType = GetType(anObjId);
        const BndBox3     aBox  = GetMinMax(anObjId);

        aJson << ",{\"id\":\"" << jsonEscape(anObjId) << "\""
              << ",\"name\":\"" << jsonEscape(anObjId) << "\""
              << ",\"type\":\"" << jsonEscape(aType) << "\""
              << ",\"parentId\":\"root\""
              << ",\"children\":[]"
              << ",\"bbox\":";

        if (aBox.IsValid()) {
            const Vec3 aMin = aBox.GetCornerMin();
            const Vec3 aMax = aBox.GetCornerMax();
            aJson << "{\"min\":[" << aMin.x() << "," << aMin.y() << "," << aMin.z() << "],"
                  << "\"max\":[" << aMax.x() << "," << aMax.y() << "," << aMax.z() << "]}";
        } else {
            aJson << "null";
        }
        aJson << "}";
    }

    aJson << "]}";
    return aJson.str();
}

// ================================================================
// Function : GetLastDraggedObject
// ================================================================
std::string InteractiveContext::GetLastDraggedObject() const
{
    const Handle(SelectMgr_EntityOwner)& anObj = myWebViewer->LastDraggedOwner();
    if (anObj.IsNull() || anObj->Selectable()->DisplayStatus() != PrsMgr_DisplayStatus_Displayed) {
        return std::string();
    }

    const ViewBase::InteractiveOwner* anId = dynamic_cast<ViewBase::InteractiveOwner*>(anObj.get());
    return anId != nullptr ? std::string(anId->Id().ToCString()) : std::string();
}

// ================================================================
// Function : Clone
// ================================================================
ObjectId InteractiveContext::Clone(const ObjectId& theObjIdOld, const ObjectId& theObjIdNew)
{
    Handle(ViewBase::InteractiveOwner) anOldOwner = FindPrsOwner(theObjIdOld);
    if (anOldOwner.IsNull()) {
        Message::SendFail() << "Error: InteractiveContext::Clone (" << theObjIdOld << ", " << theObjIdNew
                            << ") called for non-existing object";
        return ObjectId();
    }
    if (theObjIdOld == theObjIdNew) {
        Message::SendFail() << "Error: InteractiveContext::Clone (" << theObjIdOld << ", " << theObjIdNew
                            << ") called with invalid new id";
        return ObjectId();
    }

    ObjectId aNewId = theObjIdNew;
    if (aNewId.empty()) {
        aNewId = generateId(theObjIdOld);
    } else {
        removeObject(aNewId);
    }

    Handle(ViewBase::InteractiveOwner) aClone = anOldOwner->Clone(aNewId.c_str());
    if (aClone.IsNull()) {
        Message::SendFail() << "Error: Clone (" << theObjIdOld << ", " << theObjIdNew
                            << ") called for non-clonable object";
        return ObjectId();
    }

    Handle(AIS_InteractiveObject) anAisPrs = aClone->CreatePresentation();
    if (anAisPrs.IsNull()) {
        Message::SendFail() << "Error: Clone (" << theObjIdOld << ", " << theObjIdNew
                            << ") called for non-displayable object";
        return ObjectId();
    }

    if (ViewBase::IInteractiveObject* anSCPrs = dynamic_cast<ViewBase::IInteractiveObject*>(anAisPrs.get())) {
        anSCPrs->OnPixelScaleChange(anAisPrs, myWebViewer->DevicePixelRatio());
    }

    myWebViewer->GetContext()->Display(anAisPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);
    return aNewId;
}

// ================================================================
// Function : loadData
// ================================================================
bool InteractiveContext::loadData(const ObjectId& theObjId, RWBase::IPromise* theCallback)
{
    return LoadData(myWebViewer->GetContext(), theObjId, theCallback);
}

// ================================================================
// Function : LoadData
// ================================================================
bool InteractiveContext::LoadData(const Handle(AIS_InteractiveContext)& theCtx,
                                  const ObjectId&                       theObjId,
                                  RWBase::IPromise*                   theCallback)
{
    Handle(ViewShape::InteractiveShape) aShape;
    if (ViewBase::IInteractiveObject::FindObjectById(aShape, *theCtx, theObjId.c_str())) {
        return RWShapeConv::ShapeLoader::LoadShape(aShape->MeshOwner()->ShapeData(), theCallback);
    }

    return RWShapeConv::ShapeLoader::LoadShape(Handle(RWShape::ShapeData)(),
                                                 theCallback); // just to report error
}

// ================================================================
// Function : RemoveObject
// ================================================================
bool InteractiveContext::RemoveObject(const Handle(AIS_InteractiveContext)& theCtx,
                                      const Handle(AIS_InteractiveObject)&  theObject)
{
    if (theObject.IsNull()) {
        return false;
    }

    ViewBase::IInteractiveObject::InvalidateViewer(theObject);
    for (const Handle(PrsMgr_PresentableObject)& aChildIter : theObject->Children()) {
        Handle(AIS_InteractiveObject) aChildObj = Handle(AIS_InteractiveObject)::DownCast(aChildIter);
        theCtx->Remove(aChildObj, false);
    }
    theCtx->Remove(theObject, false);
    return true;
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(InteractiveContext)
{
    emscripten::register_vector<float>("FloatVector");

    emscripten::class_<InteractiveContext, emscripten::base<IManager>>("InteractiveContext")
        .function("isAlive", &InteractiveContext::IsAlive)
        .function("getType", &InteractiveContext::GetType)
        .function("isLoaded", &InteractiveContext::IsLoaded)
        .function("isEnabled", &InteractiveContext::IsEnabled)
        .function("setEnabled", &InteractiveContext::SetEnabled)
        .function("isVisible", &InteractiveContext::IsVisible)
        .function("setVisible", &InteractiveContext::SetVisible)
        .function("setVisibleMany", &InteractiveContext::SetVisibleMany)
        .function("setVisibleOnly", &InteractiveContext::SetVisibleOnly)
        .function("isVisibleInView", &InteractiveContext::IsVisibleInView)
        .function("getVisibleInView", &InteractiveContext::GetVisibleInView)
        .function("setVisibleInView", &InteractiveContext::SetVisibleInView)
        .function("getViewCorner", &InteractiveContext::GetViewCorner)
        .function("setViewCorner", &InteractiveContext::SetViewCorner)
        .function("getViewCornerOffset", &InteractiveContext::GetViewCornerOffset)
        .function("setViewCornerOffset", &InteractiveContext::SetViewCornerOffset)
        .function("getMinMax", &InteractiveContext::GetMinMax)
        .function("getPriorityModifier", &InteractiveContext::GetPriorityModifier)
        .function("setPriorityModifier", &InteractiveContext::SetPriorityModifier)
        .function("isInfiniteState", &InteractiveContext::IsInfiniteState)
        .function("setInfiniteState", &InteractiveContext::SetInfiniteState)
        .function("remove", &InteractiveContext::removeObject)
        .function("removeAll", &InteractiveContext::RemoveAll)
        .function("loadData", &InteractiveContext::loadData, emscripten::allow_raw_pointers())
        .function("clone", &InteractiveContext::Clone)
        .function("generateId", &InteractiveContext::generateId)
        .function("getObjectsInside", &InteractiveContext::GetObjectsInside)
        .function("getSceneGraphJson", &InteractiveContext::GetSceneGraphJson)
        .function("getLastDraggedObject", &InteractiveContext::GetLastDraggedObject);
}
#endif

} // namespace


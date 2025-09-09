#include "SelectionManager.h"

#include "WasmViewer.h"

#include <ViewCae/InteractiveCaeMeshOwner.h>

#include <ViewShape/InteractiveShapeOwner.h>

#include <RWShapeConv/ShapeLoader.h>

#include <AIS_InteractiveObject.hxx>
#include <Message.hxx>
#include <TColStd_HPackedMapOfInteger.hxx>

#include <list>

#ifdef __EMSCRIPTEN__
    #include <emscripten/bind.h>
#endif

namespace Viewer {

// ================================================================
// Function : SelectionManager
// ================================================================
SelectionManager::SelectionManager(WasmViewer* theViewer) : IManager(theViewer)
{
    //
}

// ================================================================
// Function : SetAllowMultiselection
// ================================================================
void SelectionManager::SetAllowMultiselection(bool theIsMultiselection)
{
    if (myToAllowMultiselection != theIsMultiselection) {
        myToAllowMultiselection = theIsMultiselection;
        myWebViewer->ConfigureFocusView();
    }
}

// ================================================================
// Function : GetLastPickedPoint
// ================================================================
Viewer::Vec3 SelectionManager::GetLastPickedPoint(const Viewer::ObjectId& theId) const
{
    const Handle(StdSelect_ViewerSelector3d)& aSelector  = myWebViewer->GetContext()->MainSelector();
    const Handle(SelectMgr_EntityOwner)&      aPickOwner = myWebViewer->GetContext()->DetectedOwner();
    if (aSelector->NbPicked() == 0 || aPickOwner.IsNull()) {
        return Viewer::Vec3::InvalidPoint();
    }

    const TCollection_AsciiString anObjId = theId != "*" ? theId.c_str() : "";
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        if (!anObjId.IsEmpty()) {
            Handle(ViewBase::InteractiveOwner) aPrsId = Handle(ViewBase::InteractiveOwner)::DownCast(
                aSelector->Picked(aPickIter));
            if (!aPrsId.IsNull() && aPrsId->Id() == anObjId) {
                const gp_Pnt aPnt = aSelector->PickedPoint(aPickIter);
                return Viewer::Vec3(aPnt.XYZ());
            }
        } else if (aSelector->Picked(aPickIter) == aPickOwner) {
            const gp_Pnt aPnt = aSelector->PickedPoint(aPickIter);
            return Viewer::Vec3(aPnt.XYZ());
        }
    }
    return Viewer::Vec3::InvalidPoint();
}

// ================================================================
// Function : GetSelectedObjects
// ================================================================
std::string SelectionManager::GetSelectedObjects() const
{
    std::string aList;
    for (const Handle(SelectMgr_EntityOwner)& aSelOwner : myWebViewer->GetContext()->Selection()->Objects()) {
        Handle(ViewBase::InteractiveOwner) anIOwner = Handle(ViewBase::InteractiveOwner)::DownCast(aSelOwner);
        Handle(SelectMgr_SelectableObject)  aSelObj  = aSelOwner->Selectable();
        if (aSelObj.IsNull() || anIOwner.IsNull()) {
            continue;
        }

        if (!aList.empty()) {
            aList += THE_LIST_SEPARATOR;
        }
        aList += anIOwner->Id().ToCString();
    }
    return aList;
}

//! Map of strings
using StringNMap =
    NCollection_IndexedDataMap<TCollection_AsciiString, Handle(SelectMgr_EntityOwner), TCollection_AsciiString>;

//! Parse ids from string.
static void parseIdsInString(StringNMap& theIdMap, bool& theToSelectAll, const std::string& theNames)
{
    std::stringstream aListStream(theNames);
    std::string       aName;
    while (std::getline(aListStream, aName, IManager::THE_LIST_SEPARATOR)) {
        if (aName == "*") {
            theToSelectAll = true;
            theIdMap.Clear();
            return;
        }

        theIdMap.Add(TCollection_AsciiString(aName.c_str()), Handle(SelectMgr_EntityOwner)());
    }
}

//! Select owners starting with specified id.
static void selectUpLevel(const StringNMap& theIdMap, const Handle(AIS_InteractiveContext)& theCtx)
{
    std::vector<TCollection_AsciiString> aPrefixList;
    aPrefixList.reserve(theIdMap.Extent());
    for (StringNMap::Iterator anIdIter(theIdMap); anIdIter.More(); anIdIter.Next()) {
        if (anIdIter.Value().IsNull()) {
            aPrefixList.push_back(anIdIter.Key() + RWShapeConv::ShapeLoader::THE_ID_SEPARATOR);
        }
    }

    StringNMap aHandledMap;
    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*theCtx); anOwnIter.More(); anOwnIter.Next()) {
        ViewBase::InteractiveOwner* aCurrId = anOwnIter.Current();
        for (const TCollection_AsciiString& anIdIter : aPrefixList) {
            if (!aCurrId->Id().StartsWith(anIdIter)) {
                continue;
            }
            if (!aCurrId->IsLoaded()) {
                Message::SendFail() << "Error: Can't select object '" << aCurrId->Id()
                                    << "', probably it isn't loaded correctly";
                continue;
            }

            theCtx->SetSelectedState(aCurrId, true);
            aHandledMap.Add(anIdIter, aCurrId);
            break;
        }
    }

    if (aHandledMap.Extent() == (int)aPrefixList.size()) {
        return;
    }

    TCollection_AsciiString anIdsString;
    for (StringNMap::Iterator anIdIter(theIdMap); anIdIter.More(); anIdIter.Next()) {
        const TCollection_AsciiString& anId = anIdIter.Key();
        if (aHandledMap.Contains(anId + RWShapeConv::ShapeLoader::THE_ID_SEPARATOR)) {
            continue;
        }

        if (!anIdsString.IsEmpty()) {
            anIdsString += " ";
        }
        anIdsString += TCollection_AsciiString("'") + anId + "'";
    }

    Message::SendFail() << "Error: Cannot find objects " << anIdsString;
}

// ================================================================
// Function : SetSelectedObjects
// ================================================================
void SelectionManager::SetSelectedObjects(const std::string& theNames, bool theToLevelUp)
{
    bool toSelectAll = false;

    StringNMap anIdMap;
    parseIdsInString(anIdMap, toSelectAll, theNames);

    // Avoid extra calculations on clear selection logic when nothing selected.
    if (myWebViewer->GetContext()->NbSelected() == 0 && anIdMap.IsEmpty() && !toSelectAll) {
        return;
    }

    myWebViewer->GetContext()->ClearSelected(false);

    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*myWebViewer->GetContext()); anOwnIter.More();
         anOwnIter.Next()) {
        ViewBase::InteractiveOwner* aCurrId = anOwnIter.Current();

        // try to find current id in list
        const int anIndex = anIdMap.FindIndex(aCurrId->Id());
        if (anIndex == 0 && !toSelectAll) {
            continue;
        }
        if (!aCurrId->IsLoaded()) {
            Message::SendFail() << "Error: Can't select object '" << aCurrId->Id()
                                << "', probably it isn't loaded correctly";
            continue;
        }

        if (anIndex != 0) {
            anIdMap.ChangeFromIndex(anIndex) = aCurrId;
        } else {
            myWebViewer->GetContext()->SetSelectedState(aCurrId, true);
        }
    }

    int aNbVirt = 0;
    for (StringNMap::Iterator anOwnerIter(anIdMap); anOwnerIter.More(); anOwnerIter.Next()) {
        if (!anOwnerIter.Value().IsNull()) {
            myWebViewer->GetContext()->SetSelectedState(anOwnerIter.Value(), true);
        } else if (theToLevelUp) {
            ++aNbVirt;
        } else {
            Message::SendFail() << "Error: object '" << anOwnerIter.Key() << "' not found for selection";
        }
    }

    if (aNbVirt != 0 && theToLevelUp) {
        // find virtual assemblies
        selectUpLevel(anIdMap, myWebViewer->GetContext());
    }

    myWebViewer->GetContext()->MainSelector()->Clear();
    myWebViewer->OnSelectionChanged(myWebViewer->GetContext(), myWebViewer->FocusView());
}

// ================================================================
// Function : getSelectionMode
// ================================================================
int SelectionManager::getSelectionMode(const Viewer::ObjectId& theId) const
{
    Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theId);
    if (anOwner.IsNull()) {
        return 0;
    }

    int aSelMode = anOwner->GetSelectionMode();
    if (Handle(ViewCae::InteractiveCaeMeshOwner)::DownCast(anOwner)) {
        aSelMode &= ~ViewCae::InteractiveCaeMeshAuxSelModeMask;
    }
    return aSelMode;
}

// ================================================================
// Function : setSelectionMode
// ================================================================
void SelectionManager::setSelectionMode(const std::string& theNames, const int theMode)
{
    std::list<std::string> anIdsList;
    // parse ids
    {
        std::stringstream aListStream(theNames);
        std::string       aName;
        while (std::getline(aListStream, aName, THE_LIST_SEPARATOR)) {
            anIdsList.push_back(aName);
        }
    }
    if (anIdsList.empty()) {
        return;
    }

    myWebViewer->GetContext()->ClearSelected(false);
    for (ViewBase::InteractiveOwner::Iterator anOwnIter(*myWebViewer->GetContext()); anOwnIter.More();
         anOwnIter.Next()) {
        ViewBase::InteractiveOwner* aCurrOwner = anOwnIter.Current();

        // try to find current id in list
        for (const std::string& anId : anIdsList) {
            bool toSetMode = (aCurrOwner->Id().Length() >= (int)anId.size()
                              && memcmp(aCurrOwner->Id().ToCString(), anId.c_str(), anId.size()) == 0)
                          || anId == "*";
            if (!toSetMode) {
                continue;
            }

            int aMode = theMode;
            if (dynamic_cast<ViewCae::InteractiveCaeMeshOwner*>(aCurrOwner) != nullptr) {
                aMode |= aCurrOwner->GetSelectionMode() & ViewCae::InteractiveCaeMeshAuxSelModeMask;
            }

            if (!aCurrOwner->AcceptSelectionMode(aMode)) {
                Message::SendFail() << "Error: Can't set mode '" << aMode << "' for object '" << anId << "'";
                continue;
            }
            aCurrOwner->SetSelectionMode(aMode);
            if (anId != "*") {
                break;
            }
        }
    }
}

// ================================================================
// Function : getLastDetectedObject
// ================================================================
SelectionManager::DetectedObjectInfo SelectionManager::getLastDetectedObject() const
{
    DetectedObjectInfo anInfo;

    if (!myWebViewer->GetContext()->HasDetected()) {
        return anInfo;
    }

    Handle(ViewBase::InteractiveOwner) aPickOwner = Handle(ViewBase::InteractiveOwner)::DownCast(
        myWebViewer->GetContext()->DetectedOwner());
    if (aPickOwner.IsNull()) {
        // handle native AIS presentations with externally set owner
        aPickOwner = Handle(ViewBase::InteractiveOwner)::DownCast(
            myWebViewer->GetContext()->DetectedInteractive()->GetOwner());
    }

    if (aPickOwner.IsNull()) {
        return anInfo;
    }

    anInfo.objectId  = aPickOwner->Id().ToCString();
    anInfo.partId    = aPickOwner->GetSelectionPart().Id;
    anInfo.subPartId = aPickOwner->GetSelectionPart().SubId;

    const Handle(StdSelect_ViewerSelector3d)& aSelector = myWebViewer->GetContext()->MainSelector();
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        if (aSelector->Picked(aPickIter) == myWebViewer->GetContext()->DetectedOwner()) {
            const SelectMgr_SortCriterion& aPickedData = aSelector->PickedData(aPickIter);

            anInfo.pickedPoint  = Vec3(aPickedData.Point.XYZ());
            anInfo.pickedNormal = Vec3(Graphic3d_Vec3d(aPickedData.Normal));
            break;
        }
    }

    return anInfo;
}

// ================================================================
// Function : getDetectedShapeElem
// ================================================================
DetectedShapeElem SelectionManager::getDetectedShapeElem(const Viewer::ObjectId& theId) const
{
    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: getDetectedShapeElem(" << theId << ") is called for non-existing shape";
        return DetectedShapeElem();
    }

    const Handle(StdSelect_ViewerSelector3d)& aSelector = myWebViewer->GetContext()->MainSelector();
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        if (aSelector->Picked(aPickIter) == aShapeOwner) {
            if (!aShapeOwner->GetLastDetectedElem().IsValid()) {
                return DetectedShapeElem();
            }

            DetectedShapeElem anInfo;
            anInfo.entityId   = aShapeOwner->GetLastDetectedElem().Index;
            anInfo.entityType = (int)ViewShape::ShapeElemTypeToSelMode(aShapeOwner->GetLastDetectedElem().Type);
            return anInfo;
        }
    }

    return DetectedShapeElem();
}

// ================================================================
// Function : getSelectedShapeElems
// ================================================================
std::string SelectionManager::getSelectedShapeElems(const Viewer::ObjectId& theId, const int theElemType) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: getSelectedShapeElems(" << theId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: getSelectedShapeElems(" << theId << ", " << theElemType
                            << ") is called for non-existing object";
        return std::string();
    }
    if (!myWebViewer->GetContext()->IsSelected(aShapeOwner)) {
        return std::string();
    }

    const Handle(TColStd_HPackedMapOfInteger)& anIndexMap = aShapeOwner->GetSelectedElemsByType(anElemType);
    if (!anIndexMap.IsNull()) {
        std::string aRes;
        IdsMapToString(aRes, anIndexMap);
        return aRes;
    }
    return std::string();
}

// ================================================================
// Function : getDisabledShapeElems
// ================================================================
std::string SelectionManager::getDisabledShapeElems(const Viewer::ObjectId& theId,
                                                    int                          theElemType,
                                                    bool                         theToReverse) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: getDisabledShapeElems(" << theId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: getDisabledShapeElems(" << theId << ", " << theElemType
                            << ") is called for non-existing object";
        return std::string();
    }

    const Handle(TColStd_HPackedMapOfInteger)& aDisIndexMap = aShapeOwner->GetDisabledElemsByType(anElemType);
    if (!theToReverse) {
        if (aDisIndexMap.IsNull()) {
            return std::string();
        }

        std::string aRes;
        IdsMapToString(aRes, aDisIndexMap);
        return aRes;
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap = aShapeOwner->GetSubshapesIndexes(anElemType, false);
    if (anIndexMap.IsNull() || anIndexMap->Map().IsEmpty()) {
        return std::string();
    } else if (aDisIndexMap.IsNull() || aDisIndexMap->Map().IsEmpty()) {
        std::string aRes;
        IdsMapToString(aRes, anIndexMap);
        return aRes;
    }

    TColStd_PackedMapOfInteger aResMap;
    aResMap.Subtraction(anIndexMap->Map(), aDisIndexMap->Map());

    std::string aRes;
    IdsMapToString(aRes, aResMap);
    return aRes;
}

// ================================================================
// Function : setDisabledShapeElems
// ================================================================
void SelectionManager::setDisabledShapeElems(const Viewer::ObjectId& theId,
                                             int                          theElemType,
                                             bool                         theToReverse,
                                             const std::string&           theElems)
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: setDisabledShapeElems(" << theId << ", " << theElemType
                            << ") is called with invalid subshape type";
        return;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: setDisabledShapeElems(" << theId << "," << theElemType
                            << ") is called for non-existing shape";
        return;
    }

    Handle(TColStd_HPackedMapOfInteger)& aDisIndexMap = aShapeOwner->ChangeDisabledElemsByType(anElemType);
    if (!theToReverse) {
        aDisIndexMap = IdsMapFromString(theElems);
        return;
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap = aShapeOwner->GetSubshapesIndexes(anElemType, false);
    if (anIndexMap.IsNull() || anIndexMap->Map().IsEmpty()) {
        return;
    }

    Handle(TColStd_HPackedMapOfInteger) anEnabledMap = IdsMapFromString(theElems);
    if (anEnabledMap.IsNull() || anEnabledMap->Map().IsEmpty()) {
        aDisIndexMap = new TColStd_HPackedMapOfInteger(anIndexMap->Map());
        return;
    }

    aDisIndexMap = new TColStd_HPackedMapOfInteger();
    aDisIndexMap->ChangeMap().Subtraction(anIndexMap->Map(), anEnabledMap->Map());
    if (aDisIndexMap->Map().Extent() != (anIndexMap->Map().Extent() - anEnabledMap->Map().Extent())) {
        Message::SendFail() << "Error: setDisabledShapeElems(" << theId << "," << theElemType << ", " << theElems
                            << ") specifies non-existing indices";
    }
}

// ================================================================
// Function : getSubshapesIndexes
// ================================================================
std::string SelectionManager::getSubshapesIndexes(const Viewer::ObjectId& theId,
                                                  const int                    theElemType,
                                                  const bool                   theIsFree,
                                                  const int                    theParentType,
                                                  const int                    theParentIndex,
                                                  const int                    theVisibility) const
{
    ViewShape::ShapeSelElemType anElemType = ViewShape::ShapeSelElemType::Face;
    if (!ViewShape::ShapeElemTypeFromSelMode(anElemType, theElemType)) {
        Message::SendFail() << "Error: getSubshapesIndexes(" << theId << ", " << theElemType << ", " << theParentType
                            << ", " << theParentIndex << ", " << theVisibility
                            << ") is called with invalid subshape type";
        return std::string();
    } else if (theVisibility < -1 || theVisibility > 1) {
        Message::SendFail() << "Error: getSubshapesIndexes(" << theId << ", " << theElemType << ", " << theParentType
                            << ", " << theParentIndex << ", " << theVisibility
                            << ") is called with invalid visibility flag";
        return std::string();
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: getSubshapesIndexes(" << theId << ", " << theElemType << ", " << theParentType
                            << ", " << theParentIndex << ", " << theVisibility << ") is called for non-existing object";
        return std::string();
    }

    TopoDS_Shape                 aParent;
    ViewShape::ShapeSelElemType aParentType = ViewShape::ShapeSelElemType::Group;
    if (theParentIndex != -1 && !ViewShape::ShapeElemTypeFromSelMode(aParentType, theParentType)) {
        Message::SendFail() << "Error: getSubshapesIndexes(" << theId << ", " << theElemType << ", " << theParentType
                            << ", " << theParentIndex << ", " << theVisibility
                            << ") is called with invalid parent type";
        return std::string();
    } else if (theParentIndex != -1) {
        aParent = aShapeOwner->GetSubshape(aParentType, theParentIndex);
        if (aParent.IsNull()) {
            Message::SendFail() << "Error: getSubshapesIndexes(" << theId << ", " << theElemType << ", "
                                << theParentType << ", " << theParentIndex << ", " << theVisibility
                                << ") is called with invalid parent index";
            return std::string();
        }
    }

    Handle(TColStd_HPackedMapOfInteger) anIndexMap =
        aShapeOwner->GetSubshapesIndexes(anElemType, theIsFree, aParent, theVisibility);
    if (!anIndexMap.IsNull()) {
        std::string aRes;
        IdsMapToString(aRes, anIndexMap);
        return aRes;
    }
    return std::string();
}

// ================================================================
// Function : resetSelection
// ================================================================
void SelectionManager::resetSelection()
{
    // Avoid extra calculations on clear selection logic when nothing selected.
    if (myWebViewer->GetContext()->NbSelected() == 0) {
        return;
    }

    myWebViewer->GetContext()->ClearSelected(false);
    myWebViewer->GetContext()->MainSelector()->Clear();
    myWebViewer->OnSelectionChanged(myWebViewer->GetContext(), myWebViewer->FocusView());
}

// ================================================================
// Function : setSelectedShapeElems
// ================================================================
void SelectionManager::setSelectedShapeElems(const Viewer::ObjectId&            theId,
                                             const Viewer::SelectionShapeElems& theSelElems)
{
    if (myWebViewer->GetContext()->NbSelected() == 0 && !theSelElems.IsValid()) {
        // avoid extra calculations on clear selection logic when nothing selected
        return;
    }

    Handle(ViewShape::InteractiveShapeOwner) aShapeOwner;
    if (!FindPrsOwner(aShapeOwner, theId)) {
        Message::SendFail() << "Error: setSelectedShapeElems(" << theId << ") is called for non-existing shape";
        return;
    }

    myWebViewer->GetContext()->MainSelector()->Clear();
    myWebViewer->GetContext()->SetSelectedState(aShapeOwner, true);

    ViewShape::ShapeSelElems anElems;
    for (int aTypeIter = 0; aTypeIter < ViewShape::ShapeSelElemType_NB; ++aTypeIter) {
        const ViewShape::ShapeSelElemType aType = (ViewShape::ShapeSelElemType)aTypeIter;
        anElems.ChangeIndices(aType)             = IdsMapFromString(theSelElems.GetElems(aType));
    }
    aShapeOwner->SetSelectedElems(anElems);

    myWebViewer->OnSelectionChanged(myWebViewer->GetContext(), myWebViewer->FocusView());
}

// ================================================================
// Function : getDetectedMeshEntity
// ================================================================
SelectionMeshEntity SelectionManager::getDetectedMeshEntity(const Viewer::ObjectId& theId) const
{
    SelectionMeshEntity aRes;

    Handle(ViewCae::InteractiveCaeMeshOwner) aMeshOwner;
    if (!FindPrsOwner(aMeshOwner, theId)) {
        Message::SendFail() << "Error: getDetectedMeshEntity(" << theId << ") is called for non-existing mesh";
        return aRes;
    }

    bool isPicked = false;

    const Handle(StdSelect_ViewerSelector3d)& aSelector = myWebViewer->GetContext()->MainSelector();
    for (int aPickIter = 1; aPickIter <= aSelector->NbPicked(); ++aPickIter) {
        if (aSelector->Picked(aPickIter) == aMeshOwner) {
            isPicked = true;
            break;
        }
    }

    if (!isPicked) {
        return aRes;
    }

    ViewCae::MeshSubEntity aSubEntity = aMeshOwner->DetectedSubEntity();

    aRes = SelectionMeshEntity(aSubEntity, aMeshOwner);
    return aRes;
}

// ================================================================
// Function : getSelectedMeshEntities
// ================================================================
std::vector<SelectionMeshEntity> SelectionManager::getSelectedMeshEntities(const Viewer::ObjectId& theId) const
{
    std::vector<SelectionMeshEntity> aRes;

    Handle(ViewCae::InteractiveCaeMeshOwner) aMeshOwner;
    if (!FindPrsOwner(aMeshOwner, theId)) {
        Message::SendFail() << "Error: getSelectedMeshEntities(" << theId << ") is called for non-existing mesh";
        return aRes;
    } else if (myWebViewer->GetContext()->IsSelected(aMeshOwner)) {
        const ViewCae::InteractiveCaeMeshOwner::MapOfSelectedEntities& aSelected = aMeshOwner->SelectedSubEntities();
        ViewCae::InteractiveCaeMeshOwner::MapOfSelectedEntities::Iterator anIter(aSelected);
        for (; anIter.More(); anIter.Next()) {
            SelectionMeshEntity aSel(anIter.Key(), aMeshOwner);
            if (aSel.IsDefined()) {
                aRes.push_back(aSel);
            }
        }
        return aRes;
    }

    auto processGroupAnnotation = [this, &aRes](const ViewCae::MeshAnnotation& theAnn,
                                                const TCollection_AsciiString&  thePrsId) {
        if (!theAnn.IsVisible()) {
            return;
        }
        NCollection_DataMap<int, Handle(ViewCae::MeshAnnotationLineOwner)>::Iterator aLineIter(theAnn.LineOwners());
        for (; aLineIter.More(); aLineIter.Next()) {
            if (!myWebViewer->GetContext()->IsSelected(aLineIter.Value())) {
                continue;
            }

            SelectionMeshEntity aSel(thePrsId, theAnn, aLineIter.Key());
            if (aSel.IsDefined()) {
                aRes.push_back(aSel);
            }
        }
    };

    for (const Handle(ViewCae::MeshPresentationBase)& aPrsBase : aMeshOwner->Presentations()) {
        ViewCae::MeshPresentation* aPrs = dynamic_cast<ViewCae::MeshPresentation*>(aPrsBase.get());
        const Handle(ViewCae::MeshPartGroupsInfoAnnotations)& aGroupsInfo =
            Handle(ViewCae::MeshPartGroupsInfoAnnotations)::DownCast(
                aPrs->ChangeParts()(ViewCae::MeshPartType::GroupsInfo));

        for (const ViewCae::MeshAnnotation& aGroupAnnotation : aGroupsInfo->Annotations().Annotations()) {
            processGroupAnnotation(aGroupAnnotation, aPrs->Id());
        }
    }
    return aRes;
}

// ================================================================
// Function : setSelectedMeshEntity
// ================================================================
void SelectionManager::setSelectedMeshEntity(const Viewer::ObjectId& theId, const SelectionMeshEntity& theEntity)
{
    Handle(ViewCae::InteractiveCaeMeshOwner) aMeshOwner;
    if (!FindPrsOwner(aMeshOwner, theId)) {
        Message::SendFail() << "Error: setSelectedMeshEntity(" << theId << ") is called for non-existing mesh";
        return;
    }

    if (theEntity.entityId == -1 || theEntity.entityType == (int)ViewCae::MeshEntityType::UNKNOWN) {
        return;
    }

    Handle(ViewCae::MeshPresentation) aPrs = aMeshOwner->Presentation(theEntity.prsId.c_str());
    if (aPrs.IsNull()) {
        Message::SendFail() << "Error: setSelectedMeshEntity (" << theId
                            << ") called for non-existing mesh presentation '" << theEntity.prsId << "'";
        return;
    }

    if (theEntity.entityType == (int)ViewCae::MeshEntityType::LABEL_GROUP) {
        const Handle(ViewCae::MeshPartGroupsInfoAnnotations)& aGroupsInfo =
            Handle(ViewCae::MeshPartGroupsInfoAnnotations)::DownCast(
                aPrs->ChangeParts()(ViewCae::MeshPartType::GroupsInfo));

        const ViewCae::MeshAnnotation* aGroupAnnotation = aGroupsInfo->Annotations().Annotation(theEntity.entityId);
        if (aGroupAnnotation == nullptr) {
            Message::SendFail() << "Error: setSelectedMeshEntity (" << theId << ") called for non-existing mesh group '"
                                << theEntity.entityId << "'";
            return;
        }

        if (!aGroupAnnotation->IsVisible()) {
            return;
        }

        Handle(ViewCae::MeshAnnotationLineOwner) aLineOwner;

        int aLineIndex = theEntity.lineIndex;
        if (!aGroupAnnotation->LineOwners().Find(aLineIndex, aLineOwner)) {
            Message::SendFail() << "Error: setSelectedMeshEntity (" << theId << ") called for wrong label line '"
                                << aLineIndex << "'";
            return;
        }

        myWebViewer->GetContext()->SetSelected(aLineOwner, false);
    } else {
        theEntity.UpdateSelectedEntity(aMeshOwner, myWebViewer->GetContext());
    }
}

// Module exports
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(SelectionManager)
{
    emscripten::class_<SelectionManager, emscripten::base<IManager>>("SelectionManager")
        .property("toAllowMultiselection",
                  &SelectionManager::ToAllowMultiselection,
                  &SelectionManager::SetAllowMultiselection)
        .function("getSelectedObjects", &SelectionManager::GetSelectedObjects)
        .function("setSelectedObjects", &SelectionManager::SetSelectedObjects)
        .function("getLastPickedPoint", &SelectionManager::GetLastPickedPoint)
        .function("setSelectionMode", &SelectionManager::setSelectionMode)
        .function("getSelectionMode", &SelectionManager::getSelectionMode)
        .function("getLastDetectedObject", &SelectionManager::getLastDetectedObject)
        .function("getDetectedShapeElem", &SelectionManager::getDetectedShapeElem)
        .function("getSelectedShapeElems", &SelectionManager::getSelectedShapeElems)
        .function("setSelectedShapeElems", &SelectionManager::setSelectedShapeElems)
        .function("getDisabledShapeElems", &SelectionManager::getDisabledShapeElems)
        .function("setDisabledShapeElems", &SelectionManager::setDisabledShapeElems)
        .function("getSubshapesIndexes", &SelectionManager::getSubshapesIndexes)
        .function("resetSelection", &SelectionManager::resetSelection)
        .function("getDetectedMeshEntity", &SelectionManager::getDetectedMeshEntity)
        .function("getSelectedMeshEntities", &SelectionManager::getSelectedMeshEntities)
        .function("setSelectedMeshEntity", &SelectionManager::setSelectedMeshEntity);
    emscripten::value_object<SelectionManager::DetectedObjectInfo>("DetectedObjectInfo")
        .field("objectId", &SelectionManager::DetectedObjectInfo::objectId)
        .field("partId", &SelectionManager::DetectedObjectInfo::partId)
        .field("subPartId", &SelectionManager::DetectedObjectInfo::subPartId)
        .field("pickedPoint", &SelectionManager::DetectedObjectInfo::pickedPoint)
        .field("pickedNormal", &SelectionManager::DetectedObjectInfo::pickedNormal);
}
#endif
}
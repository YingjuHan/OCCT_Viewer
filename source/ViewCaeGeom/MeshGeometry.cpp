#include <ViewCaeGeom/MeshGeometry.h>

#include <ViewCaeGeom/MeshGeometryBlock.h>
#include <ViewCaeGeom/MeshGeometryClip.h>
#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryGen.h>
#include <ViewCaeGeom/MeshGeometryIso.h>
#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshGroupProperty.h>
#include <RWCae/MeshScalarProperty.h>

#include <Standard_Atomic.hxx>
#include <limits>

namespace {
static volatile int g_meshGeomCounter = 0; //!< counter used in auto-generated mesh geometry id

}

namespace ViewCaeGeom {
// =======================================================================
// function : HashCode
// =======================================================================
int MeshInterpolatedNode::HashCode(const MeshInterpolatedNode& theElem, const int theUpper)
{
    unsigned int aHashCode = 0;
    for (const std::pair<RWCae::MeshEntityId, double>& aNode : theElem.BaseNodes()) {
        if (aNode.second >= myRatioConfusion) {
            aHashCode = aHashCode ^ ::HashCode(aNode.first, theUpper);
        }
    }
    return int(static_cast<unsigned int>(aHashCode & 0x7fffffff)) % theUpper + 1;
}

// =======================================================================
// function : IsEqual
// =======================================================================
bool MeshInterpolatedNode::IsEqual(const MeshInterpolatedNode& theElem1, const MeshInterpolatedNode& theElem2)
{
    static constexpr size_t aTwo = 2;

    // in most cases elements contain 1-2 nodes
    if (theElem1.BaseNodes().size() == 1 && theElem2.BaseNodes().size() == 1) {
        return theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[0].first;
    }

    if (theElem1.BaseNodes().size() == 1 && theElem2.BaseNodes().size() == aTwo) {
        return (theElem2.BaseNodes()[0].second < myRatioConfusion
                && theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[1].first)
            || (theElem2.BaseNodes()[1].second < myRatioConfusion
                && theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[0].first);
    }

    if (theElem1.BaseNodes().size() == aTwo && theElem2.BaseNodes().size() == 1) {
        return (theElem1.BaseNodes()[0].second < myRatioConfusion
                && theElem1.BaseNodes()[1].first == theElem2.BaseNodes()[0].first)
            || (theElem1.BaseNodes()[1].second < myRatioConfusion
                && theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[0].first);
    }

    if (theElem1.BaseNodes().size() == aTwo && theElem2.BaseNodes().size() == aTwo) {
        if (theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[0].first
            && theElem1.BaseNodes()[1].first == theElem2.BaseNodes()[1].first) {
            return true;
        }
        if (theElem1.BaseNodes()[0].first == theElem2.BaseNodes()[1].first
            && theElem1.BaseNodes()[1].first == theElem2.BaseNodes()[0].first) {
            return true;
        }
        int aBnd1 = theElem1.BaseNodes()[1].second < myRatioConfusion ? theElem1.BaseNodes()[0].first
                  : theElem1.BaseNodes()[0].second < myRatioConfusion ? theElem1.BaseNodes()[1].first
                                                                      : -1;
        int aBnd2 = theElem2.BaseNodes()[1].second < myRatioConfusion ? theElem2.BaseNodes()[0].first
                  : theElem2.BaseNodes()[0].second < myRatioConfusion ? theElem2.BaseNodes()[1].first
                                                                      : -1;
        return aBnd1 != -1 && aBnd1 == aBnd2;
    }

    // general case
    return IsEqualGeneral(theElem1, theElem2);
}

// =======================================================================
// function : IsEqualGeneral
// =======================================================================
bool MeshInterpolatedNode::IsEqualGeneral(const MeshInterpolatedNode& theElem1, const MeshInterpolatedNode& theElem2)
{
    auto getNotZeroNodes = [aRatio = myRatioConfusion](const MeshInterpolatedNode&         theElem,
                                                       std::vector<RWCae::MeshEntityId>& theNodes) {
        theNodes.reserve(theElem.BaseNodes().size());
        for (const std::pair<RWCae::MeshEntityId, double>& aNode : theElem.BaseNodes()) {
            if (aNode.second >= aRatio) {
                theNodes.push_back(aNode.first);
            }
        }
    };

    std::vector<RWCae::MeshEntityId> aNodes1;
    getNotZeroNodes(theElem1, aNodes1);
    std::vector<RWCae::MeshEntityId> aNodes2;
    getNotZeroNodes(theElem2, aNodes2);

    if (aNodes1.size() != aNodes2.size()) {
        return false;
    }

    std::sort(aNodes1.begin(), aNodes1.end());
    std::sort(aNodes2.begin(), aNodes2.end());
    return aNodes1 == aNodes2;
}

// =======================================================================
// function : MeshGeometry
// =======================================================================
MeshGeometry::MeshGeometry(const Handle(MeshBvhData)& theBvhData)
    : myIsUpdated(false),
      myBvhData(theBvhData),
      myId(TCollection_AsciiString("geom_") + TCollection_AsciiString(Standard_Atomic_Increment(&g_meshGeomCounter)))
{
}

// =======================================================================
// function : GetMeshData
// =======================================================================
const Handle(RWCae::MeshData)& MeshGeometry::GetMeshData()
{
    if (!myIsUpdated) {
        generateMeshData();
        myIsUpdated = true;
    }

    return myMeshData;
}

// =======================================================================
// function : Append
// =======================================================================
void MeshGeometry::Append(const Handle(MeshGeometry)& theChild)
{
    Handle(MeshGeometry) aGeom = this;
    while (!aGeom->myChildGeometry.IsNull() && aGeom->myChildGeometry->OrderIndex() <= theChild->OrderIndex()) {
        aGeom = aGeom->myChildGeometry.get();
    }
    if (!aGeom->myChildGeometry.IsNull()) {
        aGeom->myChildGeometry->myParentGeometry = theChild;
        theChild->myChildGeometry                = aGeom->myChildGeometry;
    }
    aGeom->myChildGeometry     = theChild;
    theChild->myParentGeometry = aGeom;
    SetToUpdate();
}

// =======================================================================
// function : Remove
// =======================================================================
void MeshGeometry::Remove(const TCollection_AsciiString& theId)
{
    for (const Handle(MeshGeometry)& aGeom : MeshGeomChildren(this)) {
        if (aGeom->Id() == theId) {
            if (!aGeom->myParentGeometry.IsNull()) {
                aGeom->myParentGeometry->myChildGeometry = aGeom->myChildGeometry;
            }
            if (!aGeom->myChildGeometry.IsNull()) {
                aGeom->myChildGeometry->myParentGeometry = aGeom->myParentGeometry;
            }
            break;
        }
    }
    SetToUpdate();
}

// =======================================================================
// function : RemoveChildren
// =======================================================================
void MeshGeometry::RemoveChildren()
{
    for (MeshGeometry* aGeom : MeshGeomParents(GetLastChild().get())) {
        if (aGeom == this) {
            break;
        }
        aGeom->clear();
        aGeom->myChildGeometry.Nullify();
    }

    myChildGeometry.Nullify();
}

// =======================================================================
// function : GetLastChild
// =======================================================================
Handle(MeshGeometry) MeshGeometry::GetLastChild() const
{
    Handle(MeshGeometry) aGeom = this;
    while (!aGeom->myChildGeometry.IsNull()) {
        aGeom = aGeom->myChildGeometry;
    }

    return aGeom;
}

// =======================================================================
// function : GetLastNotModifierChild
// =======================================================================
Handle(MeshGeometry) MeshGeometry::GetLastNotModifierChild() const
{
    Handle(MeshGeometry) aGeom = GetLastChild();
    while (!aGeom->myParentGeometry.IsNull() && aGeom->IsModifier()) {
        aGeom = aGeom->myParentGeometry;
    }

    return aGeom;
}

// =======================================================================
// function : GetFirstParent
// =======================================================================
Handle(MeshGeometry) MeshGeometry::GetFirstParent() const
{
    Handle(MeshGeometry) aGeom = this;
    while (!aGeom->myParentGeometry.IsNull()) {
        aGeom = aGeom->myParentGeometry;
    }

    return aGeom;
}

// =======================================================================
// function : Clear
// =======================================================================
void MeshGeometry::Clear()
{
    for (MeshGeometry* aGeom : MeshGeomChildren(this)) {
        aGeom->clear();
        aGeom->myParentGeometry.Nullify();
    }
}

// =======================================================================
// function : SetToUpdate
// =======================================================================
void MeshGeometry::SetToUpdate()
{
    setToUpdate();

    for (MeshGeometry* aChild : MeshGeomChildren(myChildGeometry.get())) {
        aChild->setToUpdate();
        aChild->onUpdatedFromOthers();
    }

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        aParent->setToUpdate();
        aParent->onUpdatedFromOthers();
    }
}

// =======================================================================
// function : ModifyNode
// =======================================================================
void MeshGeometry::ModifyNode(const RWCae::MeshEntityId theNodeIdx, Graphic3d_Vec3& theNode) const
{
    bool canModifyNodes = false;
    for (MeshGeometry* aChild : MeshGeomChildren(myChildGeometry.get())) {
        if (aChild->myCanModifyNodes) {
            canModifyNodes = true;
            break;
        }
    }

    if (!canModifyNodes) {
        return;
    }

    MeshInterpolatedNode aBaseNodes;
    if (!OriginNode(theNodeIdx, aBaseNodes)) {
        return;
    }
    for (MeshGeometry* aChild : MeshGeomChildren(myChildGeometry.get())) {
        if (aChild->IsModifier()) {
            aChild->modifyNode(theNodeIdx, &aBaseNodes, theNode);
        }
    }
}

// =======================================================================
// function : AcceptNode
// =======================================================================
bool MeshGeometry::AcceptNode(const RWCae::MeshEntityId theNodeIdx, const Graphic3d_Vec3& theNode) const
{
    for (MeshGeometry* aChild : MeshGeomChildren(myChildGeometry.get())) {
        if (aChild->IsModifier() && !aChild->acceptNode(theNodeIdx, this, theNode)) {
            return false;
        }
    }

    return true;
}

// =======================================================================
// function : AcceptElement
// =======================================================================
bool MeshGeometry::AcceptElement(const RWCae::MeshEntityId theElemIdx) const
{
    for (MeshGeometry* aChild : MeshGeomChildren(myChildGeometry.get())) {
        if (aChild->IsModifier() && !aChild->acceptElement(myMeshData, theElemIdx, this)) {
            return false;
        }
    }

    return true;
}

// =======================================================================
// function : CalculatePropertyComponent
// =======================================================================
RWCae::MeshPropertyComponent MeshGeometry::CalculatePropertyComponent(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    int                                       theCompIndex,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    Handle(RWCae::MeshNVecProperty) aProp = CalculateProperty(theProp, theElem, theNodes);
    if (aProp.IsNull() || theCompIndex >= aProp->NbRawComponents()) {
        return RWCae::MeshPropertyComponent();
    }
    return RWCae::MeshPropertyComponent(aProp, theCompIndex);
}

// =======================================================================
// function : calculateCommonProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) MeshGeometry::calculateCommonProperty(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes,
    const RWCae::MeshData::EntityMap*       theNodesMap) const
{
    if (theProp.IsNull()) {
        return nullptr;
    }

    if (myMeshData.IsNull()) {
        return nullptr;
    }

    if (myMeshData->NbNodes() == 0) {
        return nullptr;
    }

    Handle(RWCae::MeshNVecProperty) aProp =
        new RWCae::MeshNVecProperty(theProp->Name(), theProp->NbComponents(), theProp->MapType(), theProp->Type());

    if (theProp->IsNodal()) {
        calculateCommonPropertyNodal(theProp, aProp, theNodes, theNodesMap);
    } else {
        auto compute = [this, &theProp, &aProp, &theElem](const auto& theAdaptor) {
            calculateCommonPropertyElemental(theProp, aProp, theAdaptor, theElem);
        };

        if (myMeshData->NbIndexedElems() > 0 || theElem != nullptr) {
            RWCae::MeshAdaptorsIteratorAllElems::For(compute, myMeshData);
        }
        if (myMeshData->NbIndexedElems() == 0 || theElem != nullptr) {
            RWCae::MeshAdaptorsIterator2dElems::For(compute, myMeshData, true);
        }
    }

    return aProp->IsInitialized() ? aProp : nullptr;
}

// =======================================================================
// function : calculateCommonPropertyNodal
// =======================================================================
void MeshGeometry::calculateCommonPropertyNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                                const Handle(RWCae::MeshNVecProperty)&  theProp,
                                                const std::vector<RWCae::MeshEntityId>* theNodes,
                                                const RWCae::MeshData::EntityMap*       theNodesMap) const
{
    if (theBaseProp->HasScalarComponents()) {
        genScalarCommonPropNodal(theBaseProp, theProp, theNodes, theNodesMap);
    } else if (theBaseProp->HasGroupComponents()) {
        genGroupCommonPropNodal(theBaseProp, theProp, theNodes, theNodesMap);
    }
}

// =======================================================================
// function : calculateCommonPropertyElemental
// =======================================================================
template<class Elem_T>
void MeshGeometry::calculateCommonPropertyElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                    const Handle(RWCae::MeshNVecProperty)& theProp,
                                                    const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                                    const MeshGeomElem*                      theElem) const
{
    if (theMesh.NbElements() == 0) {
        return;
    }

    if (theElem != nullptr
        && (theElem->Type != theMesh.GetElementType() || theElem->IsSurface != theMesh.IsSurface())) {
        return;
    }

    if (theBaseProp->HasNodeVecComponents()) {
        genNodeVecCommonProp(theBaseProp, theProp, theMesh, theElem);
    } else if (theBaseProp->HasScalarComponents()) {
        genScalarCommonPropElemental(theBaseProp, theProp, theMesh, theElem);
    } else if (theBaseProp->HasGroupComponents()) {
        genGroupCommonPropElemental(theBaseProp, theProp, theMesh, theElem);
    }
}

// =======================================================================
// function : genScalarCommonPropNodal
// =======================================================================
void MeshGeometry::genScalarCommonPropNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                            const Handle(RWCae::MeshNVecProperty)&  theProp,
                                            const std::vector<RWCae::MeshEntityId>* theNodes,
                                            const RWCae::MeshData::EntityMap*       theNodesMap) const
{
    if (!theProp->IsInitialized()) {
        theProp->Init<RWCae::MeshScalarProperty>();
    }

    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        Handle(RWCae::MeshScalarPropertyBase) aBaseComp = theBaseProp->GetComponent(aCompInd);
        Handle(RWCae::MeshScalarProperty)     aComp     = Handle(RWCae::MeshScalarProperty)::DownCast(
            theProp->GetComponent(aCompInd));
        if (theNodes != nullptr) {
            aComp->Init(static_cast<int>(theNodes->size()), aBaseComp->ElemType());
            for (RWCae::MeshEntityId i = 0; i < theNodes->size(); i++) {
                calculateScalarNodalValue(aBaseComp, aComp, (*theNodes)[i], i, theNodesMap);
            }
        } else {
            aComp->Init(myMeshData->NbNodes(), aBaseComp->ElemType());
            for (RWCae::MeshEntityId i = 0; i < myMeshData->NbNodes(); i++) {
                calculateScalarNodalValue(aBaseComp, aComp, i, i, theNodesMap);
            }
        }
    }

    updateGenPropComponents(theBaseProp, theProp);
}

// =======================================================================
// function : calculateScalarNodalValue
// =======================================================================
void MeshGeometry::calculateScalarNodalValue(const Handle(RWCae::MeshScalarPropertyBase)& theBaseComp,
                                             const Handle(RWCae::MeshScalarPropertyBase)& theComp,
                                             RWCae::MeshEntityId                          theNodeIdx,
                                             RWCae::MeshEntityId                          thePropValIdx,
                                             const RWCae::MeshData::EntityMap*            theNodesMap) const
{
    MeshInterpolatedNode aLerpPnt;
    if (!OriginNode(theNodeIdx, aLerpPnt)) {
        return;
    }
    if (aLerpPnt.BaseNodes().size() == 1) {
        RWCae::MeshEntityId aBaseNodeIdx = aLerpPnt.BaseNodes().front().first;
        if (theNodesMap != nullptr) {
            theNodesMap->Find(aBaseNodeIdx, aBaseNodeIdx);
        }
        theComp->SetRawValue(RWCae::MeshPropertyEntityInfo(thePropValIdx),
                             theBaseComp->RawValue(RWCae::MeshPropertyEntityInfo(aBaseNodeIdx)),
                             false);
    } else {
        double aResValue = 0.0;
        for (const std::pair<RWCae::MeshEntityId, double>& aBaseNode : aLerpPnt.BaseNodes()) {
            if (aBaseNode.second == 0.0) { continue; }
            RWCae::MeshEntityId aBaseNodeIdx = aBaseNode.first;
            if (theNodesMap != nullptr) {
                theNodesMap->Find(aBaseNodeIdx, aBaseNodeIdx);
            }
            double aValue = theBaseComp->Value(RWCae::MeshPropertyEntityInfo(aBaseNodeIdx));
            if (RWCae::IsNaN(aValue)) {
                aResValue = aValue;
                break;
            }

            aResValue += aValue * aBaseNode.second;
        }
        theComp->SetValue(RWCae::MeshPropertyEntityInfo(thePropValIdx), aResValue, false);
    }
}

// =======================================================================
// function : genScalarCommonPropElemental
// =======================================================================
template<class Elem_T>
void MeshGeometry::genScalarCommonPropElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                const Handle(RWCae::MeshNVecProperty)& theProp,
                                                const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                                const MeshGeomElem*                      theElem) const
{
    if (!theProp->IsInitialized()) {
        theProp->Init<RWCae::MeshScalarProperty>();
    }

    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        Handle(RWCae::MeshScalarPropertyBase) aBaseComp = theBaseProp->GetComponent(aCompInd);
        Handle(RWCae::MeshScalarProperty)     aComp     = Handle(RWCae::MeshScalarProperty)::DownCast(
            theProp->GetComponent(aCompInd));

        if (!aComp->IsLoaded()) {
            aComp->Init(theElem != nullptr ? 1 : aNbElems, aBaseComp->ElemType());
        }

        auto setValue = [this, &theMesh, &aComp, &aBaseComp](RWCae::MeshEntityId theIdx,
                                                             RWCae::MeshEntityId thePropValIdx) {
            RWCae::MeshEntityId aBaseElem = 0;
            if (!OriginElement(MeshGeomElem(theIdx, theMesh.GetElementType(), theMesh.IsSurface()), aBaseElem)) {
                return;
            }
            aComp->SetRawValue(RWCae::MeshPropertyEntityInfo(thePropValIdx),
                               aBaseComp->RawValue(RWCae::MeshPropertyEntityInfo(aBaseElem)),
                               false);
        };

        if (theElem != nullptr) {
            setValue(theElem->Index, 0);
        } else {
            for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
                int aGlobalIndex = theMesh.GetElement(i).GlobalIndex();
                setValue(i, aGlobalIndex);
            }
        }
    }

    updateGenPropComponents(theBaseProp, theProp);
}

// =======================================================================
// function : genNodeVecCommonProp
// =======================================================================
template<class Elem_T>
void MeshGeometry::genNodeVecCommonProp(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                        const Handle(RWCae::MeshNVecProperty)& theProp,
                                        const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                        const MeshGeomElem*                      theElem) const
{
    if (!theProp->IsInitialized()) {
        theProp->Init<RWCae::MeshNodeVecProperty>();
    }

    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        Handle(RWCae::MeshNodeVecProperty) aBaseComp = Handle(RWCae::MeshNodeVecProperty)::DownCast(
            theBaseProp->GetComponent(aCompInd));
        Handle(RWCae::MeshNodeVecProperty) aComp = Handle(RWCae::MeshNodeVecProperty)::DownCast(
            theProp->GetComponent(aCompInd));
        if (!aComp->IsLoaded()) {
            aComp->Init(theElem != nullptr ? 1 : aNbElems, 0, false, false, aBaseComp->ElemType());
        }

        auto setValue = [this, &theMesh, &aBaseComp, &aComp](RWCae::MeshEntityId theIdx,
                                                             RWCae::MeshEntityId thePropValIdx) {
            RWCae::MeshEntityId aBaseElem = 0;
            if (!OriginElement(MeshGeomElem(theIdx, theMesh.GetElementType(), theMesh.IsSurface()), aBaseElem)) {
                return;
            }
            calculateNodeVecValue(theMesh.GetElement(theIdx), aBaseElem, thePropValIdx, aBaseComp, aComp);
        };

        if (theElem != nullptr) {
            setValue(theElem->Index, 0);
        } else {
            for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
                setValue(i, theMesh.GetElement(i).GlobalIndex());
            }
        }
    }

    updateGenPropComponents(theBaseProp, theProp);
}

// =======================================================================
// function : calculateNodeVecValue
// =======================================================================
template<class Elem_T>
void MeshGeometry::calculateNodeVecValue(const Elem_T&                               theElem,
                                         RWCae::MeshEntityId                       theBaseElemIdx,
                                         RWCae::MeshEntityId                       thePropValIdx,
                                         const Handle(RWCae::MeshNodeVecProperty)& theBaseComp,
                                         const Handle(RWCae::MeshNodeVecProperty)& theComp) const
{
    std::vector<int> aSubIndices;
    theBaseComp->SubIndices(theBaseElemIdx, aSubIndices);
    if (aSubIndices.empty()) {
        theComp->SetValue(RWCae::MeshPropertyEntityInfo(thePropValIdx), std::numeric_limits<double>::quiet_NaN(), false);
        return;
    }

    auto computeLerpValue = [&theBaseComp, &theBaseElemIdx](const MeshInterpolatedNode& theLerpPnt) -> double {
        double aResValue = 0.0;
        for (const std::pair<RWCae::MeshEntityId, double>& aBaseNode : theLerpPnt.BaseNodes()) {
            if (aBaseNode.second == 0.0) { continue; }
            double aValue = theBaseComp->Value(RWCae::MeshPropertyEntityInfo(theBaseElemIdx, aBaseNode.first));
            if (RWCae::IsNaN(aValue)) {
                aResValue = aValue;
                break;
            }

            aResValue += aValue * aBaseNode.second;
        }
        return aResValue;
    };

    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theElem.NbNodes(); aNodeIter++) {
        MeshInterpolatedNode aLerpPnt;
        if (!OriginNode(theElem[aNodeIter], aLerpPnt)) {
            continue;
        }

        RWCae::MeshPropertyEntityInfo anInfo(thePropValIdx, theElem[aNodeIter]);
        if (aLerpPnt.BaseNodes().size() == 1) {
            RWCae::MeshPropertyEntityInfo aBaseInfo(theBaseElemIdx, aLerpPnt.BaseNodes().front().first);
            theComp->SetRawValue(anInfo, theBaseComp->RawValue(aBaseInfo), false);
        } else {
            theComp->SetValue(anInfo, computeLerpValue(aLerpPnt), false);
        }
    }
}

// =======================================================================
// function : genGroupCommonPropNodal
// =======================================================================
void MeshGeometry::genGroupCommonPropNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                           const Handle(RWCae::MeshNVecProperty)&  theProp,
                                           const std::vector<RWCae::MeshEntityId>* theNodes,
                                           const RWCae::MeshData::EntityMap*       theNodesMap) const
{
    if (!theProp->IsInitialized()) {
        theProp->Init<RWCae::MeshGroupProperty>();
    }

    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        Handle(RWCae::MeshGroupProperty) aBaseComp = Handle(RWCae::MeshGroupProperty)::DownCast(
            theBaseProp->GetComponent(aCompInd));
        Handle(RWCae::MeshGroupProperty) aComp = Handle(RWCae::MeshGroupProperty)::DownCast(
            theProp->GetComponent(aCompInd));

        auto setValue = [this, &aBaseComp, &aComp, &theNodesMap](RWCae::MeshEntityId theIdx,
                                                                 RWCae::MeshEntityId thePropValIdx) {
            MeshInterpolatedNode aLerpPnt;
            if (!OriginNode(theIdx, aLerpPnt)) {
                return;
            }
            RWCae::MeshEntityId aBaseNodeIdx = aLerpPnt.BaseNodes().front().first;
            if (theNodesMap != nullptr) {
                theNodesMap->Find(aBaseNodeIdx, aBaseNodeIdx);
            }
            aComp->SetGroups(thePropValIdx,
                             !aLerpPnt.BaseNodes().empty() ? aBaseComp->GetGroups(aBaseNodeIdx)
                                                           : RWCae::MeshGroupProperty::MeshGroupsMaskType(0),
                             false,
                             false);
        };
        if (theNodes != nullptr) {
            aComp->Init(static_cast<int>(theNodes->size()));
            for (RWCae::MeshEntityId i = 0; i < theNodes->size(); i++) {
                setValue((*theNodes)[i], i);
            }
        } else {
            aComp->Init(myMeshData->NbNodes());
            for (RWCae::MeshEntityId i = 0; i < myMeshData->NbNodes(); i++) {
                setValue(i, i);
            }
        }
    }

    updateGenPropComponents(theBaseProp, theProp);
}

// =======================================================================
// function : genGroupCommonPropElemental
// =======================================================================
template<class Elem_T>
void MeshGeometry::genGroupCommonPropElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                               const Handle(RWCae::MeshNVecProperty)& theProp,
                                               const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                               const MeshGeomElem*                      theElem) const
{
    if (!theProp->IsInitialized()) {
        theProp->Init<RWCae::MeshGroupProperty>();
    }

    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        Handle(RWCae::MeshGroupProperty) aBaseComp = Handle(RWCae::MeshGroupProperty)::DownCast(
            theBaseProp->GetComponent(aCompInd));
        Handle(RWCae::MeshGroupProperty) aComp = Handle(RWCae::MeshGroupProperty)::DownCast(
            theProp->GetComponent(aCompInd));
        if (!aComp->IsLoaded()) {
            aComp->Init(theElem != nullptr ? 1 : aNbElems);
        }

        auto setVal = [this, &theMesh, &aBaseComp, &aComp](RWCae::MeshEntityId theIdx,
                                                           RWCae::MeshEntityId thePropValIdx) {
            RWCae::MeshEntityId aBaseElem = 0;
            if (!OriginElement(MeshGeomElem(theIdx, theMesh.GetElementType(), theMesh.IsSurface()), aBaseElem)) {
                return;
            }
            aComp->SetGroups(thePropValIdx, aBaseComp->GetGroups(aBaseElem), false, false);
        };

        if (theElem != nullptr) {
            setVal(theElem->Index, 0);
        } else {
            for (RWCae::MeshEntityId i = 0; i < aNbElems; i++) {
                setVal(i, theMesh.GetElement(i).GlobalIndex());
            }
        }
    }

    updateGenPropComponents(theBaseProp, theProp);
}

// =======================================================================
// function : genPropValues
// =======================================================================
Handle(RWCae::MeshNVecProperty) MeshGeometry::genPropValues(const Handle(RWCae::MeshNVecProperty)&  theProp,
                                                              const MeshGeomElem*                       theElem,
                                                              const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    if (theProp.IsNull()) {
        return nullptr;
    }

    if (theProp->IsElemental()) {
        if (theElem != nullptr) {
            return calculateCommonProperty(theProp, theElem, nullptr);
        }
    } else if (theProp->IsNodal()) {
        if (theNodes != nullptr) {
            return calculateCommonProperty(theProp, nullptr, theNodes);
        }
    }

    if (theElem != nullptr || theNodes != nullptr) {
        return nullptr;
    }

    return theProp;
}

// =======================================================================
// function : updateGenPropComponents
// =======================================================================
void MeshGeometry::updateGenPropComponents(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                           const Handle(RWCae::MeshNVecProperty)& theProp) const
{
    theProp->UpdateComponents();

    for (int aCompIdx = 0; aCompIdx < theProp->NbRawComponents(); aCompIdx++) {
        Handle(RWCae::MeshScalarPropertyBase) aBaseComp = theBaseProp->GetRawComponent(aCompIdx);
        Handle(RWCae::MeshScalarPropertyBase) aComp     = theProp->GetRawComponent(aCompIdx);
        aComp->SetValueRange(aBaseComp->ValueRange());
        aComp->SetId(aBaseComp->Id());
    }
}

// ================================================================
// Function : OriginElement
// ================================================================
bool MeshGeometry::OriginElement(const MeshGeomElem& theElem, RWCae::MeshEntityId& theOriginElem) const
{
    MeshGeomElem         anElem = theElem;
    Handle(MeshGeometry) aGeom  = this;
    while (!aGeom->GetParent().IsNull()) {
        const std::pair<MeshGeometry*, MeshGeomElem>* aParentElem = aGeom->GetElementsMap().Seek(anElem);
        if (aParentElem == nullptr || aParentElem->first == nullptr) {
            return false;
        }

        aGeom  = aParentElem->first;
        anElem = aParentElem->second;
    }

    if (anElem.IsSurface) {
        theOriginElem = aGeom->GetMeshData()->SurfaceElements(anElem.Type).GlobalIndex(anElem.Index);
    } else {
        theOriginElem = aGeom->GetMeshData()->Elements(anElem.Type).GlobalIndex(anElem.Index);
    }

    return true;
}

// ================================================================
// Function : OriginNode
// ================================================================
bool MeshGeometry::OriginNode(RWCae::MeshEntityId theNode, MeshInterpolatedNode& theOriginNodes) const
{
    theOriginNodes.AddBaseNode(theNode, 1.0);
    Handle(MeshGeometry) aGeom = this;
    while (!aGeom->GetParent().IsNull()) {
        size_t aNbNodes = theOriginNodes.BaseNodes().size();
        for (size_t aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++) {
            const std::pair<MeshGeometry*, MeshInterpolatedNode>* aParentNode = aGeom->GetNodesMap().Seek(
                theOriginNodes.BaseNodes()[aNodeIter].first);
            if (aParentNode == nullptr || aParentNode->first == nullptr) {
                return false;
            }

            if (aNodeIter == aNbNodes - 1) {
                aGeom = aParentNode->first;
            }

            double aCoeff = theOriginNodes.BaseNodes()[aNodeIter].second;
            for (size_t aParNodeIter = 1; aParNodeIter < aParentNode->second.BaseNodes().size(); aParNodeIter++) {
                theOriginNodes.AddBaseNode(aParentNode->second.BaseNodes()[aParNodeIter].first,
                                           aParentNode->second.BaseNodes()[aParNodeIter].second * aCoeff);
            }

            theOriginNodes.BaseNodes()[aNodeIter].first  = aParentNode->second.BaseNodes()[0].first;
            theOriginNodes.BaseNodes()[aNodeIter].second = aParentNode->second.BaseNodes()[0].second * aCoeff;
        }
    }

    return true;
}

// ================================================================
// Function : ResetStaticCounter
// ================================================================
void MeshGeometry::ResetStaticCounter()
{
    g_meshGeomCounter = 0;
}

// =======================================================================
// function : MeshGeometryBase
// =======================================================================
MeshGeometryBase::MeshGeometryBase(const Handle(MeshBvhData)& theBvhData, const Handle(RWCae::MeshData)& theMeshData)
    : MeshGeometry(theBvhData)
{
    myMeshData = theMeshData;
}

// =======================================================================
// function : Create
// =======================================================================
Handle(MeshGeometry) MeshGeometryFactory::Create(const Handle(RWCae::MeshData)& theMeshData,
                                                 const Handle(MeshBvhData)&       theBvhData,
                                                 MeshGeometryType                 theType)
{
    switch (theType) {
        case MeshGeometryType::Base: return new MeshGeometryBase(theBvhData, theMeshData);
        case MeshGeometryType::Block: return new MeshGeometryBlock(theBvhData, theMeshData->Blocks());
        case MeshGeometryType::Clip: return new MeshGeometryClip(theBvhData);
        case MeshGeometryType::Deformed: return new MeshGeometryDeformed(theBvhData);
        case MeshGeometryType::Gen: return new MeshGeometryGen(theBvhData);
        case MeshGeometryType::Iso: return new MeshGeometryIso(theBvhData);
        case MeshGeometryType::Slice: return new MeshGeometrySlice(theBvhData);
        default: return nullptr;
    }
}

} // namespace

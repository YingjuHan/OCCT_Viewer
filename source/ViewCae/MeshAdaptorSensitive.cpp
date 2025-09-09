#include <ViewCae/MeshAdaptorSensitive.h>

namespace ViewCae {

// =======================================================================
// function : MeshAdaptorSensitive
// =======================================================================
MeshAdaptorSensitive::MeshAdaptorSensitive(const Handle(SelectMgr_EntityOwner)& theOwnerId,
                                           ViewCaeGeom::MeshGeometry*          theMeshGeom,
                                           MeshPresentation*                    theMeshPrs)
    : Select3D_SensitiveSet(theOwnerId),
      myMeshGeom(theMeshGeom),
      myMeshPrs(theMeshPrs)
{
    mySFactor = DefaultSensiveFactor2D;
}

// =======================================================================
// function : Matches
// =======================================================================
Standard_Boolean MeshAdaptorSensitive::Matches(SelectBasics_SelectingVolumeManager& theMgr,
                                               SelectBasics_PickResult&             thePickResult)
{
    myDetectedIndices.Clear();

    bool toCheckAllInside = false;
    if (!theMgr.IsOverlapAllowed() && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        const ViewBase::InteractiveOwner* anOwner = dynamic_cast<ViewBase::InteractiveOwner*>(myOwnerId.get());
        // check for InteractiveCaeMeshSelMode::Global
        toCheckAllInside = anOwner != nullptr && anOwner->GetSelectionMode() != 0;
    }

    /* @code
    if (theMgr.GetActiveSelectionType() == SelectMgr_SelectionType_Point && mySFactor == 0) {
        // reset sensitivity factor to 0 pixels
        // (discard factor added to entire viewer by SelectMgr_ViewerSelector::computeFrustum())
        AIS_InteractiveObject* aPrs = static_cast<AIS_InteractiveObject*>(myOwnerId->Selectable().get());

        gp_GTrsf anInvTrsf = HasInitLocation() ? InvInitLocation() * aPrs->InversedTransformation()
                                               : aPrs->InversedTransformation();
        SelectMgr_SelectingVolumeManager aTmpMgr =
            aPrs->InteractiveContext()->MainSelector()->GetManager().ScaleAndTransform(
                0,
                anInvTrsf,
                Handle(SelectMgr_FrustumBuilder)());
        return Select3D_SensitiveSet::matches(aTmpMgr, thePickResult, toCheckAllInside);
    } @endcode */

    bool aRes = Select3D_SensitiveSet::matches(theMgr, thePickResult, toCheckAllInside);
    if (aRes && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        myDetectedIdx = -1;
    }
    return aRes;
}

// =======================================================================
// function : Init
// =======================================================================
template<class Elem_T>
void MeshAdaptorSensitiveElement<Elem_T>::Init(const RWCae::MeshAdaptor<Elem_T>& theMeshAdaptor)
{
    MarkDirty();
    myMeshAdaptor = theMeshAdaptor;

    RWCae::MeshEntityId aNbElems = myMeshAdaptor.NbElements();
    if (aNbElems == 0) {
        myBVHPrimIndexes.Nullify();
        return;
    }

    mySFactor = DefaultSensitiveFactor<Elem_T>();

    if (!myBVHPrimIndexes.IsNull()) {
        myBVHPrimIndexes->Resize(0, aNbElems - 1, false);
    } else {
        myBVHPrimIndexes = new TColStd_HArray1OfInteger(0, aNbElems - 1);
    }
    TColStd_Array1OfInteger& aBVHPrimIdxs = myBVHPrimIndexes->ChangeArray1();

    Graphic3d_Vec3 aCenter(0.0f);
    Graphic3d_Vec3 aNode;
    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = myMeshAdaptor.GetElement(anElemIter);

        Graphic3d_Vec3 anElemCenter(0.0f);
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
            getNodePosition(aNode, anElem[aNodeIter]);
            anElemCenter += aNode;
        }
        aCenter += anElemCenter / anElem.NbNodes();
        aBVHPrimIdxs(anElemIter) = anElemIter;
    }

    aCenter /= (float)aNbElems;
    myCDG3D = gp_Pnt(aCenter.x(), aCenter.y(), aCenter.z());
    computeBoundingBox();
}

// =======================================================================
// function : Box
// =======================================================================
template<class Elem_T>
Select3D_BndBox3d MeshAdaptorSensitiveElement<Elem_T>::Box(const Standard_Integer theIdx) const
{
    const Standard_Integer anElemIdx = myBVHPrimIndexes->Value(theIdx);

    const Elem_T& anElem = myMeshAdaptor.GetElement(anElemIdx);

    Graphic3d_Vec3 aMinPnt(ShortRealLast());
    Graphic3d_Vec3 aMaxPnt(-ShortRealLast());

    Graphic3d_Vec3 aNode;
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
        getNodePosition(aNode, anElem[aNodeIter]);

        aMinPnt = aMinPnt.cwiseMin(aNode);
        aMaxPnt = aMaxPnt.cwiseMax(aNode);
    }

    return Select3D_BndBox3d(SelectMgr_Vec3(aMinPnt), SelectMgr_Vec3(aMaxPnt));
}

// =======================================================================
// function : computeBoundingBox
// =======================================================================
template<class Elem_T>
inline void MeshAdaptorSensitiveElement<Elem_T>::computeBoundingBox()
{
    myBndBox.Clear();

    RWCae::MeshEntityId aNbNodes = myMeshAdaptor.NbNodes();

    Graphic3d_Vec3 aNode;
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter) {
        getNodePosition(aNode, aNodeIter);
        myBndBox.Add(SelectMgr_Vec3(aNode));
    }
}

// =======================================================================
// function : overlapsElement
// =======================================================================
template<class Elem_T>
Standard_Boolean MeshAdaptorSensitiveElement<Elem_T>::overlapsElement(SelectBasics_PickResult& thePickResult,
                                                                      SelectBasics_SelectingVolumeManager& theMgr,
                                                                      Standard_Integer                     theElemIdx,
                                                                      Standard_Boolean theIsFullInside)
{
    const Standard_Integer anElemIdx = myBVHPrimIndexes->Value(theElemIdx);

    const Elem_T& anElem = myMeshAdaptor.GetElement(anElemIdx);
    if (!myMeshGeom->AcceptElement(anElem.GlobalIndex())) {
        return false;
    }

    if (theIsFullInside) {
        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
            myDetectedIndices.Add(anElemIdx);
        }
        return true;
    }

    std::vector<Graphic3d_Vec3> aNodes(anElem.NbNodes());
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
        getNodePosition(aNodes[aNodeIter], anElem[aNodeIter]);
    }

    bool aRes = false;

    if (Elem_T::Type == RWCae::MeshElemType::Elem0d) {
        aRes = theMgr.OverlapsPoint(gp_Pnt(aNodes[m_n0].x(), aNodes[m_n0].y(), aNodes[m_n0].z()), thePickResult);
        if (aRes && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
            myDetectedIndices.Add(anElemIdx);
        }
        return aRes;
    }

    RWCae::MeshElemEntityId aNbFaces = myMeshAdaptor.NbFaces(anElem);
    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces && !aRes; aFaceIter++) {
        for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(myMeshAdaptor, anElem, aFaceIter);
             aTrgIter.More() && !aRes;
             aTrgIter.Next()) {
            const Graphic3d_Vec3ub& aTrg = aTrgIter.TriangleIndices();

            const Graphic3d_Vec3& aNode1 = aNodes[aTrg[m_n0]];
            const Graphic3d_Vec3& aNode2 = aNodes[aTrg[m_n1]];
            const Graphic3d_Vec3& aNode3 = aNodes[aTrg[m_n2]];

            aRes = theMgr.OverlapsTriangle(gp_Pnt(aNode1.x(), aNode1.y(), aNode1.z()),
                                           gp_Pnt(aNode2.x(), aNode2.y(), aNode2.z()),
                                           gp_Pnt(aNode3.x(), aNode3.y(), aNode3.z()),
                                           Select3D_TOS_INTERIOR,
                                           thePickResult);
        }
    }

    if (aRes && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        myDetectedIndices.Add(anElemIdx);
    }

    return aRes;
}

// =======================================================================
// function : elementIsInside
// =======================================================================
template<class Elem_T>
Standard_Boolean MeshAdaptorSensitiveElement<Elem_T>::elementIsInside(SelectBasics_SelectingVolumeManager& theMgr,
                                                                      Standard_Integer                     theElemIdx,
                                                                      Standard_Boolean theIsFullInside)
{
    const Standard_Integer anElemIdx = myBVHPrimIndexes->Value(theElemIdx);
    if (theIsFullInside) {
        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
            myDetectedIndices.Add(anElemIdx);
        }
        return true;
    }

    const Elem_T& anElem = myMeshAdaptor.GetElement(anElemIdx);

    Graphic3d_Vec3 aNode;
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
        getNodePosition(aNode, anElem[aNodeIter]);
        if (!theMgr.OverlapsPoint(gp_Pnt(aNode.x(), aNode.y(), aNode.z()))) {
            return false;
        }
    }

    if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        myDetectedIndices.Add(anElemIdx);
    }
    return true;
}

// =======================================================================
// function : Init
// =======================================================================
void MeshAdaptorSensitiveNode::Init()
{
    MarkDirty();

    const RWCae::MeshEntityId aNbNodes = !myMeshGeom->GetMeshData().IsNull() ? myMeshGeom->GetMeshData()->NbNodes()
                                                                               : 0;
    if (aNbNodes == 0) {
        myBVHPrimIndexes.Nullify();
        return;
    }

    if (!myBVHPrimIndexes.IsNull()) {
        myBVHPrimIndexes->Resize(0, aNbNodes - 1, false);
    } else {
        myBVHPrimIndexes = new TColStd_HArray1OfInteger(0, aNbNodes - 1);
    }
    TColStd_Array1OfInteger& aBVHPrimIdxs = myBVHPrimIndexes->ChangeArray1();

    Graphic3d_Vec3 aCenter(0.0, 0.0, 0.0);
    Graphic3d_Vec3 aNode;
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++) {
        getNodePosition(aNode, aNodeIter);
        aCenter += aNode;
        aBVHPrimIdxs(aNodeIter) = aNodeIter;
    }

    aCenter /= (float)aNbNodes;
    myCDG3D = gp_Pnt(aCenter.x(), aCenter.y(), aCenter.z());
    computeBoundingBox();
}

// =======================================================================
// function : Box
// =======================================================================
Select3D_BndBox3d MeshAdaptorSensitiveNode::Box(const Standard_Integer theIdx) const
{
    const Standard_Integer aNodeIdx = myBVHPrimIndexes->Value(theIdx);

    Graphic3d_Vec3 aNode;
    getNodePosition(aNode, aNodeIdx);
    return Select3D_BndBox3d(SelectMgr_Vec3(aNode), SelectMgr_Vec3(aNode));
}

// =======================================================================
// function : computeBoundingBox
// =======================================================================
inline void MeshAdaptorSensitiveNode::computeBoundingBox()
{
    myBndBox.Clear();

    const RWCae::MeshEntityId aNbNodes = !myMeshGeom->GetMeshData().IsNull() ? myMeshGeom->GetMeshData()->NbNodes()
                                                                               : 0;

    Graphic3d_Vec3 aNode;
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++) {
        getNodePosition(aNode, aNodeIter);
        myBndBox.Add(SelectMgr_Vec3(aNode));
    }
}

// =======================================================================
// function : overlapsElement
// =======================================================================
Standard_Boolean MeshAdaptorSensitiveNode::overlapsElement(SelectBasics_PickResult&             thePickResult,
                                                           SelectBasics_SelectingVolumeManager& theMgr,
                                                           Standard_Integer                     theElemIdx,
                                                           Standard_Boolean                     theIsFullInside)
{
    const Standard_Integer aNodeIdx = myBVHPrimIndexes->Value(theElemIdx);
    Graphic3d_Vec3         aNode;
    getNodePosition(aNode, aNodeIdx);
    if (!myMeshGeom->AcceptNode(aNodeIdx, aNode)) {
        return false;
    }

    if (theIsFullInside) {
        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
            myDetectedIndices.Add(aNodeIdx);
        }
        return true;
    }

    bool aRes = theMgr.OverlapsPoint(gp_Pnt(aNode.x(), aNode.y(), aNode.z()), thePickResult);
    if (aRes && theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        myDetectedIndices.Add(aNodeIdx);
    }
    return aRes;
}

// =======================================================================
// function : elementIsInside
// =======================================================================
Standard_Boolean MeshAdaptorSensitiveNode::elementIsInside(SelectBasics_SelectingVolumeManager& theMgr,
                                                           Standard_Integer                     theElemIdx,
                                                           Standard_Boolean                     theIsFullInside)
{
    const Standard_Integer aNodeIdx = myBVHPrimIndexes->Value(theElemIdx);
    Graphic3d_Vec3         aNode;
    getNodePosition(aNode, aNodeIdx);
    if (theIsFullInside) {
        if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
            myDetectedIndices.Add(aNodeIdx);
        }
        return true;
    }

    if (!theMgr.OverlapsPoint(gp_Pnt(aNode.x(), aNode.y(), aNode.z()))) {
        return false;
    }

    if (theMgr.GetActiveSelectionType() != SelectMgr_SelectionType_Point) {
        myDetectedIndices.Add(aNodeIdx);
    }
    return true;
}

template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElement0d>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementTriangle>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementTriangle2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementQuad>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementQuad2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPolygon>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementTetrahedron>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementTetrahedron2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPyramid>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPyramid2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPrism>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPrism2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementHexahedron>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementHexahedron2nd>;
template ExportTemplate class MeshAdaptorSensitiveElement<RWCae::MeshElementPolyhedron>;

} // namespace


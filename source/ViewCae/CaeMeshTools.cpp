#include <ViewCae/CaeMeshTools.h>

#include <ViewBase/PrsTools.h>

#include <RWBase/TaskManager.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <Message.hxx>
#include <Poly_MergeNodesTool.hxx>

namespace ViewCae {
// ================================================================
// Function : CopyAttributesFrom
// ================================================================
void CaeMeshArrayOfTriangles::CopyAttributesFrom(const Handle(CaeMeshArrayOfTriangles)& theFromArray,
                                                 RWCae::MeshElemEntityOffset          theFromVertex,
                                                 RWCae::MeshElemEntityOffset          theToVertex,
                                                 RWCae::MeshElemEntityOffset          theNbVertices)
{
    if (theNbVertices == 0) {
        return;
    }

    if (theFromArray->Attributes()->IsInterleaved()) {
        // Interleaved copying
        const Handle(Graphic3d_Buffer)& aSrcBuff = theFromArray->Attributes();
        std::copy(aSrcBuff->Data() + aSrcBuff->Stride * theFromVertex,
                  aSrcBuff->Data() + aSrcBuff->Stride * (theFromVertex + theNbVertices),
                  Attributes()->ChangeData() + Attributes()->Stride * theToVertex);
    } else {
        // Deinterleaved copying
        const Handle(Graphic3d_Buffer)& aSrcBuff  = theFromArray->Attributes();
        const Handle(Graphic3d_Buffer)& aDestBuff = Attributes();
        // Iterate over attributes to get offsets to where we should write individual attributes.
        for (int anIter = 0; anIter < aSrcBuff->NbAttributes; anIter++) {
            // Offset at which the beginning of attribute data is located in the source array.
            size_t               aSrcAttrStride = 0;
            const Standard_Byte* aSrcAttrData   = aSrcBuff->AttributeData(anIter, aSrcAttrStride);
            const Standard_Byte* aFromPtr       = aSrcAttrData + theFromVertex * aSrcAttrStride;
            // Individual size of source attribute subrange.
            const size_t aFromSize = static_cast<size_t>(theNbVertices) * aSrcAttrStride;

            // Offset to which we need to copy attribute data.
            size_t         aDestAttrStride = 0;
            Standard_Byte* aDestAttrData   = aDestBuff->ChangeAttributeData(anIter, aDestAttrStride);
            Standard_Byte* aToPtr          = aDestAttrData + theToVertex * aDestAttrStride;

            // Asserts for preventing read/write overflow.
            Standard_RangeError_Raise_if(aFromPtr + aFromSize > aSrcBuff->Data() + aSrcBuff->Size()
                                             || aFromPtr < aSrcBuff->Data(),
                                         "Attempted buffer read overflow.");
            Standard_RangeError_Raise_if(aToPtr + aFromSize > aDestBuff->Data() + aDestBuff->Size()
                                             || aToPtr < aDestBuff->Data(),
                                         "Attempted buffer write overflow.");
            std::copy(aFromPtr, aFromPtr + aFromSize, aToPtr);
        }
    }
}

// ================================================================
// Function : CreateFrom
// ================================================================
Handle(CaeMeshArrayOfTriangles) CaeMeshArrayOfTriangles::CreateFrom(const Handle(CaeMeshArrayOfTriangles)& theOther,
                                                                    int                                    theNbVerts,
                                                                    int                                    theNbIndices)
{
    return new CaeMeshArrayOfTriangles(theNbVerts,
                                       theNbIndices,
                                       theOther->HasVertexNormals(),
                                       theOther->HasVertexColors(),
                                       theOther->HasVertexTexels());
}

// ================================================================
// Function : GetMeshEdgesStats
// ================================================================
CaeMeshTools::ArrayInfo CaeMeshTools::GetMeshEdgesStats(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                        MeshShadedType                            theShadedType)
{
    CaeMeshTools::ArrayInfo aResInfo;

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();

        if (aMeshData.IsNull()) {
            continue;
        }

        CaeMeshTools::ArrayInfo anInfo;

        auto perform = [&anInfo](const auto& theAdaptor) {
            getMeshEdgesNb(theAdaptor, anInfo);
        };

        if (theShadedType == MeshShadedType::FACES) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME) {
            RWCae::MeshAdaptorsIterator3dElems::For(perform, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
        }

        aResInfo.Add(anInfo);
    }
    return aResInfo;
}

// =======================================================================
// function : getMeshEdgesNb
// =======================================================================
template<class Elem_T>
void CaeMeshTools::getMeshEdgesNb(const RWCae::MeshAdaptor<Elem_T>& theMeshAdaptor, ArrayInfo& theInfo)
{
    if (theMeshAdaptor.NbElements() == 0) {
        return;
    }

    static constexpr int nbSNodes2 = 2;

    theInfo.nbNodes += theMeshAdaptor.NbNodes();

    NCollection_DataMap<RWCae::MeshElemEntityId, RWCae::MeshElemEntityOffset> aNbFaces;
    theMeshAdaptor.NbFaces(aNbFaces);
    for (NCollection_DataMap<RWCae::MeshElemEntityId, RWCae::MeshElemEntityOffset>::Iterator anIter(aNbFaces);
         anIter.More();
         anIter.Next()) {
        theInfo.nbIndices += static_cast<int>(anIter.Key() * anIter.Value() * nbSNodes2);
    }
}

// =======================================================================
// function : addMeshEdges
// =======================================================================
template<class Elem_T>
inline void CaeMeshTools::addMeshEdges(const Handle(Graphic3d_ArrayOfSegments)&  theSegments,
                                       const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                       const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    RWCae::MeshEntityId aNbElems = theMeshAdaptor.NbElements();
    if (aNbElems == 0) {
        return;
    }

    RWCae::MeshEntityId aNbNodes   = theMeshAdaptor.NbNodes();
    const int             aFirstNode = theSegments->VertexNumber() + 1;
    Graphic3d_Vec3        aNode;
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter) {
        theMeshAdaptor.NodePosition(aNode, aNodeIter);
        theMeshGeom->ModifyNode(aNodeIter, aNode);
        theSegments->AddVertex(aNode);
    }

    Handle(NCollection_IncAllocator) anAllocator = new NCollection_IncAllocator();

    NCollection_Map<Graphic3d_Vec2i, ViewBase::PrsTools::SegmentsHasher> aPairsMap(aNbElems, anAllocator);

    auto processFace = [&theMeshAdaptor, &theSegments, &aPairsMap, aFirstNode](const auto&               theElem,
                                                                               RWCae::MeshElemEntityId theFaceIter) {
        RWCae::MeshElemEntityId aNbFaceNodes = theMeshAdaptor.NbFaceNodes(theElem, theFaceIter);

        RWCae::MeshEntityId aPrevNodeId = theMeshAdaptor.FaceNode(theElem, theFaceIter, 0);
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
            RWCae::MeshEntityId aNodeId = theMeshAdaptor.FaceNode(theElem,
                                                                    theFaceIter,
                                                                    (aNodeIter + 1) % aNbFaceNodes);
            Graphic3d_Vec2i       aSeg(aPrevNodeId, aNodeId);
            if (aPairsMap.Add(aSeg)) {
                theSegments->AddEdge(aFirstNode + aSeg[0]);
                theSegments->AddEdge(aFirstNode + aSeg[1]);
            }
            aPrevNodeId = aNodeId;
        }
    };

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theMeshAdaptor.GetElement(anElemIter);
        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        RWCae::MeshElemEntityId aNbFaces = theMeshAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            processFace(anElem, aFaceIter);
        }
    }
}

// ================================================================
// Function : CreateMeshFreeEdges
// ================================================================
Handle(Graphic3d_ArrayOfSegments) CaeMeshTools::CreateMeshEdges(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                                MeshShadedType                            theShadedType)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    ArrayInfo aCfg = GetMeshEdgesStats(aMeshGeom, theShadedType);
    if (aCfg.nbNodes == 0 || aCfg.nbIndices == 0) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments(aCfg.nbNodes, aCfg.nbIndices);

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        auto perform = [&aSegs, &aGeom](const auto& theAdaptor) {
            addMeshEdges(aSegs, theAdaptor, aGeom);
        };

        if (theShadedType == MeshShadedType::FACES) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME) {
            RWCae::MeshAdaptorsIterator3dElems::For(perform, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
        }
    }

    if (aSegs->Indices()->NbElements == 0) {
        aSegs.Nullify();
    }

    return aSegs;
}

// =======================================================================
// function : addMeshFreeEdges
// =======================================================================
template<class Elem_T>
inline void CaeMeshTools::addMeshFreeEdges(NCollection_Vector<Graphic3d_Vec3>&       theSegments,
                                           const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();
    if (aNbElems == 0) {
        return;
    }

    Handle(NCollection_IncAllocator) anAllocator = new NCollection_IncAllocator();

    NCollection_Map<Graphic3d_Vec2i, ViewBase::PrsTools::SegmentsHasher> aPairsMap(aNbElems, anAllocator);

    auto processFace = [&theAdaptor, &aPairsMap](const auto& theElem, RWCae::MeshElemEntityId theFaceIter) {
        RWCae::MeshElemEntityId aNbFaceNodes = theAdaptor.NbFaceNodes(theElem, theFaceIter);

        RWCae::MeshEntityId aPrevNodeId = theAdaptor.FaceNode(theElem, theFaceIter, 0);
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
            RWCae::MeshEntityId aNodeId = theAdaptor.FaceNode(theElem, theFaceIter, (aNodeIter + 1) % aNbFaceNodes);
            Graphic3d_Vec2i       aSeg(aPrevNodeId, aNodeId);
            if (!aPairsMap.Add(aSeg)) {
                aPairsMap.Remove(aSeg);
            }
            aPrevNodeId = aNodeId;
        }
    };

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theAdaptor.GetElement(anElemIter);
        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        RWCae::MeshElemEntityId aNbFaces = theAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            processFace(anElem, aFaceIter);
        }
    }

    Graphic3d_Vec3 aNodes[2];
    for (NCollection_Map<Graphic3d_Vec2i, ViewBase::PrsTools::SegmentsHasher>::Iterator anIter(aPairsMap);
         anIter.More();
         anIter.Next()) {
        const Graphic3d_Vec2i& aSeg = anIter.Key();
        theAdaptor.NodePosition(aNodes[0], aSeg[0]);
        theAdaptor.NodePosition(aNodes[1], aSeg[1]);
        theMeshGeom->ModifyNode(aSeg[0], aNodes[0]);
        theMeshGeom->ModifyNode(aSeg[1], aNodes[1]);
        theSegments.Append(aNodes[0]);
        theSegments.Append(aNodes[1]);
    }
}

// ================================================================
// Function : CreateMeshFreeEdges
// ================================================================
Handle(Graphic3d_ArrayOfSegments) CaeMeshTools::CreateMeshFreeEdges(
    const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
    MeshShadedType                            theShadedType)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    NCollection_Vector<Graphic3d_Vec3> aFreeSegVec;
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        auto perform = [&aFreeSegVec, &aGeom](const auto& theAdaptor) {
            addMeshFreeEdges(aFreeSegVec, theAdaptor, aGeom);
        };

        if (theShadedType == MeshShadedType::FACES) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
        }
    }
    if (aFreeSegVec.IsEmpty()) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments(aFreeSegVec.Size(), 0);
    for (const Graphic3d_Vec3& aSegiter : aFreeSegVec) {
        aSegs->AddVertex(aSegiter);
    }
    return aSegs;
}

// =======================================================================
// function : mergedTriangulation
// =======================================================================
template<class Elem_T>
inline Handle(Poly_Triangulation) CaeMeshTools::mergedTriangulation(
    const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
    const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    static constexpr double mergeAngle = M_PI / 8.0;

    Graphic3d_Vec3 aNode;

    RWCae::MeshEntityId aNbElems = theMeshAdaptor.NbElements();
    if (aNbElems == 0) {
        return Handle(Poly_Triangulation)();
    }

    Handle(Poly_MergeNodesTool) aMergeTool = new Poly_MergeNodesTool(mergeAngle, 0.0, aNbElems);
    aMergeTool->ChangeOutput()->SetDoublePrecision(false);

    auto processTrg = [&aMergeTool, &theMeshAdaptor, &theMeshGeom, &aNode](const Graphic3d_Vec3u& theTrg) {
        for (int aNodeIter = 0; aNodeIter < 3; ++aNodeIter) {
            // it might be better merging untransformed elements
            // and apply transformation while adding edges to presentation,
            // but this would require collecting index map
            theMeshAdaptor.NodePosition(aNode, theTrg[aNodeIter]);
            theMeshGeom->ModifyNode(theTrg[aNodeIter], aNode);

            aMergeTool->ChangeElementNode(aNodeIter).SetCoord(aNode.x(), aNode.y(), aNode.z());
        }
        aMergeTool->PushLastElement(3);
    };

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theMeshAdaptor.GetElement(anElemIter);
        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            // clipping elements here might be not what user would want to see...
            continue;
        }

        RWCae::MeshElemEntityId aNbFaces = theMeshAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(theMeshAdaptor, anElem, aFaceIter);
                 aTrgIter.More();
                 aTrgIter.Next()) {
                processTrg(aTrgIter.Triangle());
            }
        }
    }
    return aMergeTool->Result();
}

// =======================================================================
// function : addMeshSharpEdges
// =======================================================================
template<class Elem_T>
inline void CaeMeshTools::addMeshSharpEdges(NCollection_Vector<Graphic3d_Vec3>&       theSegments,
                                            const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                            const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    Handle(Poly_Triangulation) aMergedTri = mergedTriangulation(theMeshAdaptor, theMeshGeom);
    if (aMergedTri.IsNull()) {
        return;
    }

    // collect the edges sharing statistics
    Graphic3d_Vec2i aSegs[3];

    Handle(NCollection_IncAllocator) anAllocator = new NCollection_IncAllocator();

    using IndexedVec2iIntMap = NCollection_IndexedDataMap<Graphic3d_Vec2i, int, ViewBase::PrsTools::SegmentsHasher>;

    IndexedVec2iIntMap aPairsMap(aMergedTri->NbTriangles(), anAllocator);

    Graphic3d_Vec4i anElem;
    Graphic3d_Vec3  aNode;

    int aNbFreeEdges = 0;
    for (int aTriIter = 1; aTriIter <= aMergedTri->NbTriangles(); ++aTriIter) {
        aMergedTri->Triangle(aTriIter).Get(anElem[0], anElem[1], anElem[2]);

        static constexpr int aNbElemNodes = 3;

        aSegs[0] = Graphic3d_Vec2i(anElem[0], anElem[1]);
        aSegs[1] = Graphic3d_Vec2i(anElem[1], anElem[2]);
        aSegs[2] = Graphic3d_Vec2i(anElem[2], anElem[0]);
        for (int aSegIt = 0; aSegIt < aNbElemNodes; ++aSegIt) {
            const Graphic3d_Vec2i& aSeg = aSegs[aSegIt];

            const int anIndex  = aPairsMap.Add(aSeg, 0);
            int&      aCounter = aPairsMap.ChangeFromIndex(anIndex);
            ++aCounter;
            if (aCounter == 2) {
                --aNbFreeEdges;
            } else if (aCounter == 1) {
                ++aNbFreeEdges;
            }
        }
    }
    if (aNbFreeEdges == 0) {
        return;
    }

    for (IndexedVec2iIntMap::Iterator aSegIter(aPairsMap); aSegIter.More(); aSegIter.Next()) {
        if (aSegIter.Value() != 1) {
            continue;
        }

        const Graphic3d_Vec2i& aSeg = aSegIter.Key();

        const gp_Pnt& aNode1 = aMergedTri->Node(aSeg[0]);
        aNode.SetValues((float)aNode1.X(), (float)aNode1.Y(), (float)aNode1.Z());
        theSegments.Append(aNode);

        const gp_Pnt& aNode2 = aMergedTri->Node(aSeg[1]);
        aNode.SetValues((float)aNode2.X(), (float)aNode2.Y(), (float)aNode2.Z());
        theSegments.Append(aNode);
    }
}

// ================================================================
// Function : CreateMeshSharpEdges
// ================================================================
Handle(Graphic3d_ArrayOfSegments) CaeMeshTools::CreateMeshSharpEdges(
    const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
    MeshShadedType                            theShadedType)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    NCollection_Vector<Graphic3d_Vec3> aSharpSegVec;
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        try {
            OCC_CATCH_SIGNALS
            auto perform = [&aSharpSegVec, &aGeom](const auto& theAdaptor) {
                addMeshSharpEdges(aSharpSegVec, theAdaptor, aGeom);
            };

            if (theShadedType == MeshShadedType::FACES) {
                RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
            } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
                RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
            }
        } catch (const Standard_Failure& theFail) {
            Message::SendFail() << "Internal error in CreateMeshSharpEdges():/n" << theFail;
        }
    }
    if (aSharpSegVec.IsEmpty()) {
        return Handle(Graphic3d_ArrayOfSegments)();
    }

    Handle(Graphic3d_ArrayOfSegments) aSegs = new Graphic3d_ArrayOfSegments(aSharpSegVec.Size(), 0);
    for (const Graphic3d_Vec3& aSegiter : aSharpSegVec) {
        aSegs->AddVertex(aSegiter);
    }
    return aSegs;
}

// =======================================================================
// function : getShadedMeshElemsNb
// =======================================================================
template<class Elem_T>
inline void CaeMeshTools::getShadedMeshElemsNb(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                               const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                               ArrayInfo&                                theInfo,
                                               bool                                      toDuplicate)
{
    RWCae::MeshEntityId aNbElems = theMeshAdaptor.NbElements();
    if (aNbElems == 0) {
        return;
    }
    static constexpr RWCae::MeshEntityId aMinNbElemsToSplit = 10000;
    const int aNbParts = aNbElems > aMinNbElemsToSplit ? RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch()
                                                       : 1;
    RWCae::MeshEntityId aPartNbElems = (RWCae::MeshEntityId)Ceiling((double)aNbElems / aNbParts);

    ArrayInfo::ElemNodeOffsets* anInfo = theInfo.parts.Bound(std::make_pair(theMeshGeom, (int)Elem_T::Type),
                                                             ArrayInfo::ElemNodeOffsets(aNbParts));

    for (int aPart = 0; aPart < aNbParts; aPart++) {
        RWCae::MeshEntityId aFirstElemId = aPart * aPartNbElems;
        RWCae::MeshEntityId aLastElemId  = std::min(aFirstElemId + aPartNbElems, aNbElems) - 1;

        (*anInfo)[aPart].first = aLastElemId + 1;

        uint64_t aNbTrgs = theMeshAdaptor.NbTriangles(aFirstElemId, aLastElemId);
        if (toDuplicate) {
            theInfo.nbNodes += static_cast<int>(aNbTrgs * m_nbTNodes3);
            (*anInfo)[aPart].second = aNbTrgs * m_nbTNodes3 + (aPart > 0 ? (*anInfo)[aPart - 1].second : 0);
        } else {
            theInfo.nbNodes += theMeshAdaptor.NbNodes();
            theInfo.nbIndices += static_cast<int>(aNbTrgs * m_nbTNodes3);
        }
    }
}

// ================================================================
// Function : GetShadedMeshStats
// ================================================================
CaeMeshTools::ArrayInfo CaeMeshTools::GetShadedMeshStats(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                         MeshShadedType                            theShadedType,
                                                         const bool                                toDuplicate)
{
    CaeMeshTools::ArrayInfo aResInfo;

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();

        if (aMeshData.IsNull()) {
            continue;
        }

        auto compute = [&aGeom, &aResInfo, toDuplicate](const auto& theAdaptor) {
            getShadedMeshElemsNb(aGeom, theAdaptor, aResInfo, toDuplicate);
        };

        if (theShadedType == MeshShadedType::FACES) {
            RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME) {
            RWCae::MeshAdaptorsIterator3dElems::For(compute, aMeshData);
        } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
            RWCae::MeshAdaptorsIterator2dElems::For(compute, aMeshData, true);
        }
    }
    return aResInfo;
}

// =======================================================================
// function : addShadedMesh
// =======================================================================
void CaeMeshTools::addShadedMesh(const Handle(CaeMeshArrayOfTriangles)&        theTris,
                                 const CaeMeshTools::ArrayInfo&                theInfo,
                                 MeshShadedType                                theShadedType,
                                 const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                 const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                 const bool                                    theSmoothNormals,
                                 const float                                   theShrink)
{
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(theMeshGeom.get())) {
        Handle(RWCae::MeshData) aMesh = aGeom->GetMeshData();
        if (aMesh.IsNull()) {
            continue;
        }

        RWCae::MeshPropertyComponent aPropComponent;
        theGeomProps.Find(aGeom->Id(), aPropComponent);
        Handle(RWCae::MeshScalarPropertyBase) aProp = aPropComponent.GetScalarComp();

        auto perform = [&theTris, &theInfo, &aProp, &aGeom, theSmoothNormals, theShrink](const auto& theAdaptor) {
            const ArrayInfo::ElemNodeOffsets* anInfo = theInfo.parts.Seek(
                std::make_pair(aGeom, (int)theAdaptor.GetElementType()));
            if (anInfo == nullptr) {
                return;
            }
            addShadedMeshT(theTris, *anInfo, theAdaptor, aProp, aGeom, theSmoothNormals, theShrink);
        };

        if (theShadedType == MeshShadedType::FACES) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMesh);
        } else if (theShadedType == MeshShadedType::VOLUME) {
            RWCae::MeshAdaptorsIterator3dElems::For(perform, aMesh);
        } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
            RWCae::MeshAdaptorsIterator2dElems::For(perform, aMesh, true);
        }
    }
}

// =======================================================================
// function : addShadedMeshT
// =======================================================================
template<class Elem_T>
void CaeMeshTools::addShadedMeshT(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                  const ArrayInfo::ElemNodeOffsets&              theInfo,
                                  const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                  const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                  const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                                  const bool                                     theSmoothNormals,
                                  const float                                    theShrink)
{
    if ((!theProp.IsNull() && theProp->IsElemental()) || !theSmoothNormals || theShrink < 1.0f) {
        // separate vertices for each triangle
        addShadedMeshSplit(theTris, theInfo, theMesh, theProp, theMeshGeom, theSmoothNormals, theShrink);
    } else {
        addShadedMeshMerged(theTris, theMesh, theProp, theMeshGeom);
    }
}

// =======================================================================
// function : addShadedMeshSplit
// =======================================================================
template<class Elem_T>
void CaeMeshTools::addShadedMeshSplit(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                      const ArrayInfo::ElemNodeOffsets&              theParts,
                                      const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                      const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                      const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                                      const bool                                     theSmoothNormals,
                                      const float                                    theShrink)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return;
    }

    int aPrevNbNodes = theTris->VertexNumber();

    std::vector<RWCae::MeshElemEntityOffset> aNbAddedNodes(theParts.size(), 0);

    auto processPart = [&](int, int thePart) {
        aNbAddedNodes[thePart] = addShadedMeshSplitPart(theTris,
                                                        theParts,
                                                        theMesh,
                                                        theProp,
                                                        theMeshGeom,
                                                        theSmoothNormals,
                                                        theShrink,
                                                        thePart,
                                                        aPrevNbNodes);
    };

    theTris->SetVertexNumber(static_cast<int>(theParts.back().second) + aPrevNbNodes);

    RWBase::TaskManager::Instance()->RunInParallel(0, (int)theParts.size(), processPart);

    RWCae::MeshElemEntityOffset aNbAdded = 0;
    for (size_t aPart = 0; aPart < theParts.size(); aPart++) {
        if (aPart != 0 && theParts[aPart - 1].second > aNbAdded) {
            theTris->CopyAttributesFrom(theTris,
                                        theParts[aPart - 1].second + aPrevNbNodes,
                                        aNbAdded + aPrevNbNodes,
                                        aNbAddedNodes[aPart]);
        }
        aNbAdded += aNbAddedNodes[aPart];
    }
    theTris->SetVertexNumber(static_cast<int>(aNbAdded) + aPrevNbNodes);
}

// =======================================================================
// function : addShadedMeshSplitPart
// =======================================================================
template<class Elem_T>
RWCae::MeshElemEntityOffset CaeMeshTools::addShadedMeshSplitPart(
    const Handle(CaeMeshArrayOfTriangles)&          theTris,
    const CaeMeshTools::ArrayInfo::ElemNodeOffsets& theParts,
    const RWCae::MeshAdaptor<Elem_T>&             theMesh,
    const Handle(RWCae::MeshScalarPropertyBase)&  theProp,
    const Handle(ViewCaeGeom::MeshGeometry)&       theMeshGeom,
    const bool                                      theSmoothNormals,
    const float                                     theShrink,
    int                                             thePart,
    int                                             theVertsOffset)
{
    RWCae::MeshElemEntityOffset aNbAddedVerts = 0;

    RWCae::MeshAdaptor<Elem_T> anAdaptor(theMesh);

    Graphic3d_Vec2d aValMinMax(0.0, 1.0);
    if (!theProp.IsNull())
        theProp->ValueRange().GetBounds(aValMinMax.x(), aValMinMax.y());

    Graphic3d_Vec3 aNorm;
    Graphic3d_Vec3 anElemCenter;

    NCollection_Array1<Graphic3d_Vec3> aTriNodes(0, m_nbTNodes3 - 1);
    std::vector<RWCae::MeshEntityId> aNodesIds;
    std::vector<Graphic3d_Vec3>        aNodes;

    int aStartVertIdx = theVertsOffset + (thePart > 0 ? int(theParts[thePart - 1].second) : 0);

    static constexpr int            aNbTempArrayVerts = 1024;
    Handle(CaeMeshArrayOfTriangles) aTempTris = CaeMeshArrayOfTriangles::CreateFrom(theTris, aNbTempArrayVerts, 0);

    RWCae::MeshEntityId anElemLower = thePart > 0 ? theParts[thePart - 1].first : 0;
    RWCae::MeshEntityId anElemUpper = theParts[thePart].first - 1;

    for (RWCae::MeshEntityId anElemIter = anElemLower; anElemIter <= anElemUpper; ++anElemIter) {
        const Elem_T& anElem = anAdaptor.GetElement(anElemIter);

        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        anAdaptor.ElementNodes(anElem, aNodesIds);

        getAndModifyNodes(aNodes, aNodesIds, anAdaptor, theMeshGeom);

        if (theShrink < 1.0f) {
            nodesCenter(aNodes, anElemCenter);
        }

        RWCae::MeshElemEntityId aNbFaces = anAdaptor.NbFaces(anElem);

        using TriangleIterator = typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator;
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            for (TriangleIterator aTrgIter(anAdaptor, anElem, aFaceIter); aTrgIter.More(); aTrgIter.Next()) {
                getTriNodes(aTriNodes, aNodes, aTrgIter.TriangleIndices(), anElemCenter, theShrink);
                addShadedMeshTrg(theTris,
                                 aTempTris,
                                 theProp,
                                 aValMinMax,
                                 anElem,
                                 aTrgIter.Triangle(),
                                 aTriNodes,
                                 theSmoothNormals,
                                 aStartVertIdx,
                                 aNorm,
                                 aNbAddedVerts);
                // TO DO smooth normals @c if (theTris->HasVertexNormals()) {}
            }
        }
    }

    theTris->CopyAttributesFrom(aTempTris, 0, aStartVertIdx + aNbAddedVerts, aTempTris->VertexNumber());
    aNbAddedVerts += aTempTris->VertexNumber();
    aTempTris->SetVertexNumber(0);

    return aNbAddedVerts;
}

// =======================================================================
// function : addShadedMeshTrg
// =======================================================================
template<class Elem_T>
void CaeMeshTools::addShadedMeshTrg(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                    const Handle(CaeMeshArrayOfTriangles)&         theTempTris,
                                    const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                    const Graphic3d_Vec2d&                         theValMinMax,
                                    const Elem_T&                                  theElem,
                                    const Graphic3d_Vec3u&                         theTrg,
                                    const NCollection_Array1<Graphic3d_Vec3>&      theTriNodes,
                                    const bool                                     theSmoothNormals,
                                    const int                                      theStartVertIdx,
                                    Graphic3d_Vec3&                                theNorm,
                                    RWCae::MeshElemEntityOffset&                 theNbAddedVerts)
{
    if (theTris->HasVertexNormals() && !theSmoothNormals && !getTriangleNormal(theNorm, theTriNodes))
        return; // skip degenerate element

    for (int anIter = 0; anIter < 3; ++anIter) {
        if (theTempTris->VertexNumber() == theTempTris->VertexNumberAllocated()) {
            theTris->CopyAttributesFrom(theTempTris, 0, theStartVertIdx + theNbAddedVerts, theTempTris->VertexNumber());
            theNbAddedVerts += theTempTris->VertexNumber();
            theTempTris->SetVertexNumber(0);
        }
        int aNewVertId = theTempTris->AddVertex(theTriNodes[anIter]);
        if (!theProp.IsNull()) {
            const int aPropIdx = theProp->IsElemental() ? theElem.GlobalIndex() : theTrg[anIter];

            const double   aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(aPropIdx, theTrg[anIter]));
            Graphic3d_Vec2 aTexUV = GetUVCoords(aValue, theValMinMax[0], theValMinMax[1], NbColorscaleColors);
            theTempTris->SetVertexTexel(aNewVertId, aTexUV.x(), aTexUV.y());
        }

        if (theTempTris->HasVertexNormals() && !theSmoothNormals)
            theTempTris->SetVertexNormal(aNewVertId, theNorm.x(), theNorm.y(), theNorm.z());
    }
}

// =======================================================================
// function : addShadedMeshMerged
// =======================================================================
template<class Elem_T>
void CaeMeshTools::addShadedMeshMerged(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                       const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                       const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                       const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom)
{
    // This function is not used for now and should be checked and revised.
    // Problems to fix:
    // - all nodes are added for each elements type
    // - fill primitive array in parallel
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return;
    }

    const int       aNbColors = CaeMeshTools::NbColorscaleColors;
    Graphic3d_Vec2d aValMinMax(0.0, 1.0);
    if (!theProp.IsNull()) {
        theProp->ValueRange().GetBounds(aValMinMax.x(), aValMinMax.y());
    }

    const int aFirstNode = theTris->VertexNumber() + 1;

    Graphic3d_Vec3 aNode;

    RWCae::MeshEntityId aNbNodes = theMesh.NbNodes();
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter) {
        theMesh.NodePosition(aNode, aNodeIter);

        theMeshGeom->ModifyNode(aNodeIter, aNode);

        const int aNodeIndex = theTris->AddVertex(aNode);

        if (theTris->HasVertexTexels()) {
            const double   aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(aNodeIter));
            Graphic3d_Vec2 aTexUV = GetUVCoords(aValue, aValMinMax[0], aValMinMax[1], aNbColors);
            theTris->SetVertexTexel(aNodeIndex, aTexUV.x(), aTexUV.y());
        }
    }

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theMesh.GetElement(anElemIter);
        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        RWCae::MeshElemEntityId aNbFaces = theMesh.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
            for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(theMesh, anElem, aFaceIter);
                 aTrgIter.More();
                 aTrgIter.Next()) {
                const Graphic3d_Vec3u& aTrg = aTrgIter.Triangle();
                theTris->AddEdges(aTrg[0] + aFirstNode, aTrg[1] + aFirstNode, aTrg[2] + aFirstNode);
            }
        }
    }

    if (theTris->HasVertexNormals()) {
        // TO DO smooth normals
    }
}

// ================================================================
// Function : CreateShadedMesh
// ================================================================
Handle(Graphic3d_ArrayOfTriangles) CaeMeshTools::CreateShadedMesh(
    const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
    const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
    bool                                          theSmoothNormals,
    MeshShadedType                                theShadedType,
    const float                                   theShrink)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfTriangles)();
    }

    RWCae::MeshPropertyComponent aPropComponent;
    theGeomProps.Find(aMeshGeom->Id(), aPropComponent);
    Handle(RWCae::MeshScalarPropertyBase) aBaseProp = aPropComponent.GetScalarComp();

    bool toDuplicate = (!aBaseProp.IsNull() && aBaseProp->IsElemental()) || !theSmoothNormals || theShrink < 1.0f;

    CaeMeshTools::ArrayInfo anInfo = GetShadedMeshStats(aMeshGeom, theShadedType, toDuplicate);

    if ((anInfo.nbNodes == 0 && toDuplicate) || (anInfo.nbIndices == 0 && !toDuplicate)) {
        return Handle(Graphic3d_ArrayOfTriangles)();
    }

    bool hasColors  = false;
    bool hasNormals = true;
    bool hasUVs     = !aBaseProp.IsNull();

    Handle(CaeMeshArrayOfTriangles) aTris =
        new CaeMeshArrayOfTriangles(anInfo.nbNodes, anInfo.nbIndices, hasNormals, hasColors, hasUVs);

    addShadedMesh(aTris, anInfo, theShadedType, aMeshGeom, theGeomProps, theSmoothNormals, theShrink);

    fillDeinterleavedArrayOfTriangles(aTris);

    if (!toDuplicate && aTris->Indices()->NbElements == 0) {
        aTris.Nullify();
    }

    return aTris;
}

// ================================================================
// Function : CreateNodes
// ================================================================
Handle(Graphic3d_ArrayOfPoints) CaeMeshTools::CreateNodes(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                                          const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                                                          bool                                          toShowFreeNodes)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfPoints)();
    }

    int aNbNodes = 0;
    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();

        if (aMeshData.IsNull()) {
            continue;
        }
        aNbNodes += aMeshData->NbNodes();
    }
    if (aNbNodes == 0) {
        return Handle(Graphic3d_ArrayOfPoints)();
    }

    RWCae::MeshPropertyComponent aPropComponent;
    theGeomProps.Find(aMeshGeom->Id(), aPropComponent);
    Handle(RWCae::MeshScalarPropertyBase) aBaseProp = aPropComponent.GetScalarComp();

    const bool hasUVs = !aBaseProp.IsNull() && !aBaseProp->IsElemental();

    Graphic3d_ArrayFlags            aFlags  = hasUVs ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None;
    Handle(Graphic3d_ArrayOfPoints) aPoints = new Graphic3d_ArrayOfPoints(aNbNodes, aFlags);

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        RWCae::MeshPropertyComponent aGeomPropComponent;
        theGeomProps.Find(aGeom->Id(), aGeomPropComponent);
        Handle(RWCae::MeshScalarPropertyBase) aProp = aGeomPropComponent.GetScalarComp();

        addNodes(aPoints, aProp, aGeom, toShowFreeNodes, hasUVs);
    }

    return aPoints;
}

// ================================================================
// Function : addNodes
// ================================================================
void CaeMeshTools::addNodes(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                            const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                            const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                            bool                                           toShowFreeNodes,
                            bool                                           hasUVs)
{
    const int aNbColors = CaeMeshTools::NbColorscaleColors;

    Handle(RWCae::MeshData) aMeshData = theMeshGeom->GetMeshData();

    Graphic3d_Vec2d aValMinMax(0.0, 1.0);
    if (!theProp.IsNull()) {
        theProp->ValueRange().GetBounds(aValMinMax.x(), aValMinMax.y());
    }

    RWCae::MeshEntityId aNbNodes = aMeshData->NbNodes();
    Graphic3d_Vec3        aNode;
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; ++aNodeIter) {
        if (!toShowFreeNodes && aMeshData->IsNodeFree(aNodeIter)) {
            continue;
        }
        aNode = aMeshData->Nodes().Value(aNodeIter);
        theMeshGeom->ModifyNode(aNodeIter, aNode);

        if (!theMeshGeom->AcceptNode(aNodeIter, aNode)) {
            continue;
        }

        const int aNodeIdx = thePnts->AddVertex(aNode);
        if (hasUVs) {
            const double   aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(aNodeIter));
            Graphic3d_Vec2 aTexUV = GetUVCoords(aValue, aValMinMax[0], aValMinMax[1], aNbColors);
            thePnts->SetVertexTexel(aNodeIdx, aTexUV.x(), aTexUV.y());
        }
    }
}

// ================================================================
// Function : CreateElems0d
// ================================================================
Handle(Graphic3d_ArrayOfPoints) CaeMeshTools::CreateElems0d(const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                                                            const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfPoints)();
    }

    RWCae::MeshPropertyComponent aPropComponent;
    theGeomProps.Find(aMeshGeom->Id(), aPropComponent);
    Handle(RWCae::MeshScalarPropertyBase) aBaseProp = aPropComponent.GetScalarComp();

    Handle(Graphic3d_ArrayOfPoints) aPnts0d;

    const bool hasUVs = !aBaseProp.IsNull() && aBaseProp->IsElemental();

    Graphic3d_ArrayFlags aFlags = hasUVs ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None;

    int aNbElems = 0;
    for (int aPass = 0; aPass <= 1; ++aPass) {
        if (aPass == 1) {
            if (aNbElems == 0) {
                return Handle(Graphic3d_ArrayOfPoints)();
            }

            aPnts0d  = new Graphic3d_ArrayOfPoints(aNbElems, aFlags);
            aNbElems = 0;
        }

        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
            Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
            if (aMeshData.IsNull()) {
                continue;
            }

            RWCae::MeshPropertyComponent aGeomPropComponent;
            theGeomProps.Find(aGeom->Id(), aGeomPropComponent);
            Handle(RWCae::MeshScalarPropertyBase) aProp = aGeomPropComponent.GetScalarComp();

            RWCae::MeshAdaptor0d anAdaptor0d(*aMeshData);
            aNbElems += addCentroidsT(aPnts0d, anAdaptor0d, aProp, aGeom);
        }
    }

    return aPnts0d;
}

// ================================================================
// Function : CreateCentroids
// ================================================================
Handle(Graphic3d_ArrayOfPoints) CaeMeshTools::CreateCentroids(
    const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
    const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
    const MeshShadedType                          theShadedType)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theMeshGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return Handle(Graphic3d_ArrayOfPoints)();
    }

    RWCae::MeshPropertyComponent aPropComponent;
    theGeomProps.Find(aMeshGeom->Id(), aPropComponent);
    Handle(RWCae::MeshScalarPropertyBase) aBaseProp = aPropComponent.GetScalarComp();

    Handle(Graphic3d_ArrayOfPoints) aCentPnts;

    const bool hasUVs = !aBaseProp.IsNull() && aBaseProp->IsElemental();

    Graphic3d_ArrayFlags aFlags = hasUVs ? Graphic3d_ArrayFlags_VertexTexel : Graphic3d_ArrayFlags_None;

    int aNbElems = 0;
    for (int aPass = 0; aPass <= 1; ++aPass) {
        if (aPass == 1) {
            if (aNbElems == 0) {
                return Handle(Graphic3d_ArrayOfPoints)();
            }

            aCentPnts = new Graphic3d_ArrayOfPoints(aNbElems, aFlags);
            aNbElems  = 0;
        }

        for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
            Handle(RWCae::MeshData) aMeshData = aGeom->GetMeshData();
            if (aMeshData.IsNull()) {
                continue;
            }

            RWCae::MeshPropertyComponent aGeomPropComponent;
            theGeomProps.Find(aGeom->Id(), aGeomPropComponent);

            aNbElems += addCentroids(aCentPnts, aGeomPropComponent.GetScalarComp(), aGeom, theShadedType);
        }
    }

    return aCentPnts;
}

// ================================================================
// Function : addCentroids
// ================================================================
int CaeMeshTools::addCentroids(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                               const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                               const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                               const MeshShadedType                           theShadedType)
{
    Handle(RWCae::MeshData) aMeshData = theMeshGeom->GetMeshData();

    int aNbElems = 0;

    auto perform = [&thePnts, &theProp, &theMeshGeom, &aNbElems](const auto& theAdaptor) {
        aNbElems += addCentroidsT(thePnts, theAdaptor, theProp, theMeshGeom);
    };

    if (theShadedType == MeshShadedType::FACES) {
        RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
    } else if (theShadedType == MeshShadedType::VOLUME) {
        RWCae::MeshAdaptorsIterator3dElems::For(perform, aMeshData);
    } else if (theShadedType == MeshShadedType::VOLUME_SURFACE) {
        RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
    }

    return aNbElems;
}

// =======================================================================
// function : nbCentroidsT
// =======================================================================
template<class Elem_T>
int CaeMeshTools::nbCentroidsT(const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                               const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();

    int anElemCount = 0;
    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        if (theMeshGeom->AcceptElement(theMesh.GetElement(anElemIter).GlobalIndex())) {
            ++anElemCount;
        }
    }
    return anElemCount;
}

// =======================================================================
// function : addCentroidsT
// =======================================================================
template<class Elem_T>
int CaeMeshTools::addCentroidsT(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                                const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return 0;
    }

    if (thePnts.IsNull()) {
        return nbCentroidsT(theMesh, theMeshGeom);
    }

    const int       aNbColors = CaeMeshTools::NbColorscaleColors;
    Graphic3d_Vec2d aValMinMax(0.0, 1.0);
    if (!theProp.IsNull()) {
        theProp->ValueRange().GetBounds(aValMinMax.x(), aValMinMax.y());
    }

    int anElemCount = 0;

    std::vector<RWCae::MeshEntityId> aNodesIds;
    std::vector<Graphic3d_Vec3>        aNodes;
    Graphic3d_Vec3                     aCenter;

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; ++anElemIter) {
        const Elem_T& anElem = theMesh.GetElement(anElemIter);
        if (!theMeshGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        theMesh.ElementNodes(anElem, aNodesIds);
        getAndModifyNodes(aNodes, aNodesIds, theMesh, theMeshGeom);
        nodesCenter(aNodes, aCenter);

        ++anElemCount;

        const int aNodeIndex = thePnts->AddVertex(aCenter.x(), aCenter.y(), aCenter.z());
        if (!thePnts->HasVertexTexels()) {
            continue;
        }

        // what for MeshNodeVecProperty?
        if (theProp->IsElemental()) {
            const double   aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(anElem.GlobalIndex()));
            Graphic3d_Vec2 aTexUV = GetUVCoords(aValue, aValMinMax[0], aValMinMax[1], aNbColors);
            thePnts->SetVertexTexel(aNodeIndex, aTexUV.x(), aTexUV.y());
        } else {
            // nodal properties could be interpolated, but lets skip them for now
        }
    }
    return anElemCount;
}

// ================================================================
// Function : fillDeinterleavedArrayOfTriangles
// ================================================================
void CaeMeshTools::fillDeinterleavedArrayOfTriangles(const Handle(CaeMeshArrayOfTriangles)& theArray)
{
    if (theArray.IsNull()) {
        return;
    }

    Handle(Graphic3d_Buffer) anArrayAttrs = theArray->Attributes();
    if (anArrayAttrs->IsInterleaved()) {
        return;
    }

    const int aNbElems = anArrayAttrs->NbElements;
    if (aNbElems >= anArrayAttrs->NbMaxElements() || aNbElems == 0) {
        return;
    }

    // The chosen position to write is the first position.
    // This will write degenerate triangles to the buffer.
    // Other attributes are left uninitialized.
    const gp_Pnt aPos           = theArray->Vertice(1);
    const int    aNbElemsToFill = anArrayAttrs->NbMaxElements() - aNbElems;
    for (int anIter = 0; anIter < aNbElemsToFill; ++anIter) {
        theArray->AddVertex(aPos);
    }
}

// ================================================================
// Function : GetUVCoords
// ================================================================
Graphic3d_Vec2 CaeMeshTools::GetUVCoords(const double theValue,
                                         const double theMinValue,
                                         const double theMaxValue,
                                         const int    theNbValues)
{
    static constexpr double uinvalid  = 0.25;
    static constexpr double vinvalid  = 0.25;
    static constexpr double vvalid    = 1.75;
    static constexpr double halfTexel = 0.5;

    if (RWCae::IsNaN(theValue) || theMaxValue - theMinValue <= -gp::Resolution()) {
        return Graphic3d_Vec2(uinvalid, vinvalid);
    }

    double anInvRange = 1.0;
    if (theMaxValue - theMinValue <= gp::Resolution()) {
        if (Abs(theValue - theMinValue) > gp::Resolution()) {
            return Graphic3d_Vec2(uinvalid, vinvalid);
        }
    } else {
        anInvRange = 1.0 / (theMaxValue - theMinValue);
    }

    const double aNormVal = (theValue - theMinValue) * anInvRange;
    return Graphic3d_Vec2(float((aNormVal * (theNbValues - 1) + halfTexel) / theNbValues), float(vvalid));
}

// ================================================================
// Function : getAndModifyNodes
// ================================================================
template<class Elem_T>
void CaeMeshTools::getAndModifyNodes(std::vector<Graphic3d_Vec3>&              theNodes,
                                     const std::vector<RWCae::MeshEntityId>& theNodesIds,
                                     const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                     const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom)
{
    theNodes.resize(theNodesIds.size());
    for (size_t i = 0; i < theNodesIds.size(); i++) {
        theMesh.NodePosition(theNodes[i], theNodesIds[i]);
        theMeshGeom->ModifyNode(theNodesIds[i], theNodes[i]);
    }
}

// ================================================================
// Function : nodesCenter
// ================================================================
void CaeMeshTools::nodesCenter(std::vector<Graphic3d_Vec3>& theNodes, Graphic3d_Vec3& theCenter)
{
    if (theNodes.empty()) {
        return;
    }
    theCenter.SetValues(0.0f, 0.0f, 0.0f);
    for (const Graphic3d_Vec3& aNode : theNodes) {
        theCenter += aNode;
    }
    theCenter /= static_cast<float>(theNodes.size());
}

// ================================================================
// Function : ExtractElements
// ================================================================
bool CaeMeshTools::ExtractElements(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                   const RWCae::MeshPropertyComponent&     theProp,
                                   InteractiveCaeMeshElementType             theType,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return false;
    }

    theGenGeom->SetMeshData(new RWCae::MeshData());

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        const Handle(RWCae::MeshData)& aMeshData = aGeom->GetMeshData();
        if (aMeshData.IsNull()) {
            continue;
        }

        auto perform = [&aGeom, &theProp, &theGenGeom](const auto& theAdaptor) {
            extractElements(aGeom, theProp, theAdaptor, theGenGeom);
        };

        switch (theType) {
            case InteractiveCaeMeshElementType::Element0d: perform(RWCae::MeshAdaptor0d(*aMeshData)); break;
            case InteractiveCaeMeshElementType::Face:
                RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData);
                break;
            case InteractiveCaeMeshElementType::Volume:
                RWCae::MeshAdaptorsIterator2dElems::For(perform, aMeshData, true);
                break;
            default: break;
        }
    }

    theGenGeom->GetMeshData()->SetToUpdateReconstructedSurface(false);

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theGenGeom->GetMeshData()->Properties()) {
        aProp->UpdateComponents(false);
    }

    return true;
}

// ================================================================
// Function : extractElements
// ================================================================
template<class Elem_T>
void CaeMeshTools::extractElements(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                   const RWCae::MeshPropertyComponent&     theProp,
                                   const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    RWCae::MeshEntityId aNbElems = theMesh.NbElements();
    if (aNbElems == 0) {
        return;
    }

    const Handle(RWCae::MeshData)& aResMeshData = theGenGeom->GetMeshData();

    const RWCae::MeshDataVector<Graphic3d_Vec3>& aNodes = theGeom->GetMeshData()->Nodes();

    Handle(RWCae::MeshScalarPropertyBase) aSrcComp;
    Handle(RWCae::MeshScalarPropertyBase) aDstComp;
    extractProperty(aResMeshData, theGeom, theProp, aSrcComp, aDstComp);

    auto addValues =
        [&aDstComp,
         &aSrcComp](int theDstIndex, int theDstSubIndex, int theSrcIndex, int theSrcSubIndex, bool theToAdd) {
            if (theToAdd) {
                aDstComp->SetRawValue(RWCae::MeshPropertyEntityInfo(theDstIndex, theDstSubIndex),
                                      aSrcComp->RawValue(RWCae::MeshPropertyEntityInfo(theSrcIndex, theSrcSubIndex)),
                                      false);
            }
        };

    bool isNodal     = !aDstComp.IsNull() && aDstComp->IsNodal();
    bool isNodeVec   = !aDstComp.IsNull() && aDstComp->BaseType() == RWCae::MeshScalarPropertyBaseType::NodeVec;
    bool isElemental = !aDstComp.IsNull() && !isNodeVec && aDstComp->IsElemental();

    RWCae::MeshData::EntityMap aNodesMap;

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++) {
        const Elem_T& anElem = theMesh.GetElement(anElemIter);
        if (!theGeom->AcceptElement(anElem.GlobalIndex())) {
            continue;
        }

        Elem_T aNewElem(anElem.NbNodes());

        int aGlobalIndex = theMesh.IsSurface() ? aResMeshData->NbSurfElems() : aResMeshData->NbIndexedElems();
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < anElem.NbNodes(); aNodeIter++) {
            RWCae::MeshEntityId aNodeIdx = 0;
            if (!aNodesMap.Find(anElem[aNodeIter], aNodeIdx)) {
                Graphic3d_Vec3 aNode = aNodes[anElem[aNodeIter]];
                theGeom->ModifyNode(anElem[aNodeIter], aNode);
                aNodeIdx = aResMeshData->AddNodePosition(aNode);
                aNodesMap.Bind(anElem[aNodeIter], aNodeIdx);

                addValues(aNodeIdx, -1, anElem[aNodeIter], -1, isNodal);

                ViewCaeGeom::MeshInterpolatedNode aLerpNode;
                aLerpNode.AddBaseNode(anElem[aNodeIter], 1.0);
                theGenGeom->ChangeNodesMap().Bind(aNodeIdx, std::make_pair(theGeom.get(), aLerpNode));
            }
            aNewElem[aNodeIter] = aNodeIdx;

            addValues(aGlobalIndex, aNodeIdx, anElem.GlobalIndex(), anElem[aNodeIter], isNodeVec);
        }

        addValues(aGlobalIndex, -1, anElem.GlobalIndex(), -1, isElemental);

        addExtractedElem(theGeom, theGenGeom, aResMeshData, theMesh, anElemIter, aGlobalIndex, aNewElem);
    }
}

// ================================================================
// Function : addExtractedElem
// ================================================================
template<class Elem_T>
void CaeMeshTools::addExtractedElem(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                    const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom,
                                    const Handle(RWCae::MeshData)&          theMeshData,
                                    const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                    RWCae::MeshEntityId                     theElemIndex,
                                    int                                       theGlobalIndex,
                                    Elem_T&                                   theElem)
{
    ViewCaeGeom::MeshGeomElem aNewElemInfo;
    if (theMesh.IsSurface()) {
        theElem.SetGlobalIndex(theGlobalIndex);
        theMeshData->AddSurfaceElement(theElem);
        aNewElemInfo.Index     = theMeshData->NbSurfElems(theElem.Type) - 1;
        aNewElemInfo.Type      = theElem.Type;
        aNewElemInfo.IsSurface = true;
    } else {
        theMeshData->AddElement(theElem);
        aNewElemInfo.Index     = theMeshData->NbElems(theElem.Type) - 1;
        aNewElemInfo.Type      = theElem.Type;
        aNewElemInfo.IsSurface = false;
    }

    ViewCaeGeom::MeshGeomElem anOldElemInfo(theElemIndex, theMesh.GetElementType(), theMesh.IsSurface());

    theGenGeom->ChangeElementsMap().Bind(aNewElemInfo, std::make_pair(theGeom.get(), anOldElemInfo));
}

// ================================================================
// Function : ExtractPoints
// ================================================================
bool CaeMeshTools::ExtractPoints(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const RWCae::MeshPropertyComponent&     theProp,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    Handle(ViewCaeGeom::MeshGeometry) aMeshGeom = theGeom->GetLastNotModifierChild();
    if (aMeshGeom.IsNull()) {
        return false;
    }

    theGenGeom->SetMeshData(new RWCae::MeshData());

    for (ViewCaeGeom::MeshGeometry* aGeom : ViewCaeGeom::MeshGeomChildren(aMeshGeom.get())) {
        if (aGeom->GetMeshData().IsNull()) {
            continue;
        }

        extractPoints(aGeom, theProp, theGenGeom);
    }

    theGenGeom->GetMeshData()->SetToUpdateReconstructedSurface(false);

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theGenGeom->GetMeshData()->Properties()) {
        aProp->UpdateComponents(false);
    }

    return true;
}

// ================================================================
// Function : extractPoints
// ================================================================
void CaeMeshTools::extractPoints(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const RWCae::MeshPropertyComponent&     theProp,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom)
{
    const Handle(RWCae::MeshData)& aResMeshData = theGenGeom->GetMeshData();

    Handle(RWCae::MeshScalarPropertyBase) aSrcComp;
    Handle(RWCae::MeshScalarPropertyBase) aDstComp;
    extractProperty(aResMeshData, theGeom, theProp, aSrcComp, aDstComp);
    if (!aDstComp.IsNull() && !aDstComp->IsNodal()) {
        aDstComp.Nullify();
    }

    const Handle(RWCae::MeshData)&  aMeshData  = theGeom->GetMeshData();
    const TColStd_PackedMapOfInteger& aFreeNodes = aMeshData->FreeNodes();

    Graphic3d_Vec3         aNode;
    RWCae::MeshElement0d anElem;

    RWCae::MeshEntityId aNbNodes = aMeshData->NbNodes();
    for (RWCae::MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++) {
        aNode = aMeshData->Nodes()[aNodeIter];
        theGeom->ModifyNode(aNodeIter, aNode);
        if (!theGeom->AcceptNode(aNodeIter, aNode)) {
            continue;
        }
        RWCae::MeshEntityId aNodeIdx = aResMeshData->AddNodePosition(aNode);
        if (!aFreeNodes.Contains(aNodeIter)) {
            anElem[0] = aNodeIdx;
            aResMeshData->AddElement(anElem);
        }

        ViewCaeGeom::MeshInterpolatedNode aLerpNode;
        aLerpNode.AddBaseNode(aNodeIter, 1.0);
        theGenGeom->ChangeNodesMap().Bind(aNodeIdx, std::make_pair(theGeom.get(), aLerpNode));

        if (!aDstComp.IsNull()) {
            aDstComp->SetRawValue(RWCae::MeshPropertyEntityInfo(aNodeIdx),
                                  aSrcComp->RawValue(RWCae::MeshPropertyEntityInfo(aNodeIter)),
                                  false);
        }
    }
}

// ================================================================
// Function : extractProperty
// ================================================================
void CaeMeshTools::extractProperty(const Handle(RWCae::MeshData)&          theMeshData,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                   const RWCae::MeshPropertyComponent&     theProp,
                                   Handle(RWCae::MeshScalarPropertyBase)&  theSrcComp,
                                   Handle(RWCae::MeshScalarPropertyBase)&  theDstComp)
{
    Handle(RWCae::MeshNVecProperty) aSrcProp = theGeom->CalculateProperty(theProp.Property);
    if (aSrcProp.IsNull()) {
        return;
    }

    theSrcComp = aSrcProp->GetRawComponent(theProp.ComponentIndex);

    Handle(RWCae::MeshNVecProperty) aDstProp;

    if (theMeshData->Properties().IsEmpty()) {
        aDstProp = aSrcProp->CopyRaw(0, theProp.ComponentIndex);
        theMeshData->ChangeProperties().Append(aDstProp);
        aDstProp->SetId(theProp.Property->Id());
        aDstProp->SetRefFilePath("");
    } else {
        aDstProp = theMeshData->ChangeProperties()[0];
    }
    theDstComp = aDstProp->GetRawComponent(0);
    theDstComp->SetId(theProp.GetScalarComp()->Id());
    theDstComp->SetLoaded(true);
    theDstComp->SetValueRange(theProp.GetScalarComp()->ValueRange());
    theDstComp->SetRefFilePath("");
}

} // namespace
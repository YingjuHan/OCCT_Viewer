#include <RWCae/MeshData.h>

#include <RWBase/TaskManager.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <Graphic3d_Buffer.hxx>
#include <NCollection_IncAllocator.hxx>
#include <NCollection_Map.hxx>
#include <Standard_Atomic.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

namespace
{
    static volatile int g_meshQuantitiesCounter = 0;

    //! gp_XYZ -> Graphic3d_Vec3d conversion.
    static Graphic3d_Vec3d xyzToVec3(const gp_XYZ &theXYZ)
    {
        return Graphic3d_Vec3d(theXYZ.X(), theXYZ.Y(), theXYZ.Z());
    }

    //! Fill the map with indices of used nodes (used by 0d/2d/3d elements).
    template <class Elem_T>
    void addUsedElemsNodes(TColStd_PackedMapOfInteger *theUsedNodes, const RWCae::MeshAdaptor<Elem_T> &theAdaptor)
    {
        RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();
        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++)
        {
            const Elem_T &anElem = theAdaptor.GetElement(anElemIter);

            RWCae::MeshElemEntityId aNbNodes = anElem.NbNodes();
            for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++)
            {
                theUsedNodes->Add(anElem[aNodeIter]);
            }
        }
    }

    //! Common definition of mesh outer surface element.
    struct MeshOuterElem
    {
        std::vector<RWCae::MeshEntityId> Nodes;
        RWCae::MeshElemType Type;
        int GlobalIndex;
    };

    using MapOfSurfaceElements =
        NCollection_DataMap<std::vector<RWCae::MeshEntityId>, MeshOuterElem, RWCae::MeshElement2dHasher>;

    //! Add volume elements to the map.
    template <class Elem_T>
    void addVolumeElementsToMap(const RWCae::MeshAdaptor<Elem_T> &theMeshAdaptor, MapOfSurfaceElements &theMap)
    {
        RWCae::MeshEntityId aNbElems = theMeshAdaptor.NbElements();
        if (aNbElems == 0)
        {
            return;
        }

        std::vector<RWCae::MeshEntityId> aKey;
        MeshOuterElem aValue;
        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++)
        {
            const Elem_T &anElem = theMeshAdaptor.GetElement(anElemIter);

            RWCae::MeshElemEntityId aNbFaces = theMeshAdaptor.NbFaces(anElem);
            for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++)
            {
                theMeshAdaptor.FaceNodes(anElem, aFaceIter, aValue.Nodes, aValue.Type);
                aKey = aValue.Nodes;

                // use sorted indices for merging elements,
                // but keep original order for storing
                // to avoid inconsistent splitting of quad into triangles
                RWCae::SortedMeshElem2d::Sort(aKey);
                aValue.GlobalIndex = anElem.GlobalIndex();

                if (!theMap.Bind(aKey, aValue))
                {
                    theMap.UnBind(aKey);
                }
            }
        }
    }
}

namespace RWCae
{

    // ================================================================
    // Function : Constructor
    // ================================================================
    MeshData::MeshData(const TCollection_AsciiString &theId, AllocStrategy theAllocStrategy)
        : Poly_Triangulation(),
          myId(theId),
          myNodes(defaultIncrement(theAllocStrategy, Graphic3d_TOA_POS), defaultAllocator(theAllocStrategy)),
          myNormals(defaultIncrement(theAllocStrategy, Graphic3d_TOA_NORM), defaultAllocator(theAllocStrategy)),
          myNodesUV(defaultIncrement(theAllocStrategy, Graphic3d_TOA_UV), defaultAllocator(theAllocStrategy)),
          myNodalColors(defaultIncrement(theAllocStrategy, Graphic3d_TOA_COLOR), defaultAllocator(theAllocStrategy)),
          myIndexedElements(defaultIncrement(AllocStrategy_Resizable, -1), defaultAllocator(AllocStrategy_Resizable)),
          myProperties(m_defBlockSize4, defaultAllocator(theAllocStrategy)),
          myFemQuantitiesId(Standard_Atomic_Increment(&g_meshQuantitiesCounter))
    {
        ChangeElements(MeshElemType::Elem0d).Init<MeshElement0d>();
        ChangeElements(MeshElemType::Triangle).Init<MeshElementTriangle>();
        ChangeElements(MeshElemType::Triangle2nd).Init<MeshElementTriangle2nd>();
        ChangeElements(MeshElemType::Quad).Init<MeshElementQuad>();
        ChangeElements(MeshElemType::Quad2nd).Init<MeshElementQuad2nd>();
        ChangeElements(MeshElemType::Polygon).Init<MeshElementPolygon>();
        ChangeElements(MeshElemType::Tetrahedron).Init<MeshElementTetrahedron>();
        ChangeElements(MeshElemType::Tetrahedron2nd).Init<MeshElementTetrahedron2nd>();
        ChangeElements(MeshElemType::Pyramid).Init<MeshElementPyramid>();
        ChangeElements(MeshElemType::Pyramid2nd).Init<MeshElementPyramid2nd>();
        ChangeElements(MeshElemType::Prism).Init<MeshElementPrism>();
        ChangeElements(MeshElemType::Prism2nd).Init<MeshElementPrism2nd>();
        ChangeElements(MeshElemType::Hexahedron).Init<MeshElementHexahedron>();
        ChangeElements(MeshElemType::Hexahedron2nd).Init<MeshElementHexahedron2nd>();
        ChangeElements(MeshElemType::Polyhedron).Init<MeshElementPolyhedron>();
        ChangeSurfaceElements(MeshElemType::Triangle).Init<MeshElementTriangle>();
        ChangeSurfaceElements(MeshElemType::Triangle2nd).Init<MeshElementTriangle2nd>();
        ChangeSurfaceElements(MeshElemType::Quad).Init<MeshElementQuad>();
        ChangeSurfaceElements(MeshElemType::Quad2nd).Init<MeshElementQuad2nd>();
        ChangeSurfaceElements(MeshElemType::Polygon).Init<MeshElementPolygon>();
    }

    // ================================================================
    // Function : ~MeshData
    // ================================================================
    MeshData::~MeshData()
    {
        // Clear property dependency references
        // since they could be cyclic and leak memory.
        for (const Handle(MeshNVecProperty) &aProp : myProperties)
        {
            aProp->ChangeDependencyProps().Clear();
        }
    }

    // ================================================================
    // Function : Clear
    // ================================================================
    void MeshData::Clear()
    {
        myFreeNodes.Clear();
        myNodes.Clear();
        myNormals.Clear();
        myNodesUV.Clear();
        myNodalColors.Clear();
        myIndexedElements.Clear();
        for (int i = 0; i < MeshElemNbTypes; i++)
        {
            myElements[i].Clear();
        }
        for (int i = 0; i < MeshSurfElemNbTypes; i++)
        {
            mySurfElements[i].Clear();
        }
        myNodesToElemsOffsets.clear();
        myNodesToElems.clear();
        myProperties.Clear();
        unsetCachedMinMax();
    }

    // ================================================================
    // Function : MakeFace
    // ================================================================
    TopoDS_Face MeshData::MakeFace(const Handle(Poly_Triangulation) & theMesh)
    {
        TopoDS_Face aFace;
        BRep_Builder aBuilder;
        aBuilder.MakeFace(aFace, theMesh);
        theMesh->UpdateCachedMinMax();
        return aFace;
    }

    // ================================================================
    // Function : UpdateFace
    // ================================================================
    void MeshData::UpdateFace(const TopoDS_Face &theFace, const Handle(Poly_Triangulation) & theMesh)
    {
        BRep_Builder aBuilder;
        aBuilder.UpdateFace(theFace, theMesh);
        if (!theMesh.IsNull())
        {
            theMesh->UpdateCachedMinMax();
        }
    }

    // ================================================================
    // Function : InitFreeNodes
    // ================================================================
    void MeshData::InitFreeNodes()
    {
        if (!myToInitFreeNodes)
        {
            return;
        }

        myToInitFreeNodes = false;
        myFreeNodes.Clear();
        TColStd_PackedMapOfInteger aUsedNodes;

        auto compute = [&aUsedNodes](const auto &theAdaptor)
        {
            addUsedElemsNodes(&aUsedNodes, theAdaptor);
        };

        RWCae::MeshAdaptorsIteratorAllElems::For(compute, this);

        for (int aNodeIndexIter = myNodes.Lower(); aNodeIndexIter <= myNodes.Upper(); ++aNodeIndexIter)
        {
            if (!aUsedNodes.Contains(aNodeIndexIter))
            {
                myFreeNodes.Add(aNodeIndexIter);
            }
        }
    }

    // ================================================================
    // Function : computeBoundingBox
    // ================================================================
    Bnd_Box MeshData::computeBoundingBox(const gp_Trsf &theTrsf) const
    {
        Bnd_Box aBox;
        int aNodeLower = myNodes.Lower();
        int aNodeUpper = myNodes.Upper();
        if (theTrsf.Form() != gp_Identity)
        {
            for (int aNodeIter = aNodeLower; aNodeIter <= aNodeUpper; ++aNodeIter)
            {
                const Graphic3d_Vec3 &aVec3 = myNodes.Value(aNodeIter);
                gp_Pnt aNode(aVec3.x(), aVec3.y(), aVec3.z());
                aNode.Transform(theTrsf);
                aBox.Add(aNode);
            }
        }
        else
        {
            for (int aNodeIter = aNodeLower; aNodeIter <= aNodeUpper; ++aNodeIter)
            {
                const Graphic3d_Vec3 &aVec3 = myNodes.Value(aNodeIter);
                gp_Pnt aNode(aVec3.x(), aVec3.y(), aVec3.z());
                aBox.Add(aNode);
            }
        }
        return aBox;
    }

    // ================================================================
    // Function : AddBoundingBox
    // ================================================================
    void MeshData::AddBoundingBox(Bnd_Box &theBox, const TopoDS_Shape &theShape)
    {
        BRepBndLib::Add(theShape, theBox, true);
    }

    // ================================================================
    // Function : SurfaceElements
    // ================================================================
    const MeshElements &MeshData::SurfaceElements(MeshElemType theType) const
    {
        Standard_ASSERT_RAISE((int)theType >= MeshSurfElemLowerType && (int)theType <= MeshSurfElemUpperType,
                              "Error: Wrong elements type of mesh outer surface");

        int aType = (int)theType - MeshSurfElemLowerType;

        if (myAreSurfaceElementsUpdated)
        {
            return mySurfElements[aType];
        }

        myAreSurfaceElementsUpdated = true;

        for (int i = 0; i < MeshSurfElemNbTypes; i++)
        {
            mySurfElements[i].Clear();
        }

        Handle(NCollection_IncAllocator) anIncAlloc = new NCollection_IncAllocator();

        MapOfSurfaceElements anElemMap(1, anIncAlloc);

        auto compute = [&anElemMap](const auto &theAdaptor)
        {
            addVolumeElementsToMap(theAdaptor, anElemMap);
        };

        RWCae::MeshAdaptorsIterator3dElems::For(compute, this);

        for (MapOfSurfaceElements::Iterator anElemIter(anElemMap); anElemIter.More(); anElemIter.Next())
        {
            const std::vector<MeshEntityId> &aNodes = anElemIter.Value().Nodes;

            int aGlobalIndex = anElemIter.Value().GlobalIndex;

            auto add = [&thisSurfElements = mySurfElements](const auto &theElem)
            {
                thisSurfElements[(int)theElem.Type - MeshSurfElemLowerType].AddElement(theElem);
            };

            switch (anElemIter.Value().Type)
            {
            case MeshElemType::Triangle:
                add(MeshElementTriangle(aNodes.data(), aGlobalIndex));
                break;
            case MeshElemType::Triangle2nd:
                add(MeshElementTriangle2nd(aNodes.data(), aGlobalIndex));
                break;
            case MeshElemType::Quad:
                add(MeshElementQuad(aNodes.data(), aGlobalIndex));
                break;
            case MeshElemType::Quad2nd:
                add(MeshElementQuad2nd(aNodes.data(), aGlobalIndex));
                break;
            case MeshElemType::Polygon:
                add(MeshElementPolygon(aNodes.data(), MeshElemEntityId(aNodes.size()), aGlobalIndex));
                break;
            default:
                break;
            }
        }

        return mySurfElements[aType];
    }

    // ================================================================
    // Function : NodeAdjElems
    // ================================================================
    const MeshEntityId *MeshData::NodeAdjElems(MeshEntityId theNodeIdx, MeshElemEntityId &theNbAdjElems)
    {
        UpdateNodesAdjElems();

        theNbAdjElems = myNodesToElemsOffsets[theNodeIdx] - (theNodeIdx > 0 ? myNodesToElemsOffsets[theNodeIdx - 1] : 0);
        return &myNodesToElems[theNodeIdx > 0 ? myNodesToElemsOffsets[theNodeIdx - 1] : 0];
    }

    // ================================================================
    // Function : UpdateNodesAdjElems
    // ================================================================
    void MeshData::UpdateNodesAdjElems()
    {
        if (myNodesToElemsUpdated)
        {
            return;
        }

        MeshEntityId aNbNodes = NbNodes();
        MeshEntityId aNbElems = NbIndexedElems();

        int aNbThreads = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();

        static constexpr MeshEntityId aPartMinNbElems = 10000u;

        MeshEntityId aPartNbElems = std::max(aNbElems / aNbThreads, aPartMinNbElems);
        int aNbParts = Max(Min(aNbElems / aPartNbElems, aNbThreads), 1);

        std::vector<int> aNbAdded(aNbNodes, 0);

        auto processElems = [this, aNbElems, aPartNbElems, aNbParts](int thePartIdx, const auto &theFunc)
        {
            MeshEntityId anElemBegin = aPartNbElems * thePartIdx;
            MeshEntityId anElemEnd = thePartIdx == aNbParts - 1 ? aNbElems : anElemBegin + aPartNbElems;
            for (MeshEntityId anElemIter = anElemBegin; anElemIter < anElemEnd; anElemIter++)
            {
                const IndexedElement &anIndElem = myIndexedElements[anElemIter];

                const MeshElements &anElems = Elements(anIndElem.Type);

                MeshElemEntityOffset aNodesBegin = 0;
                MeshElemEntityOffset aNodesEnd = 0;
                anElems.GetElemNodesRange(anIndElem.Index, &aNodesBegin, &aNodesEnd);
                for (MeshElemEntityOffset aNodesIter = aNodesBegin; aNodesIter < aNodesEnd; aNodesIter++)
                {
                    theFunc(anElems.Nodes()[static_cast<int>(aNodesIter)], anElemIter);
                }
            }
        };

        auto computeSizes = [&processElems, &aNbAdded](int, int theIdx)
        {
            processElems(theIdx, [&aNbAdded](MeshEntityId theNodeIdx, MeshEntityId)
                         { Standard_Atomic_Increment(&aNbAdded[theNodeIdx]); });
        };

        RWBase::TaskManager::Instance()->RunInParallel(0, aNbParts, computeSizes);

        myNodesToElemsOffsets.resize(aNbNodes);

        MeshEntityId aPrevOffset = 0;
        for (MeshEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++)
        {
            myNodesToElemsOffsets[aNodeIter] = aPrevOffset + aNbAdded[aNodeIter];
            aPrevOffset = myNodesToElemsOffsets[aNodeIter];
        }

        myNodesToElems.resize(aPrevOffset);

        auto fillAdjElems =
            [&processElems, &aNodes = myNodesToElems, &anOffsets = myNodesToElemsOffsets, &aNbAdded](int, int theIdx)
        {
            processElems(theIdx, [&aNodes, &anOffsets, &aNbAdded](MeshEntityId theNodeIdx, MeshEntityId theElemIdx)
                         { aNodes[anOffsets[theNodeIdx] - Standard_Atomic_Decrement(&aNbAdded[theNodeIdx]) - 1] = theElemIdx; });
        };

        RWBase::TaskManager::Instance()->RunInParallel(0, aNbParts, fillAdjElems);

        myNodesToElemsUpdated = true;
    }

    // ================================================================
    // Function : HasProperty
    // ================================================================
    const Handle(MeshNVecProperty) & MeshData::HasProperty(const Handle(MeshNVecProperty) & theProp) const
    {
        if (theProp.IsNull())
        {
            return theProp;
        }

        for (const Handle(MeshNVecProperty) &aPropIter : myProperties)
        {
            if (aPropIter == theProp || (aPropIter->Id() == theProp->Id() && aPropIter->TimeStep() == theProp->TimeStep()))
            {
                return aPropIter;
            }
        }

        static const Handle(MeshNVecProperty) THE_NULL_MESHPROP;
        return THE_NULL_MESHPROP;
    }

    // ================================================================
    // Function : GetPropertyById
    // ================================================================
    const Handle(MeshNVecProperty) & MeshData::GetPropertyById(const TCollection_AsciiString &theId,
                                                               const int theTimeStep) const
    {
        for (const Handle(MeshNVecProperty) &aPropIter : myProperties)
        {
            if (aPropIter->Id() == theId && (theTimeStep < 0 || aPropIter->TimeStep() == theTimeStep))
            {
                return aPropIter;
            }
        }

        static const Handle(MeshNVecProperty) THE_NULL_MESHPROP;
        return THE_NULL_MESHPROP;
    }

    // ================================================================
    // Function : GetPropertyIdsByName
    // ================================================================
    NCollection_List<TCollection_AsciiString> MeshData::GetPropertyIdsByName(const TCollection_AsciiString &theName) const
    {
        NCollection_List<TCollection_AsciiString> aRes;

        for (const Handle(MeshNVecProperty) &aPropIter : myProperties)
        {
            if (aPropIter->Name() == theName)
            {
                aRes.Append(aPropIter->Id());
            }
        }

        return aRes;
    }

    // ================================================================
    // Function : NbProperties
    // ================================================================
    int MeshData::NbProperties(MeshPropertyBase::MappingType theType) const
    {
        int aRes = 0;

        for (const Handle(MeshNVecProperty) &aPropIter : myProperties)
        {
            if (aPropIter->MapType() == theType)
            {
                aRes++;
            }
        }

        return aRes;
    }

    // ================================================================
    // Function : ElementNodes
    // ================================================================
    void MeshData::ElementNodes(const MeshEntityId theElementIndex, std::vector<MeshEntityId> &theNodes) const
    {
        MeshElemType aType = myIndexedElements[theElementIndex].Type;
        MeshEntityId anInternalIndex = myIndexedElements[theElementIndex].Index;

        auto compute = [&theNodes, anInternalIndex](const auto &theAdaptor)
        {
            theAdaptor.ElementNodes(anInternalIndex, theNodes);
            return true;
        };

        RWCae::MeshAdaptorsIteratorAllElems::Switch(aType, compute, this);
    }

    //! Tool calculating new origin as center of bounding box round to integer number.
    class MeshData::CalcOriginTool
    {
    public:
        //! Compare error due to double->float conversion in two cases:
        //! 1) Point
        //! 2) Point - Center.
        bool Init(const NCollection_Sequence<gp_Pnt> &thePoints, const gp_Pnt &theCurrentOrigin)
        {
            for (const gp_Pnt &aPntIter : thePoints)
            {
                m_box.Add(aPntIter);
            }
            m_center = xyzToVec3((m_box.CornerMax().XYZ() + m_box.CornerMin().XYZ()) * m_half);

            Graphic3d_Vec3d aMaxLoss1;
            Graphic3d_Vec3d aMaxLoss2;
            for (const gp_Pnt &aPntIter : thePoints)
            {
                const Graphic3d_Vec3d aPntSrc1D = xyzToVec3(aPntIter.XYZ() - theCurrentOrigin.XYZ());
                const Graphic3d_Vec3d aPntSrc2D = aPntSrc1D - m_center;
                const Graphic3d_Vec3 aPntF1(aPntSrc1D);
                const Graphic3d_Vec3 aPntF2(aPntSrc2D);

                const Graphic3d_Vec3d aLoss1 = (aPntSrc1D - Graphic3d_Vec3d(aPntF1)).cwiseAbs();
                const Graphic3d_Vec3d aLoss2 = (aPntSrc2D - Graphic3d_Vec3d(aPntF2)).cwiseAbs();

                aMaxLoss1 = aMaxLoss1.cwiseMax(aLoss1);
                aMaxLoss2 = aMaxLoss2.cwiseMax(aLoss2);
            }
            if (aMaxLoss2.maxComp() <= gp::Resolution())
            {
                return false;
            }

            const double aLoss = aMaxLoss1.maxComp() / aMaxLoss2.maxComp();
            if (aLoss < m_ten10f)
            {
                return false;
            }
            return true;
        }

        //! Calculate new origin as center of bounding box round to integer number.
        void RoundNewOrigin()
        {
            RoundNewOriginComp<m_nc0>();
            RoundNewOriginComp<m_nc1>();
            RoundNewOriginComp<m_nc2>();
        }

        //! Calculate new origin as center of bounding box round to integer number.
        template <int theComp>
        void RoundNewOriginComp()
        {
            const double aComp = m_center[theComp];
            const int aRoundDir = Abs(aComp) < 1.0 ? 1 : -1;
            m_roundDir3[theComp] = aRoundDir;
            gp_Pnt aTmp(m_center.x(), m_center.y(), m_center.z());
            for (double aRound = 1.0;;)
            {
                aTmp.SetCoord(theComp + 1, Round(aComp * aRound) / aRound);
                if (aRoundDir == 1)
                {
                    if (!m_box.IsOut(aTmp))
                    {
                        m_roundLast[theComp] = aRound;
                        break;
                    }
                    aRound *= m_ten10f;
                }
                else
                {
                    if (m_box.IsOut(aTmp))
                    {
                        break;
                    }
                    m_roundLast[theComp] = aRound;
                    aRound *= m_invTen10f;
                }
            }
        }

        //! Increase rounding even if result goes outside of AABB to some extent.
        Graphic3d_Vec3d CalculateNewOrigin()
        {
            m_newOrigin = m_center;

            const double aDiagMax = xyzToVec3(m_box.CornerMax().XYZ() - m_box.CornerMin().XYZ()).maxComp();
            const double aMaxDev = aDiagMax * m_half;

            CalculateNewOriginComp<m_nc0>(aMaxDev);
            CalculateNewOriginComp<m_nc1>(aMaxDev);
            CalculateNewOriginComp<m_nc2>(aMaxDev);
            return m_newOrigin;
        }

        template <int theComp>
        void CalculateNewOriginComp(const double theMaxDev)
        {
            const double aCenterComp = m_center[theComp];
            for (int aCompOtherIter = 0; aCompOtherIter <= m_nc2; ++aCompOtherIter)
            {
                const double anOtherRound = m_roundLast[aCompOtherIter];
                bool toCheck = m_roundDir3[theComp] == m_roundDir3[aCompOtherIter] && (m_roundDir3[theComp] == 1 && m_roundLast[theComp] < anOtherRound) && (m_roundDir3[theComp] == -1 && m_roundLast[theComp] > anOtherRound);
                if (toCheck)
                {
                    const double aRoundComp = Round(aCenterComp * anOtherRound) / anOtherRound;
                    if (aRoundComp >= aCenterComp - theMaxDev && aRoundComp <= aCenterComp + theMaxDev)
                    {
                        m_roundLast[theComp] = anOtherRound;
                    }
                }
            }
            m_newOrigin[theComp] = m_roundLast[theComp] > 0.0
                                       ? Round(aCenterComp * m_roundLast[theComp]) / m_roundLast[theComp]
                                       : m_center[theComp];
        }

    private:
        static constexpr double m_ten10f = 10.0, m_invTen10f = 0.1, m_half = 0.5;
        static constexpr int m_nc0 = 0, m_nc1 = 1, m_nc2 = 2;

    private:
        Bnd_Box m_box;
        Graphic3d_Vec3d m_center;
        Graphic3d_Vec3d m_newOrigin;

        Graphic3d_Vec3d m_roundLast = Graphic3d_Vec3d(-1.0);
        Graphic3d_Vec3i m_roundDir3 = Graphic3d_Vec3i(0);
    };

    // ================================================================
    // Function : EstimateOrigin
    // ================================================================
    gp_XYZ MeshData::EstimateOrigin(const NCollection_Sequence<gp_Pnt> &thePoints, const gp_Pnt &theCurrentOrigin)
    {
        CalcOriginTool aTool;
        if (!aTool.Init(thePoints, theCurrentOrigin))
        {
            return gp_XYZ(0.0, 0.0, 0.0);
        }
        aTool.RoundNewOrigin();
        Graphic3d_Vec3d aNewOrigin = aTool.CalculateNewOrigin();
        return gp_XYZ(aNewOrigin.x(), aNewOrigin.y(), aNewOrigin.z());
    }

    // ================================================================
    // Function : PrintStats
    // ================================================================
    void MeshData::PrintStats(std::ostream &theStream) const
    {
        theStream << "[" << myNodes.Length() << " nodes]";
        printElemStats(theStream);
        printPropStats(theStream);
    }

    // ================================================================
    // Function : printElemStats
    // ================================================================
    void MeshData::printElemStats(std::ostream &theStream) const
    {
        auto printElemsNb = [&theStream](MeshEntityId theTotalNbElems,
                                         const char *theName,
                                         const std::vector<std::pair<MeshEntityId, const char *>> &theElems)
        {
            if (theTotalNbElems == 0)
            {
                return;
            }
            theStream << "[" << theTotalNbElems << " " << theName;
            if (theElems.empty())
            {
                theStream << "]";
                return;
            }

            theStream << " = ";

            bool toSep = false;
            for (const auto &anElem : theElems)
            {
                if (anElem.first == 0)
                {
                    continue;
                }
                if (toSep)
                {
                    theStream << " + ";
                }
                theStream << anElem.first << " " << anElem.second;
                toSep = true;
            }

            theStream << "]";
        };

        std::vector<std::pair<MeshEntityId, const char *>> anElems;

        printElemsNb(NbElems0d(), "0d elements", anElems);

        anElems.clear();
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Triangle), "triangles"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Triangle2nd), "2nd triangles"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Quad), "quads"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Quad2nd), "2nd quads"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Polygon), "polygons"));
        printElemsNb(NbElems2d(), "2d elements", anElems);

        anElems.clear();
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Tetrahedron), "tet."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Tetrahedron2nd), "2nd tet."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Pyramid), "pyr."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Pyramid2nd), "2nd pyr."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Prism), "prisms"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Prism2nd), "2nd prisms"));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Hexahedron), "hex."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Hexahedron2nd), "2nd hex."));
        anElems.push_back(std::make_pair(NbElems(MeshElemType::Polyhedron), "poly."));
        printElemsNb(NbElems3d(), "3d elements", anElems);
    }

    // ================================================================
    // Function : printPropStats
    // ================================================================
    void MeshData::printPropStats(std::ostream &theStream) const
    {
        const int aNbElemProps = NbProperties(MeshPropertyBase::MappingType::Elemental);
        const int aNbNodeProps = NbProperties(MeshPropertyBase::MappingType::Nodal);
        if (aNbElemProps != 0)
        {
            if (aNbNodeProps != 0)
            {
                theStream << "\n[" << aNbElemProps << " elem. + " << aNbNodeProps << " nodal props. = ";
            }
            else
            {
                theStream << "\n[" << aNbElemProps << " elem. props. = ";
            }
        }
        else if (aNbNodeProps != 0)
        {
            theStream << "\n[" << aNbNodeProps << " nodal props. = ";
        }

        int aNbProps[MeshNVecPropertyType_NB] = {};
        for (const Handle(MeshNVecProperty) &aPropIter : myProperties)
        {
            ++aNbProps[(int)aPropIter->Type()];
        }

        const int aNbAllProps = aNbElemProps + aNbNodeProps;
        if (aNbAllProps != 0)
        {
            theStream << aNbProps[(int)MeshNVecPropertyType::ScalarSet] << " scalar + "
                      << aNbProps[(int)MeshNVecPropertyType::Vector] << " vector + "
                      << aNbProps[(int)MeshNVecPropertyType::Tensor] << " tensor props.]";
        }
    }

    // ================================================================
    // Function : updateBlocks
    // ================================================================
    void MeshData::updateBlocks(bool theForced) const
    {
        if (!theForced && !myBlocks.IsNull())
        {
            return;
        }

        if (!myBlocks.IsNull())
        {
            if (!myBlocks->FillAndSortRanges())
            {
                myBlocks.Nullify();
            }
        }

        if (myBlocks.IsNull())
        {
            myBlocks = new MeshDataBlock("Root");
            myBlocks->Range().SetValues(0, myNodes.Size() - 1, 0, myIndexedElements.Size() - 1);
        }
    }

    // ================================================================
    // Function : ResetStaticCounter
    // ================================================================
    void MeshData::ResetStaticCounter()
    {
        g_meshQuantitiesCounter = 0;
    }

} // namespace
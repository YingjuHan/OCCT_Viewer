

#include <RWCae/MeshAdaptor.h>

#include <RWCae/MeshTools.h>

#include <array>

namespace
{
    //! Template container for faces nodes for mesh elements with fixed number of nodes.
    template <typename... ElemFace_T>
    struct ElemFaces
    {
        std::tuple<ElemFace_T...> Faces;

        //! Return number of faces.
        size_t NbFaces() const { return std::tuple_size<std::tuple<ElemFace_T...>>::value; }

        //! Declaration.
        template <typename Function, typename Indices = std::make_index_sequence<sizeof...(ElemFace_T)>>
        struct FuncTable
        {
        };

        //! Retrieve face by index and apply the specified function to it.
        template <typename Function>
        void Apply(RWCae::MeshElemEntityId theFaceIdx, Function theFunc) const
        {
            Standard_ASSERT_RAISE(theFaceIdx < std::tuple_size<std::tuple<ElemFace_T...>>::value,
                                  "Error: Invalid mesh element face index");
            FuncTable<Function>::Table[theFaceIdx](Faces, theFunc);
        }

        //! Auxiliary method that applies function to the specified tuple member.
        template <typename Function, size_t Index>
        static void ApplyForIndex(const std::tuple<ElemFace_T...> &theFaces, Function theFunc)
        {
            theFunc(std::get<Index>(theFaces));
        }

        //! Auxiliary struct holding array of functions pointers for each tuple index.
        template <typename Function, size_t... Indices>
        struct FuncTable<Function, std::index_sequence<Indices...>>
        {
            using FuncType = void (*)(const std::tuple<ElemFace_T...> &, Function);

            static constexpr FuncType Table[] = {&ApplyForIndex<Function, Indices>...};
        };
    };

    template <>
    template <typename Function>
    void ElemFaces<>::Apply(RWCae::MeshElemEntityId, Function) const
    {
    }

    //! Template container for face nodes for mesh elements with fixed number of nodes.
    template <typename Elem_T>
    struct ElemFace
    {
        static constexpr int m_n0 = 0, m_n1 = 1, m_n2 = 2, m_n4 = 4, m_n6 = 6;

        static constexpr RWCae::MeshElemType Type = Elem_T::Type;

        std::array<RWCae::MeshElemEntityId, Elem_T::NbFixedNodes> Nodes;

        //! Reorder face nodes as polyline and return specified node index.
        RWCae::MeshElemEntityId ContourNode(RWCae::MeshElemEntityId theNodeIdx) const { return Nodes[theNodeIdx]; }

        //! Split face into triangles (triangles fan by default) and fill specified triangle.
        void Triangle(Graphic3d_Vec3ub *theTrg, RWCae::MeshElemEntityId theTrgIdx) const
        {
            theTrg->SetValues(Nodes[m_n0], Nodes[theTrgIdx + m_n1], Nodes[theTrgIdx + m_n2]);
        }

        //! Return number of triangles in the face.
        RWCae::MeshElemEntityId NbTriangles() const
        {
            return static_cast<RWCae::MeshElemEntityId>(Nodes.size() - m_n2);
        }
    };

    // ================================================================
    // Function : ContourNode
    // ================================================================
    template <>
    RWCae::MeshElemEntityId ElemFace<RWCae::MeshElementTriangle2nd>::ContourNode(
        RWCae::MeshElemEntityId theNodeIdx) const
    {
        static constexpr RWCae::MeshElemEntityId anIndices[] = {0, 3, 1, 4, 2, 5};
        return Nodes[anIndices[theNodeIdx]];
    }

    // ================================================================
    // Function : Triangle
    // ================================================================
    template <>
    void ElemFace<RWCae::MeshElementTriangle2nd>::Triangle(Graphic3d_Vec3ub *theTrg,
                                                           RWCae::MeshElemEntityId theTrgIdx) const
    {
        static constexpr RWCae::MeshElemEntityId anIndices[][3] = {{5, 0, 3}, {3, 1, 4}, {4, 2, 5}, {5, 3, 4}};
        theTrg->SetValues(Nodes[anIndices[theTrgIdx][m_n0]],
                          Nodes[anIndices[theTrgIdx][m_n1]],
                          Nodes[anIndices[theTrgIdx][m_n2]]);
    }

    // ================================================================
    // Function : NbTriangles
    // ================================================================
    template <>
    RWCae::MeshElemEntityId ElemFace<RWCae::MeshElementTriangle2nd>::NbTriangles() const
    {
        return m_n4;
    }

    // ================================================================
    // Function : ContourNode
    // ================================================================
    template <>
    RWCae::MeshElemEntityId ElemFace<RWCae::MeshElementQuad2nd>::ContourNode(RWCae::MeshElemEntityId theNodeIdx) const
    {
        static constexpr RWCae::MeshElemEntityId anIndices[] = {0, 4, 1, 5, 2, 6, 3, 7};
        return Nodes[anIndices[theNodeIdx]];
    }

    // ================================================================
    // Function : Triangle
    // ================================================================
    template <>
    void ElemFace<RWCae::MeshElementQuad2nd>::Triangle(Graphic3d_Vec3ub *theTrg,
                                                       RWCae::MeshElemEntityId theTrgIdx) const
    {
        static constexpr RWCae::MeshElemEntityId anIndices[][3] =
            {{7, 0, 4}, {4, 1, 5}, {5, 2, 6}, {6, 3, 7}, {7, 4, 5}, {5, 6, 7}};
        theTrg->SetValues(Nodes[anIndices[theTrgIdx][m_n0]],
                          Nodes[anIndices[theTrgIdx][m_n1]],
                          Nodes[anIndices[theTrgIdx][m_n2]]);
    }

    // ================================================================
    // Function : NbTriangles
    // ================================================================
    template <>
    RWCae::MeshElemEntityId ElemFace<RWCae::MeshElementQuad2nd>::NbTriangles() const
    {
        return m_n6;
    }
}

namespace RWCae
{
    // ================================================================
    // Function : ElementCenter
    // ================================================================
    template <class Element_T>
    Graphic3d_Vec3 MeshAdaptor<Element_T>::ElementCenter(const Element_T &theElem) const
    {
        Graphic3d_Vec3 aRes;
        Graphic3d_Vec3 aNode;
        RWCae::MeshElemEntityId aNbNodes = theElem.NbNodes();
        for (RWCae::MeshElemEntityId i = 0; i < aNbNodes; i++)
        {
            NodePosition(aNode, theElem[i]);
            aRes += aNode;
        }
        aRes /= aNbNodes;
        return aRes;
    }

    // ================================================================
    // Function : NodePosition
    // ================================================================
    template <class Element_T>
    void MeshAdaptor<Element_T>::NodePosition(Graphic3d_Vec3 &theNode, const MeshEntityId theNodeIndex) const
    {
        theNode = myMesh->Nodes().Value(theNodeIndex);
    }

    // ================================================================
    // Function : NodePosition
    // ================================================================
    template <class Element_T>
    void MeshAdaptor<Element_T>::NodePosition(gp_Pnt &theNode, const MeshEntityId theNodeIndex) const
    {
        const Graphic3d_Vec3 &aNode = myMesh->Nodes().Value(theNodeIndex);
        theNode.SetCoord(aNode.x(), aNode.y(), aNode.z());
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <class Element_T>
    void MeshAdaptor<Element_T>::NbFaces(NCollection_DataMap<RWCae::MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const
    {
        MeshEntityId aNbElems = NbElements();

        std::apply(
            [&theNbFaces, aNbElems](const auto &...theFaces)
            {
                (
                    [&theNbFaces, aNbElems](const auto &theFace)
                    {
                        const RWCae::MeshElemEntityId aNbFaceNodes = static_cast<RWCae::MeshElemEntityId>(theFace.Nodes.size());
                        MeshElemEntityOffset *aNbFaces = theNbFaces.ChangeSeek(aNbFaceNodes);
                        if (aNbFaces == nullptr)
                        {
                            aNbFaces = theNbFaces.Bound(aNbFaceNodes, 0);
                        }
                        (*aNbFaces) += aNbElems;
                    }(theFaces),
                    ...);
            },
            ElementFaces().Faces);
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <class Element_T>
    RWCae::MeshElemEntityId MeshAdaptor<Element_T>::NbFaces(const Element_T &) const
    {
        return (RWCae::MeshElemEntityId)ElementFaces().NbFaces();
    }

    // ================================================================
    // Function : NbFaceNodes
    // ================================================================
    template <class Element_T>
    RWCae::MeshElemEntityId MeshAdaptor<Element_T>::NbFaceNodes(const Element_T &, RWCae::MeshElemEntityId theFaceIdx) const
    {
        RWCae::MeshElemEntityId aRes = 0;
        ElementFaces().Apply(theFaceIdx, [&aRes](const auto &theFace)
                             { aRes = static_cast<RWCae::MeshElemEntityId>(theFace.Nodes.size()); });
        return aRes;
    }

    // ================================================================
    // Function : FaceNode
    // ================================================================
    template <class Element_T>
    RWCae::MeshElemEntityId MeshAdaptor<Element_T>::FaceNodeIndex(const Element_T &,
                                                           RWCae::MeshElemEntityId theFaceIdx,
                                                           RWCae::MeshElemEntityId theNodeIdx) const
    {
        RWCae::MeshElemEntityId aRes = 0;
        ElementFaces().Apply(theFaceIdx, [theNodeIdx, &aRes](const auto &theFace)
                             { aRes = theFace.ContourNode(theNodeIdx); });
        return aRes;
    }

    // ================================================================
    // Function : nbTrgs
    // ================================================================
    template <class Element_T>
    RWCae::MeshElemEntityId MeshAdaptor<Element_T>::TrianglesIterator::nbTrgs() const
    {
        RWCae::MeshElemEntityId aRes = 0;
        myAdaptor->ElementFaces().Apply(myFaceIdx, [&aRes](const auto &theFace)
                                        { aRes = theFace.NbTriangles(); });
        return aRes;
    }

    // ================================================================
    // Function : fillTrg
    // ================================================================
    template <class Element_T>
    void MeshAdaptor<Element_T>::TrianglesIterator::fillTrg() const
    {
        myAdaptor->ElementFaces().Apply(myFaceIdx, [&thisTrgIdx = myTrgIdx, thisCurTrg = myCurTrg](const auto &theFace)
                                        { theFace.Triangle(&thisTrgIdx, thisCurTrg); });
    }

    // ================================================================
    // Function : NbTriangles
    // ================================================================
    template <class Element_T>
    uint64_t MeshAdaptor<Element_T>::NbTriangles(MeshEntityId theLower, MeshEntityId theUpper) const
    {
        RWCae::MeshElemEntityId aRes = 0;

        std::apply(
            [&aRes](const auto &...theFaces)
            {
                (
                    [&aRes](const auto &theFace)
                    {
                        aRes += theFace.NbTriangles();
                    }(theFaces),
                    ...);
            },
            ElementFaces().Faces);

        return aRes * (theUpper - theLower + 1);
    }

    // ================================================================
    // Function : FaceNodes
    // ================================================================
    template <class Element_T>
    void MeshAdaptor<Element_T>::FaceNodes(const Element_T &theElem,
                                           RWCae::MeshElemEntityId theFaceIdx,
                                           std::vector<MeshEntityId> &theNodes,
                                           MeshElemType &theType) const
    {
        ElementFaces().Apply(theFaceIdx, [&theElem, &theNodes, &theType](const auto &theFace)
                             {
        theNodes.resize(theFace.Nodes.size());
        for (size_t i = 0; i < theNodes.size(); i++) {
            theNodes[i] = theElem[theFace.Nodes[i]];
        }
        theType = theFace.Type; });
    }

    // ================================================================
    // Function : GetElement
    // ================================================================
    template <class Element_T>
    const Element_T &MeshAdaptor<Element_T>::GetElement(MeshEntityId theElemIndex) const
    {
        myElements->GetElementFixed(theElemIndex, myTmpElem);
        return myTmpElem;
    }

    //! @name MeshAdaptor<MeshElement0d> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElement0d>::ElementFaces() const
    {
        static constexpr ElemFaces<> EMPTY_FACES = {{}};

        return EMPTY_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementTriangle> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementTriangle>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle>> TRIANGLE_FACES = {{{0, 2, 1}}};

        return TRIANGLE_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementTriangle2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementTriangle2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle2nd>> QUAD_TRIANGLE_FACES = {{{0, 1, 2, 3, 4, 5}}};

        return QUAD_TRIANGLE_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementQuad> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementQuad>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad>> QUAD_FACES = {{{0, 1, 2, 3}}};

        return QUAD_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementQuadQuad2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementQuad2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad2nd>> QUAD_QUAD_FACES = {{{0, 1, 2, 3, 4, 5, 6, 7}}};

        return QUAD_QUAD_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPolygon> adaptor.
    //! @{

    // ================================================================
    // Function : ElementFaces
    // ================================================================
    template <>
    const auto &MeshAdaptor<MeshElementPolygon>::ElementFaces() const
    {
        throw Standard_NotImplemented("MeshAdaptor<MeshElementPolygon>::ElementFaces() shouldn't be used");

        static constexpr ElemFaces<> EMPTY_FACES = {{}};
        return EMPTY_FACES;
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolygon>::NbFaces(
        NCollection_DataMap<RWCae::MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const
    {
        MeshEntityId aNbElems = NbElements();
        for (MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++)
        {
            RWCae::MeshElemEntityId aNbFaceNodes = NbFaceNodes(anElemIter, 0);
            MeshElemEntityOffset *aNbFaces = theNbFaces.ChangeSeek(aNbFaceNodes);
            if (aNbFaces == nullptr)
            {
                aNbFaces = theNbFaces.Bound(aNbFaceNodes, 0);
            }
            (*aNbFaces)++;
        }
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolygon>::NbFaces(const MeshElementPolygon &) const
    {
        return m_n1;
    }

    // ================================================================
    // Function : NbFaceNodes
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolygon>::NbFaceNodes(const MeshElementPolygon &theElem, RWCae::MeshElemEntityId) const
    {
        return theElem.NbNodes();
    }

    // ================================================================
    // Function : FaceNode
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolygon>::FaceNodeIndex(const MeshElementPolygon &,
                                                                    RWCae::MeshElemEntityId,
                                                                    RWCae::MeshElemEntityId theNodeIdx) const
    {
        return theNodeIdx;
    }

    // ================================================================
    // Function : nbTrgs
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolygon>::TrianglesIterator::nbTrgs() const
    {
        return myElem->NbNodes() - m_n2;
    }

    // ================================================================
    // Function : fillTrg
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolygon>::TrianglesIterator::fillTrg() const
    {
        myTrgIdx.SetValues(m_n0, m_n1 + myCurTrg, m_n2 + myCurTrg);
    }

    // ================================================================
    // Function : NbTriangles
    // ================================================================
    template <>
    uint64_t MeshAdaptor<MeshElementPolygon>::NbTriangles(MeshEntityId theLower, MeshEntityId theUpper) const
    {
        uint64_t aRes = 0;

        for (MeshEntityId anElemIter = theLower; anElemIter <= theUpper; anElemIter++)
        {
            aRes += MeshAdaptor<MeshElementPolygon>::TrianglesIterator(*this, GetElement(anElemIter), 0).NbTrgs();
        }

        return aRes;
    }

    // ================================================================
    // Function : FaceNodes
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolygon>::FaceNodes(const MeshElementPolygon &theElem,
                                                    RWCae::MeshElemEntityId theFaceIdx,
                                                    std::vector<MeshEntityId> &theNodes,
                                                    MeshElemType &theType) const
    {
        RWCae::MeshElemEntityId aNbNodes = NbFaceNodes(theElem, theFaceIdx);
        theNodes.resize(aNbNodes);
        for (RWCae::MeshElemEntityId i = 0; i < aNbNodes; i++)
        {
            theNodes[i] = FaceNode(theElem, theFaceIdx, i);
        }
        theType = MeshElemType::Polygon;
    }

    // ================================================================
    // Function : GetElement
    // ================================================================
    template <>
    const MeshElementPolygon &MeshAdaptor<MeshElementPolygon>::GetElement(MeshEntityId theElemIndex) const
    {
        myElements->GetElementArb(theElemIndex, myTmpElem);
        return myTmpElem;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementTetrahedron> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementTetrahedron>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>>
            TETRAHEDRON_FACES = {{{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {0, 3, 2}}};

        return TETRAHEDRON_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementTetrahedron2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementTetrahedron2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>>
            QUAD_TETRAHEDRON_FACES = {{{0, 1, 3, 4, 8, 7}, {2, 0, 3, 6, 7, 9}, {1, 2, 3, 5, 9, 8}, {0, 2, 1, 6, 5, 4}}};

        return QUAD_TETRAHEDRON_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPyramid> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementPyramid>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementQuad>>
            PYRAMID_FACES = {{{0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4}, {3, 2, 1, 0}}};

        return PYRAMID_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPyramid2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementPyramid2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementQuad2nd>>
            QUAD_PYRAMID_FACES = {{{0, 1, 4, 5, 10, 9},
                                   {1, 2, 4, 6, 11, 10},
                                   {2, 3, 4, 7, 12, 11},
                                   {3, 0, 4, 8, 9, 12},
                                   {3, 2, 1, 0, 7, 6, 5, 8}}};

        return QUAD_PYRAMID_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPrism> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementPrism>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementTriangle>,
                                   ElemFace<MeshElementTriangle>>
            PRISM_FACES = {{{0, 1, 4, 3}, {1, 2, 5, 4}, {2, 0, 3, 5}, {0, 2, 1}, {3, 4, 5}}};

        return PRISM_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPrism2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementPrism2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementTriangle2nd>,
                                   ElemFace<MeshElementTriangle2nd>>
            QUAD_PRISM_FACES = {{{0, 1, 4, 3, 6, 13, 9, 12},
                                 {1, 2, 5, 4, 7, 14, 10, 13},
                                 {2, 0, 3, 5, 8, 12, 11, 14},
                                 {0, 2, 1, 8, 7, 6},
                                 {3, 4, 5, 9, 10, 11}}};

        return QUAD_PRISM_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementHexahedron> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementHexahedron>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>,
                                   ElemFace<MeshElementQuad>>
            HEXAHEDRON_FACES = {{{0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}, {3, 2, 1, 0}, {4, 5, 6, 7}}};

        return HEXAHEDRON_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementHexahedron2nd> adaptor.
    //! @{

    template <>
    const auto &MeshAdaptor<MeshElementHexahedron2nd>::ElementFaces() const
    {
        static constexpr ElemFaces<ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>,
                                   ElemFace<MeshElementQuad2nd>>
            QUAD_HEXAHEDRON_FACES = {{{0, 1, 5, 4, 8, 17, 12, 16},
                                      {1, 2, 6, 5, 9, 18, 13, 17},
                                      {2, 3, 7, 6, 10, 19, 14, 18},
                                      {3, 0, 4, 7, 11, 16, 15, 19},
                                      {3, 2, 1, 0, 10, 9, 8, 11},
                                      {4, 5, 6, 7, 12, 13, 14, 15}}};

        return QUAD_HEXAHEDRON_FACES;
    }

    //! @}

    //! @name MeshAdaptor<MeshElementPolyhedron> adaptor.
    //! @{

    // ================================================================
    // Function : ElementFaces
    // ================================================================
    template <>
    const auto &MeshAdaptor<MeshElementPolyhedron>::ElementFaces() const
    {
        throw Standard_NotImplemented("MeshAdaptor<MeshElementPolyhedron>::ElementFaces() shouldn't be used");

        static constexpr ElemFaces<> EMPTY_FACES = {{}};
        return EMPTY_FACES;
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolyhedron>::NbFaces(
        NCollection_DataMap<RWCae::MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const
    {
        MeshEntityId aNbElems = NbElements();
        for (MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++)
        {
            const MeshElementPolyhedron &anElem = GetElement(anElemIter);

            RWCae::MeshElemEntityId aNbElemFaces = anElem.NbFaces();
            for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbElemFaces; aFaceIter++)
            {
                RWCae::MeshElemEntityId aNbFaceNodes = anElem.NbFaceNodes(aFaceIter);
                MeshElemEntityOffset *aNbFaces = theNbFaces.ChangeSeek(aNbFaceNodes);
                if (aNbFaces == nullptr)
                {
                    aNbFaces = theNbFaces.Bound(aNbFaceNodes, 0);
                }
                (*aNbFaces)++;
            }
        }
    }

    // ================================================================
    // Function : NbFaces
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::NbFaces(const MeshElementPolyhedron &theElem) const
    {
        return theElem.NbFaces();
    }

    // ================================================================
    // Function : NbFaceNodes
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::NbFaceNodes(const MeshElementPolyhedron &theElem,
                                                                     RWCae::MeshElemEntityId theFaceIdx) const
    {
        return theElem.NbFaceNodes(theFaceIdx);
    }

    // ================================================================
    // Function : FaceNode
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::FaceNodeIndex(const MeshElementPolyhedron &theElem,
                                                                       RWCae::MeshElemEntityId theFaceIdx,
                                                                       RWCae::MeshElemEntityId theNodeIdx) const
    {
        return theElem.FaceNodeIndex(theFaceIdx, theNodeIdx);
    }

    // ================================================================
    // Function : nbTrgs
    // ================================================================
    template <>
    RWCae::MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::TrianglesIterator::nbTrgs() const
    {
        return myElem->NbFaceNodes(myFaceIdx) - m_n2;
    }

    // ================================================================
    // Function : fillTrg
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolyhedron>::TrianglesIterator::fillTrg() const
    {
        myTrgIdx.SetValues(myElem->FaceNodeIndex(myFaceIdx, m_n0),
                           myElem->FaceNodeIndex(myFaceIdx, m_n1 + myCurTrg),
                           myElem->FaceNodeIndex(myFaceIdx, m_n2 + myCurTrg));
    }

    // ================================================================
    // Function : NbTriangles
    // ================================================================
    template <>
    uint64_t MeshAdaptor<MeshElementPolyhedron>::NbTriangles(MeshEntityId theLower, MeshEntityId theUpper) const
    {
        uint64_t aRes = 0;

        for (MeshEntityId anElemIter = theLower; anElemIter <= theUpper; anElemIter++)
        {
            const MeshElementPolyhedron &anElem = GetElement(anElemIter);

            RWCae::MeshElemEntityId aNbElemFaces = anElem.NbFaces();
            for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbElemFaces; aFaceIter++)
            {
                aRes += MeshAdaptor<MeshElementPolyhedron>::TrianglesIterator(*this, anElem, aFaceIter).NbTrgs();
            }
        }

        return aRes;
    }

    // ================================================================
    // Function : FaceNodes
    // ================================================================
    template <>
    void MeshAdaptor<MeshElementPolyhedron>::FaceNodes(const MeshElementPolyhedron &theElem,
                                                       RWCae::MeshElemEntityId theFaceIdx,
                                                       std::vector<MeshEntityId> &theNodes,
                                                       MeshElemType &theType) const
    {
        theType = MeshElemType::Polygon;
        RWCae::MeshElemEntityId aNbNodes = NbFaceNodes(theElem, theFaceIdx);
        theNodes.resize(aNbNodes);
        for (RWCae::MeshElemEntityId i = 0; i < aNbNodes; i++)
        {
            theNodes[i] = FaceNode(theElem, theFaceIdx, i);
        }
    }

    // ================================================================
    // Function : GetElement
    // ================================================================
    template <>
    const MeshElementPolyhedron &MeshAdaptor<MeshElementPolyhedron>::GetElement(MeshEntityId theElemIndex) const
    {
        myElements->GetElementFaced(theElemIndex, myTmpElem);
        return myTmpElem;
    }

    //! @}

    // ================================================================
    // Function : ProbePoint
    // ================================================================
    template <class Element_T>
    bool MeshAdaptor<Element_T>::ProbePoint(const Element_T &theElem,
                                            const Graphic3d_Vec3 &thePnt,
                                            float *theWeights,
                                            const Handle(NCollection_BaseAllocator) & theAlloc) const
    {
        MeshTools::PointElementChecker<Element_T> aChecker(this, theElem, thePnt, theWeights, theAlloc);
        return aChecker.Perform();
    }

    template RWCae_ExportTemplate class MeshAdaptor<MeshElement0d>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementTriangle>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementTriangle2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementQuad>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementQuad2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPolygon>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementTetrahedron>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementTetrahedron2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPyramid>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPyramid2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPrism>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPrism2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementHexahedron>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementHexahedron2nd>;
    template RWCae_ExportTemplate class MeshAdaptor<MeshElementPolyhedron>;

} // RWCae

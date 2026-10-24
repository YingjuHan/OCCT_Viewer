#ifndef _MESHELEMENT_HEADERFILE
#define _MESHELEMENT_HEADERFILE

#include <NCollection_Vector.hxx>

#include <algorithm>
#include <vector>

namespace RWCae
{
    typedef uint32_t MeshEntityId;         //!< mesh node/element id
    typedef uint8_t MeshElemEntityId;      //!< mesh element node/face/triangle index
    typedef uint64_t MeshElemEntityOffset; //!< mesh element nodes/faces offsets

    //! Mesh data element type.
    enum class MeshElemType
    {
        Elem0d,         //!< 0d element (node)
        Triangle,       //!< 3-node triangle
        Triangle2nd,    //!< 6-node 2nd order triangle
        Quad,           //!< 4-node quad
        Quad2nd,        //!< 8-node 2nd order quad
        Polygon,        //!< n-node polygon
        Tetrahedron,    //!< 4-node tetrahedron
        Tetrahedron2nd, //!< 10-node 2nd order tetrahedron
        Pyramid,        //!< 5-node pyramid
        Pyramid2nd,     //!< 13-node 2nd order pyramid
        Prism,          //!< 6-node prism
        Prism2nd,       //!< 15-node 2nd order prism
        Hexahedron,     //!< 8-node hexahedron,
        Hexahedron2nd,  //!< 20-node 2nd order hexahedron,
        Polyhedron,     //!< n-node polyhedron
    };

    constexpr int MeshElemNbTypes = (int)MeshElemType::Polyhedron + 1;
    constexpr int MeshElem3dLowerType = (int)MeshElemType::Tetrahedron;
    constexpr int MeshElem3dUpperType = (int)MeshElemType::Polyhedron;
    constexpr int MeshSurfElemLowerType = (int)MeshElemType::Triangle;
    constexpr int MeshSurfElemUpperType = (int)MeshElemType::Polygon;
    constexpr int MeshSurfElemNbTypes = MeshSurfElemUpperType - MeshSurfElemLowerType + 1;

    //! Base class for mesh element.
    template <MeshElemType Elem_T>
    class MeshElement
    {
    public:
        //! Type of the element.
        static constexpr MeshElemType Type = Elem_T;

        //! Empty constructor.
        MeshElement() {}

        //! Constructor for mesh element with its own internal nodes array.
        explicit MeshElement(MeshElemEntityId theNbNodes)
            : myNodesRaw(theNbNodes > 0 ? new MeshEntityId[theNbNodes] : nullptr),
              myNbNodes(theNbNodes),
              myOwnNodes(theNbNodes > 0)
        {
        }

        //! Constructor for mesh element with pointer to external nodes array.
        MeshElement(const MeshEntityId *theNodes, MeshElemEntityId theNbNodes, int theGlobalIndex = -1)
            : myNodesRaw(theNodes),
              myNbNodes(theNbNodes),
              myGlobalIndex(theGlobalIndex)
        {
        }

        //! Destructor.
        ~MeshElement()
        {
            if (myOwnNodes)
            {
                delete[] myNodesRaw;
            }
            myNodesRaw = nullptr;
            myNodesVec = nullptr;
        }

        //! Copy constructor (nodes data will not be fully copied, just pointer to it).
        MeshElement(const MeshElement &theCopy)
        {
            myNbNodes = theCopy.myNbNodes;
            myNodesRaw = theCopy.myNodesRaw;
            myNodesVec = theCopy.myNodesVec;
            myNodesOffset = theCopy.myNodesOffset;
            myGlobalIndex = theCopy.myGlobalIndex;
        }

        //! Set nodes array with possibility to copy data.
        void SetNodes(const MeshEntityId *theNodes, MeshElemEntityId theNbNodes, bool theOwnNodes = true)
        {
            if (myOwnNodes)
            {
                delete[] myNodesRaw;
            }
            myNodesVec = nullptr;

            myNbNodes = theNbNodes;
            myOwnNodes = theOwnNodes;

            if (myOwnNodes)
            {
                myNodesRaw = new MeshEntityId[myNbNodes];
                std::copy(theNodes, theNodes + myNbNodes, const_cast<MeshEntityId *>(myNodesRaw));
            }
            else
            {
                myNodesRaw = theNodes;
            }
        }

        //! Set nodes array as NCollection_Vector and start/end indices (no data will be copied).
        void SetNodes(const NCollection_Vector<MeshEntityId> &theNodes,
                      MeshElemEntityOffset theNodesStart,
                      MeshElemEntityOffset theNodesEnd)
        {
            if (myOwnNodes)
            {
                delete[] myNodesRaw;
            }
            myNodesRaw = nullptr;

            myNodesOffset = theNodesStart;
            myNbNodes = (MeshElemEntityId)(theNodesEnd - theNodesStart);
            myOwnNodes = false;
            myNodesVec = &theNodes;
        }

        //! Return number of nodes.
        MeshElemEntityId NbNodes() const { return myNbNodes; }

        //! Access node within range [0, NbNodes()-1].
        MeshEntityId operator[](const MeshElemEntityId theIndex) const
        {
            return myNodesVec != nullptr ? (*myNodesVec)[static_cast<int>(myNodesOffset + theIndex)] : myNodesRaw[theIndex];
        }

        //! Access node within range [0, NbNodes()-1].
        MeshEntityId &operator[](const MeshElemEntityId theIndex)
        {
            return myNodesVec != nullptr ? (*const_cast<NCollection_Vector<MeshEntityId> *>(
                                               myNodesVec))[static_cast<int>(myNodesOffset + theIndex)]
                                         : const_cast<MeshEntityId *>(myNodesRaw)[theIndex];
        }

        //! Get the global index for CAE property link, -1 means the link not set
        int GlobalIndex() const { return myGlobalIndex; }

        //! Set the global index for CAE property link
        void SetGlobalIndex(const int theIndex) { myGlobalIndex = theIndex; }

    protected:
        const MeshEntityId *myNodesRaw = nullptr;
        const NCollection_Vector<MeshEntityId> *myNodesVec = nullptr;
        MeshElemEntityOffset myNodesOffset = 0;
        MeshElemEntityId myNbNodes = 0;
        int myGlobalIndex = -1;
        bool myOwnNodes = false;
    };

    //! Mesh element with fixed number of nodes.
    template <MeshElemEntityId N, MeshElemType Elem_T>
    class MeshElementFixed : public MeshElement<Elem_T>
    {
    public:
        using MeshElement<Elem_T>::SetNodes;

        //! Number of nodes in the element.
        static constexpr MeshElemEntityId NbFixedNodes = N;

        //! Constructor for mesh element with its own internal nodes array.
        MeshElementFixed() : MeshElement<Elem_T>(N) {}

        //! Constructor for mesh element with its own internal nodes array.
        explicit MeshElementFixed(MeshElemEntityId theNbNodes) : MeshElement<Elem_T>(theNbNodes)
        {
            Standard_ASSERT_RAISE(theNbNodes == N || theNbNodes == 0, "Error: Wrong number of nodes for fixed element");
        }

        //! Constructor for mesh element with pointer to external nodes array.
        explicit MeshElementFixed(const MeshEntityId theNodes[N], int theGlobalIndex = -1)
            : MeshElement<Elem_T>(theNodes, N, theGlobalIndex)
        {
        }

        //! Copy constructor (nodes data will not be fully copied, just pointer to it).
        MeshElementFixed(const MeshElementFixed &theCopy) : MeshElement<Elem_T>(theCopy) {}

        //! Set nodes array with possibility to copy data.
        void SetNodes(const MeshEntityId theNodes[N], bool theOwnNodes = true)
        {
            MeshElement<Elem_T>::SetNodes(theNodes, N, theOwnNodes);
        }

        //! Set nodes array with possibility to copy data.
        void SetNodes(const MeshEntityId *theNodes, MeshElemEntityId theNbNodes, bool theOwnNodes = true)
        {
            Standard_ASSERT_RAISE(theNbNodes == N, "Error: Wrong number of nodes for fixed element");
            MeshElement<Elem_T>::SetNodes(theNodes, N, theOwnNodes);
        }

        //! Set nodes array as NCollection_Vector and start index (no data will be copied).
        void SetNodes(const NCollection_Vector<MeshEntityId> &theNodes, MeshElemEntityOffset theNodesStart)
        {
            MeshElement<Elem_T>::SetNodes(theNodes, theNodesStart, theNodesStart + N);
        }
    };

    //! Mesh element with arbitrary number of nodes.
    template <MeshElemType Elem_T>
    class MeshElementArb : public MeshElement<Elem_T>
    {
    public:
        //! Empty constructor.
        MeshElementArb() : MeshElement<Elem_T>() {}

        //! Constructor for mesh element with its own internal nodes array.
        explicit MeshElementArb(MeshElemEntityId theNbNodes) : MeshElement<Elem_T>(theNbNodes) {}

        //! Constructor for mesh element with pointer to external nodes array.
        MeshElementArb(const MeshEntityId *theNodes, MeshElemEntityId theNbNodes, int theGlobalIndex = -1)
            : MeshElement<Elem_T>(theNodes, theNbNodes, theGlobalIndex)
        {
        }

        //! Copy constructor (nodes data will not be fully copied, just pointer to it).
        MeshElementArb(const MeshElementArb &theCopy) : MeshElement<Elem_T>(theCopy) {}
    };

    //! Mesh element with arbitrary number of nodes and faces.
    template <MeshElemType Elem_T>
    class MeshElementFaced : public MeshElementArb<Elem_T>
    {
    public:
        //! Empty constructor.
        MeshElementFaced() : MeshElementArb<Elem_T>() {}

        //! Constructor for mesh element with its own internal nodes array.
        explicit MeshElementFaced(MeshElemEntityId theNbNodes) : MeshElementArb<Elem_T>(theNbNodes) {}

        //! Constructor for mesh element with pointers to external nodes/faces arrays.
        MeshElementFaced(const MeshEntityId *theNodes,
                         int theNbNodes,
                         const MeshElemEntityId *theFaces,
                         uint16_t theFacesSize,
                         int theGlobalIndex = -1)
            : MeshElementArb<Elem_T>(theNodes, theNbNodes, theGlobalIndex),
              myFacesRaw(theFaces),
              myFacesSize(theFacesSize)
        {
        }

        //! Destructor.
        ~MeshElementFaced()
        {
            if (myOwnFaces)
            {
                delete[] myFacesRaw;
            }

            delete[] myFaceOffsets;
        }

        //! Copy constructor (nodes/faces data will not be fully copied, just pointers to it).
        MeshElementFaced(const MeshElementFaced &theCopy) : MeshElementArb<Elem_T>(theCopy)
        {
            myFacesSize = theCopy.myFacesSize;
            myFacesRaw = theCopy.myFacesRaw;
            myFacesVec = theCopy.myFacesVec;
            myFacesOffset = theCopy.myFacesOffset;
            myNbFaces = theCopy.myNbFaces;
        }

        //! Set faces array with possibility to copy data.
        //! Faces are stored as (k, a1...ak, m, b1...bm, ...), where
        //! k, m - number of nodes in face,
        //! ai, bj - index of face node inside nodes array
        void SetFaces(const MeshElemEntityId *theFaces, uint16_t theFacesSize, bool theOwnFaces)
        {
            if (myOwnFaces)
            {
                delete[] myFacesRaw;
            }
            myFacesVec = nullptr;

            myFacesSize = theFacesSize;
            myOwnFaces = theOwnFaces;

            if (myOwnFaces)
            {
                myFacesRaw = new MeshElemEntityId[myFacesSize];
                std::copy(theFaces, theFaces + myFacesSize, const_cast<MeshElemEntityId *>(myFacesRaw));
            }
            else
            {
                myFacesRaw = theFaces;
            }

            myFacesUpdated = false;
        }

        //! Set faces array as NCollection_Vector and start/end indices (no data will be copied).
        //! Faces are stored as (k, a1...ak, m, b1...bm, ...), where
        //! k, m - number of nodes in face,
        //! ai, bj - index of face node inside nodes array
        void SetFaces(const NCollection_Vector<MeshElemEntityId> &theFaces,
                      MeshElemEntityOffset theFaceStart,
                      MeshElemEntityOffset theFaceEnd)
        {
            if (myOwnFaces)
            {
                delete[] myFacesRaw;
            }
            myFacesRaw = nullptr;

            myFacesOffset = theFaceStart;
            myFacesSize = static_cast<uint16_t>(theFaceEnd - theFaceStart);
            myOwnFaces = false;
            myFacesVec = &theFaces;

            myFacesUpdated = false;
        }

        //! Return number of faces.
        MeshElemEntityId NbFaces() const
        {
            updateFaces();
            return myNbFaces;
        }

        //! Return number of face nodes.
        MeshElemEntityId NbFaceNodes(MeshElemEntityId theFaceIdx) const
        {
            updateFaces();
            return myFaceOffsets[theFaceIdx + 1] - myFaceOffsets[theFaceIdx] - 1;
        }

        //! Return face node by its index.
        MeshEntityId FaceNode(MeshElemEntityId theFaceIdx, MeshElemEntityId theNodeIdx) const
        {
            return (*this)[FaceNodeIndex(theFaceIdx, theNodeIdx)];
        }

        //! Return face node index (inside nodes array) by its index.
        MeshElemEntityId FaceNodeIndex(MeshElemEntityId theFaceIdx, MeshElemEntityId theNodeIdx) const
        {
            updateFaces();
            return myFacesVec != nullptr
                       ? (*myFacesVec)[static_cast<int>(myFacesOffset + myFaceOffsets[theFaceIdx] + theNodeIdx + 1)]
                       : myFacesRaw[myFaceOffsets[theFaceIdx] + theNodeIdx + 1];
        }

        //! Return raw value inside faces array by index.
        MeshElemEntityId FacesData(uint16_t theIdx) const
        {
            return myFacesVec != nullptr ? (*myFacesVec)[static_cast<int>(myFacesOffset + theIdx)] : myFacesRaw[theIdx];
        }

        //! Return size of faces array.
        uint16_t FacesDataSize() const { return myFacesSize; }

    protected:
        //! Build faces from raw array.
        void updateFaces() const
        {
            if (myFacesUpdated)
            {
                return;
            }

            if (myFaceOffsets != nullptr)
            {
                delete[] myFaceOffsets;
            }

            myNbFaces = 0;

            uint16_t anOffset = 0;
            if (myFacesVec != nullptr)
            {
                for (; anOffset < myFacesSize; anOffset += (*myFacesVec)[static_cast<int>(myFacesOffset + anOffset)] + 1)
                {
                    myNbFaces++;
                }
            }
            else
            {
                for (; anOffset < myFacesSize; anOffset += myFacesRaw[anOffset] + 1)
                {
                    myNbFaces++;
                }
            }

            myFaceOffsets = new uint16_t[myNbFaces + 1];

            anOffset = 0;
            if (myFacesVec != nullptr)
            {
                for (MeshElemEntityId aFace = 0; aFace < myNbFaces; aFace++)
                {
                    myFaceOffsets[aFace] = anOffset;
                    anOffset += (*myFacesVec)[static_cast<int>(myFacesOffset + anOffset)] + 1;
                }
            }
            else
            {
                for (MeshElemEntityId aFace = 0; aFace < myNbFaces; aFace++)
                {
                    myFaceOffsets[aFace] = anOffset;
                    anOffset += myFacesRaw[anOffset] + 1;
                }
            }
            myFaceOffsets[myNbFaces] = anOffset;

            myFacesUpdated = true;
        }

    protected:
        mutable uint16_t *myFaceOffsets = nullptr;
        const MeshElemEntityId *myFacesRaw = nullptr;
        const NCollection_Vector<MeshElemEntityId> *myFacesVec = nullptr;
        MeshElemEntityOffset myFacesOffset = 0;
        uint16_t myFacesSize = 0;
        mutable MeshElemEntityId myNbFaces = 0;
        mutable bool myFacesUpdated = false;
        bool myOwnFaces = false;
    };

    constexpr uint8_t NB_NODES_0D = 1;
    constexpr uint8_t NB_NODES_TRIANGLE = 3;
    constexpr uint8_t NB_NODES_TRIANGLE_2ND = 6;
    constexpr uint8_t NB_NODES_QUADRANGLE = 4;
    constexpr uint8_t NB_NODES_QUADRANGLE_2ND = 8;
    constexpr uint8_t NB_NODES_TETRAHEDRON = 4;
    constexpr uint8_t NB_NODES_TETRAHEDRON_2ND = 10;
    constexpr uint8_t NB_NODES_PYRAMID = 5;
    constexpr uint8_t NB_NODES_PYRAMID_2ND = 13;
    constexpr uint8_t NB_NODES_PRISM = 6;
    constexpr uint8_t NB_NODES_PRISM_2ND = 15;
    constexpr uint8_t NB_NODES_HEXAHEDRON = 8;
    constexpr uint8_t NB_NODES_HEXAHEDRON_2ND = 20;

    //! 0d element is defined by 1 nodes.
    typedef MeshElementFixed<NB_NODES_0D, MeshElemType::Elem0d> MeshElement0d;

    //! Triangle is defined by 3 nodes.
    typedef MeshElementFixed<NB_NODES_TRIANGLE, MeshElemType::Triangle> MeshElementTriangle;

    //! 2nd order triangle is defined by 6 nodes.
    typedef MeshElementFixed<NB_NODES_TRIANGLE_2ND, MeshElemType::Triangle2nd> MeshElementTriangle2nd;

    //! Quad is defined by 4 nodes.
    typedef MeshElementFixed<NB_NODES_QUADRANGLE, MeshElemType::Quad> MeshElementQuad;

    //! 2nd order quad is defined by 8 nodes.
    typedef MeshElementFixed<NB_NODES_QUADRANGLE_2ND, MeshElemType::Quad2nd> MeshElementQuad2nd;

    //! Polygon is defined by n nodes.
    typedef MeshElementArb<MeshElemType::Polygon> MeshElementPolygon;

    //! Tetrahedron is defined by 4 nodes.
    typedef MeshElementFixed<NB_NODES_TETRAHEDRON, MeshElemType::Tetrahedron> MeshElementTetrahedron;

    //! 2nd order tetrahedron is defined by 10 nodes.
    typedef MeshElementFixed<NB_NODES_TETRAHEDRON_2ND, MeshElemType::Tetrahedron2nd> MeshElementTetrahedron2nd;

    //! Pyramid is defined by 5 nodes.
    typedef MeshElementFixed<NB_NODES_PYRAMID, MeshElemType::Pyramid> MeshElementPyramid;

    //! 2nd order pyramid is defined by 13 nodes.
    typedef MeshElementFixed<NB_NODES_PYRAMID_2ND, MeshElemType::Pyramid2nd> MeshElementPyramid2nd;

    //! Prism is defined by 6 nodes.
    typedef MeshElementFixed<NB_NODES_PRISM, MeshElemType::Prism> MeshElementPrism;

    //! 2nd order prism is defined by 15 nodes.
    typedef MeshElementFixed<NB_NODES_PRISM_2ND, MeshElemType::Prism2nd> MeshElementPrism2nd;

    //! Hexahedron is defined by 8 nodes.
    typedef MeshElementFixed<NB_NODES_HEXAHEDRON, MeshElemType::Hexahedron> MeshElementHexahedron;

    //! 2nd order hexahedron is defined by 20 nodes.
    typedef MeshElementFixed<NB_NODES_HEXAHEDRON_2ND, MeshElemType::Hexahedron2nd> MeshElementHexahedron2nd;

    //! Polyhedron is defined by n nodes, m faces.
    typedef MeshElementFaced<MeshElemType::Polyhedron> MeshElementPolyhedron;

    //! @internal Sorting tool.
    struct SortedMeshElem2d
    {
        //! Sort elements of vector.
        static void Sort(std::vector<MeshEntityId> &theNodes) { std::sort(theNodes.begin(), theNodes.end()); }
    };

    //! @internal Hashing function for putting 2d mesh elements into data map.
    struct MeshElement2dHasher
    {
        //! Hash value, for Map interface.
        static int HashCode(const std::vector<MeshEntityId> &theElem, const int theUpper)
        {
            uint32_t aHashCode = 0;
            for (size_t i = 0; i < theElem.size(); i++)
            {
                aHashCode += theElem[i];
            }
            aHashCode = ::HashCode(aHashCode, theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! Matching two instances, for Map interface.
        static bool IsEqual(const std::vector<MeshEntityId> &theElem1, const std::vector<MeshEntityId> &theElem2)
        {
            return theElem1 == theElem2;
        }
    };

} // namespace

#endif // _MESHELEMENT_HEADERFILE
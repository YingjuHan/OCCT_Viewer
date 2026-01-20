#ifndef _MESHADAPTOR_HEADERFILE
#define _MESHADAPTOR_HEADERFILE

#include <RWCae/MeshData.h>
#include <RWCae/MeshDataRange.h>

#include <Graphic3d_ClipPlane.hxx>

namespace RWCae
{
    //! Auxiliary adaptor to access separate elements of specified type of @link RWCae::MeshData @endlink.
    //! - Most of element-related methods have two versions that accept element's index or element itself.
    //! - In the first case element is constructed and retrieved from @link MeshElements @endlink container.
    //! - So it is recommended to work with element itself (obtained with GetElement() method)
    //! in case of multiple methods calling.
    template <class Element_T>
    class MeshAdaptor
    {
        // element indices
        static constexpr int m_n0 = 0, m_n1 = 1, m_n2 = 2;

    public:
        //! Main constructor.
        explicit MeshAdaptor(const MeshData &theMesh, bool theIsSurface = false)
            : myMesh(&theMesh),
              myElements(theIsSurface ? &myMesh->SurfaceElements(Element_T::Type) : &myMesh->Elements(Element_T::Type)),
              myTmpElem((MeshElemEntityId)0),
              myIsSurface(theIsSurface)
        {
        }

        //! Empty constructor.
        MeshAdaptor() : myMesh(NULL), myElements(NULL), myTmpElem((MeshElemEntityId)0)
        {
            //
        }

        //! Copy constructor - should be called explicitly!
        explicit MeshAdaptor(const MeshAdaptor &theOther)
            : myMesh(theOther.myMesh),
              myElements(theOther.myElements),
              myTmpElem((MeshElemEntityId)0),
              myIsSurface(theOther.myIsSurface)
        {
            //
        }

        //! Assignment operator.
        MeshAdaptor &operator=(const MeshAdaptor &theOther)
        {
            if (&theOther != this)
            {
                myMesh = theOther.myMesh;
                myElements = theOther.myElements;
                myIsSurface = theOther.myIsSurface;
            }
            return *this;
        }

    public:
        //! Return type of iterated elements.
        static MeshElemType GetElementType() { return Element_T::Type; }

        //! Return elements container.
        const MeshElements &GetElements() const { return *myElements; }

        //! Check if adaptor is created for outer surface elements.
        bool IsSurface() const { return myIsSurface; }

        //! Return corresponding mesh data.
        const MeshData *GetMeshData() const { return myMesh; }

    public:
        //! Return faces nodes container for elements with fixed number of nodes.
        Standard_EXPORT const auto &ElementFaces() const;

        //! Return number of mesh elements.
        MeshEntityId NbElements() const { return myElements->NbElements(); }

        //! Return number of faces for each face size.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT void NbFaces(NCollection_DataMap<MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const;

        //! Return number of faces for specified element.
        MeshElemEntityId NbFaces(MeshEntityId theElemIdx) const { return NbFaces(GetElement(theElemIdx)); }

        //! Return number of faces for specified element.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT MeshElemEntityId NbFaces(const Element_T &theElem) const;

        //! Return number of face nodes for specified element.
        MeshElemEntityId NbFaceNodes(MeshEntityId theElemIdx, MeshElemEntityId theFaceIdx) const
        {
            return NbFaceNodes(GetElement(theElemIdx), theFaceIdx);
        }

        //! Return number of face nodes for specified element.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT MeshElemEntityId NbFaceNodes(const Element_T &theElem, MeshElemEntityId theFaceIdx) const;

        //! Return face node for specified element.
        MeshEntityId FaceNode(MeshEntityId theElemIdx, MeshElemEntityId theFaceIdx, MeshElemEntityId theNodeIdx) const
        {
            return FaceNode(GetElement(theElemIdx), theFaceIdx, theNodeIdx);
        }

        //! Return face node for specified element.
        MeshEntityId FaceNode(const Element_T &theElem, MeshElemEntityId theFaceIdx, MeshElemEntityId theNodeIdx) const
        {
            return theElem[FaceNodeIndex(theElem, theFaceIdx, theNodeIdx)];
        }

        //! Return face node index for specified element.
        MeshElemEntityId FaceNodeIndex(MeshEntityId theElemIdx,
                                       MeshElemEntityId theFaceIdx,
                                       MeshElemEntityId theNodeIdx) const
        {
            return FaceNodeIndex(GetElement(theElemIdx), theFaceIdx, theNodeIdx);
        }

        //! Return face node index for specified element.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT MeshElemEntityId FaceNodeIndex(const Element_T &theElem,
                                                       MeshElemEntityId theFaceIdx,
                                                       MeshElemEntityId theNodeIdx) const;

        //! Fetch face nodes for specified element.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT void FaceNodes(const Element_T &theElem,
                                       MeshElemEntityId theFaceIdx,
                                       std::vector<MeshEntityId> &theNodes,
                                       MeshElemType &theType) const;

        //! Fetch nodes for specified element.
        void ElementNodes(MeshEntityId theElemIdx, std::vector<MeshEntityId> &theNodes) const
        {
            ElementNodes(GetElement(theElemIdx), theNodes);
        }

        //! Fetch nodes for specified element.
        void ElementNodes(const Element_T &theElem, std::vector<MeshEntityId> &theNodes) const
        {
            MeshElemEntityId aNbNodes = theElem.NbNodes();
            theNodes.resize(aNbNodes);
            for (MeshElemEntityId i = 0; i < aNbNodes; i++)
            {
                theNodes[i] = theElem[i];
            }
        }

        //! Return total number of triangles.
        uint64_t NbTriangles() const { return NbTriangles(0, NbElements() - 1); }

        //! Return total number of triangles for elements range.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT uint64_t NbTriangles(MeshEntityId theLower, MeshEntityId theUpper) const;

        //! Return mesh element.
        //! Implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        Standard_EXPORT const Element_T &GetElement(MeshEntityId theElemIndex) const;

        //! Calculate center of the given element.
        Graphic3d_Vec3 ElementCenter(const MeshEntityId theElemIndex) const
        {
            return ElementCenter(GetElement(theElemIndex));
        }

        //! Calculate center of the given element.
        Standard_EXPORT Graphic3d_Vec3 ElementCenter(const Element_T &theElem) const;

        //! Iterator for triangles of element's face.
        //! Methods are implemented by default for elements with fixed number of nodes.
        //! Should be re-defined for elements of other types.
        class TrianglesIterator
        {
        public:
            //! Constructor.
            TrianglesIterator(const MeshAdaptor<Element_T> &theAdaptor,
                              const Element_T &theElem,
                              MeshElemEntityId theFaceIdx)
                : myAdaptor(&theAdaptor),
                  myElem(&theElem),
                  myFaceIdx(theFaceIdx)
            {
                myNbTrgs = nbTrgs();
                if (More())
                {
                    fillTrg();
                    myTrg.SetValues((*myElem)[myTrgIdx.x()], (*myElem)[myTrgIdx.y()], (*myElem)[myTrgIdx.z()]);
                }
            }

            //! Check if there are more triangles.
            bool More() const { return myCurTrg < myNbTrgs; }

            //! Fetch the next triangle.
            void Next()
            {
                myCurTrg++;
                if (More())
                {
                    fillTrg();
                    myTrg.SetValues((*myElem)[myTrgIdx.x()], (*myElem)[myTrgIdx.y()], (*myElem)[myTrgIdx.z()]);
                }
            }

            //! Return index of the current triangle.
            MeshElemEntityId Index() const { return myCurTrg; }

            //! Return triangle's nodes.
            const Graphic3d_Vec3u &Triangle() const { return myTrg; }

            //! Return triangle's nodes (as indices inside element's nodes array).
            const Graphic3d_Vec3ub &TriangleIndices() const { return myTrgIdx; }

            //! Return number of triangles.
            MeshElemEntityId NbTrgs() const { return myNbTrgs; }

        protected:
            //! Return number of triangles.
            Standard_EXPORT MeshElemEntityId nbTrgs() const;

            //! Fetch triangle.
            Standard_EXPORT void fillTrg() const;

        protected:
            const MeshAdaptor<Element_T> *myAdaptor;
            const Element_T *myElem;
            mutable Graphic3d_Vec3ub myTrgIdx;
            mutable Graphic3d_Vec3u myTrg;
            MeshElemEntityId myFaceIdx = 0;
            MeshElemEntityId myNbTrgs = 0;
            MeshElemEntityId myCurTrg = 0;
        };

        //! Check if point is inside the element and calculate nodes weights.
        //! For now implemented only for 3d elements.
        Standard_EXPORT bool ProbePoint(const Element_T &theElem,
                                        const Graphic3d_Vec3 &thePnt,
                                        float *theWeights,
                                        const Handle(NCollection_BaseAllocator) &theAlloc = nullptr) const;

    public:
        //! Return number of mesh nodes.
        MeshEntityId NbNodes() const { return myMesh->NbNodes(); }

        //! Fetch node position.
        Standard_EXPORT void NodePosition(Graphic3d_Vec3 &theNode, const MeshEntityId theNodeIndex) const;

        //! Fetch node position.
        Standard_EXPORT void NodePosition(gp_Pnt &theNode, const MeshEntityId theNodeIndex) const;

    protected:
        const MeshData *myMesh;
        const MeshElements *myElements;
        mutable Element_T myTmpElem;
        bool myIsSurface = false;
    };

    //! @name MeshAdaptor<MeshElement0d> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElement0d>::ElementFaces() const;

    typedef MeshAdaptor<MeshElement0d> MeshAdaptor0d;

    //! @}

    //! @name MeshAdaptor<MeshElementTriangle> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementTriangle>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementTriangle> MeshAdaptorTriangle;

    //! @}

    //! @name MeshAdaptor<MeshElementTriangle2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementTriangle2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementTriangle2nd> MeshAdaptorTriangle2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementQuad> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementQuad>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementQuad> MeshAdaptorQuad;

    //! @}

    //! @name MeshAdaptor<MeshElementQuad2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementQuad2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementQuad2nd> MeshAdaptorQuad2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementPolygon> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPolygon>::ElementFaces() const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolygon>::NbFaces(
        NCollection_DataMap<MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolygon>::NbFaces(const MeshElementPolygon &theElem) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolygon>::NbFaceNodes(const MeshElementPolygon &theElem,
                                                                                  MeshElemEntityId theFaceIdx) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolygon>::FaceNodeIndex(const MeshElementPolygon &theElem,
                                                                                    MeshElemEntityId theFaceIdx,
                                                                                    MeshElemEntityId theNodeIdx) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolygon>::TrianglesIterator::nbTrgs() const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolygon>::TrianglesIterator::fillTrg() const;

    template <>
    Standard_EXPORT uint64_t MeshAdaptor<MeshElementPolygon>::NbTriangles(MeshEntityId theLower,
                                                                          MeshEntityId theUpper) const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolygon>::FaceNodes(const MeshElementPolygon &theElem,
                                                                    MeshElemEntityId theFaceIdx,
                                                                    std::vector<MeshEntityId> &theNodes,
                                                                    MeshElemType &theType) const;

    template <>
    Standard_EXPORT const MeshElementPolygon &MeshAdaptor<MeshElementPolygon>::GetElement(MeshEntityId theElemIndex) const;

    typedef MeshAdaptor<MeshElementPolygon> MeshAdaptorPolygon;

    //! @}

    //! @name MeshAdaptor<MeshElementTetrahedron> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementTetrahedron>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementTetrahedron> MeshAdaptorTetrahedron;

    //! @}

    //! @name MeshAdaptor<MeshElementTetrahedron2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementTetrahedron2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementTetrahedron2nd> MeshAdaptorTetrahedron2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementPyramid> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPyramid>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementPyramid> MeshAdaptorPyramid;

    //! @}

    //! @name MeshAdaptor<MeshElementPyramid2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPyramid2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementPyramid2nd> MeshAdaptorPyramid2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementPrism> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPrism>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementPrism> MeshAdaptorPrism;

    //! @}

    //! @name MeshAdaptor<MeshElementPrism2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPrism2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementPrism2nd> MeshAdaptorPrism2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementHexahedron> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementHexahedron>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementHexahedron> MeshAdaptorHexahedron;

    //! @}

    //! @name MeshAdaptor<MeshElementHexahedron2nd> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementHexahedron2nd>::ElementFaces() const;

    typedef MeshAdaptor<MeshElementHexahedron2nd> MeshAdaptorHexahedron2nd;

    //! @}

    //! @name MeshAdaptor<MeshElementPolyhedron> adaptor.
    //! @{

    template <>
    Standard_EXPORT const auto &MeshAdaptor<MeshElementPolyhedron>::ElementFaces() const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolyhedron>::NbFaces(
        NCollection_DataMap<MeshElemEntityId, MeshElemEntityOffset> &theNbFaces) const;

    template <>
    Standard_EXPORT MeshElemEntityId
    MeshAdaptor<MeshElementPolyhedron>::NbFaces(const MeshElementPolyhedron &theElem) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::NbFaceNodes(const MeshElementPolyhedron &theElem,
                                                                                     MeshElemEntityId theFaceIdx) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::FaceNodeIndex(const MeshElementPolyhedron &theElem,
                                                                                       MeshElemEntityId theFaceIdx,
                                                                                       MeshElemEntityId theNodeIdx) const;

    template <>
    Standard_EXPORT MeshElemEntityId MeshAdaptor<MeshElementPolyhedron>::TrianglesIterator::nbTrgs() const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolyhedron>::TrianglesIterator::fillTrg() const;

    template <>
    Standard_EXPORT uint64_t MeshAdaptor<MeshElementPolyhedron>::NbTriangles(MeshEntityId theLower,
                                                                             MeshEntityId theUpper) const;

    template <>
    Standard_EXPORT void MeshAdaptor<MeshElementPolyhedron>::FaceNodes(const MeshElementPolyhedron &theElem,
                                                                       MeshElemEntityId theFaceIdx,
                                                                       std::vector<MeshEntityId> &theNodes,
                                                                       MeshElemType &theType) const;

    template <>
    Standard_EXPORT const MeshElementPolyhedron &MeshAdaptor<MeshElementPolyhedron>::GetElement(
        MeshEntityId theElemIndex) const;

    typedef MeshAdaptor<MeshElementPolyhedron> MeshAdaptorPolyhedron;

    //! @}

#ifdef Standard_UNUSED
// suppress CodeCheck warnings considering template exports as 'forward declarations'
#define RWCae_ExportTemplate
#endif

} // namespace

#endif // _MESHADAPTOR_HEADERFILE

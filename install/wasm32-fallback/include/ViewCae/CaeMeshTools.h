#ifndef _CAEMESHTOOLS_HEADERFILE
#define _CAEMESHTOOLS_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <ViewCae/InteractiveCaeMeshPrsMode.h>

#include <RWCae/MeshData.h>
#include <RWCae/MeshAdaptor.h>

#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_IncAllocator.hxx>
#include <NCollection_Map.hxx>
#include <TColStd_DataMapOfIntegerInteger.hxx>

namespace ViewCae {
//! Class which defines parts of volume data to be rendered.
enum class MeshShadedType { VOLUME, FACES, VOLUME_SURFACE };

//! Subclass to access private fields and contains auxiliary methods.
class CaeMeshArrayOfTriangles : public Graphic3d_ArrayOfTriangles {
public:
    //! Creates an array of triangles.
    explicit CaeMeshArrayOfTriangles(int theMaxVertexs,
                                     int theMaxEdges    = 0,
                                     int theHasVNormals = false,
                                     int theHasVColors  = false,
                                     int theHasVTexels  = false)
        : Graphic3d_ArrayOfTriangles(theMaxVertexs,
                                     theMaxEdges,
                                     ((theHasVNormals ? Graphic3d_ArrayFlags_VertexNormal : 0x0)
                                      | (theHasVColors ? Graphic3d_ArrayFlags_VertexColor : 0x0)
                                      | (theHasVTexels ? Graphic3d_ArrayFlags_VertexTexel : 0x0)
                                      | Graphic3d_ArrayFlags_AttribsDeinterleaved))
    {
        // Graphic3d_ArrayOfPrimitives constructor has different behaviour for interleaved/deinterleaved array.
        // Here the element counter is initialized to 0 to use usual Graphic3d_ArrayOfTriangles::AddVertex()
        // syntax with deinterleaved array, whereas the default initialization value would not allow it.
        Attributes()->NbElements = 0;
    }

    //! Set the number of defined vertex.
    void SetVertexNumber(int theNb) { Attributes()->NbElements = theNb; }

    //! Copy a bunch of vertices from other array.
    Standard_EXPORT void CopyAttributesFrom(const Handle(CaeMeshArrayOfTriangles)& theFromArray,
                                            RWCae::MeshElemEntityOffset          theFromVertex,
                                            RWCae::MeshElemEntityOffset          theToVertex,
                                            RWCae::MeshElemEntityOffset          theNbVertices);

    //! Create a new object from the other with specified number of vertices.
    Standard_EXPORT static Handle(CaeMeshArrayOfTriangles) CreateFrom(const Handle(CaeMeshArrayOfTriangles)& theOther,
                                                                      int                                    theNbVerts,
                                                                      int theNbIndices);
};

//! Auxiliary class for mesh presentation generation.
class CaeMeshTools {
    static constexpr int m_nbTNodes3 = 3, m_nbQNodes4 = 4;

public:
    //! Number of colors in the colors mapping texture.
    static constexpr int NbColorscaleColors = 256;

    //! Primitive array configuration.
    struct ArrayInfo {
        typedef std::pair<Handle(ViewCaeGeom::MeshGeometry), int>                           GeomTypePair;
        typedef std::vector<std::pair<RWCae::MeshEntityId, RWCae::MeshElemEntityOffset>> ElemNodeOffsets;
        typedef NCollection_DataMap<GeomTypePair, ElemNodeOffsets, ArrayInfo>                PrimitiveParts;

        PrimitiveParts parts;
        int            nbNodes   = 0;
        int            nbIndices = 0;

        //! Hash value, for Map interface.
        static int HashCode(const GeomTypePair& theElem, const int theUpper)
        {
            unsigned int aHashCode = 0;

            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.first, theUpper);
            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.second, theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! Matching two instances, for Map interface.
        static bool IsEqual(const GeomTypePair& theElem1, const GeomTypePair& theElem2)
        {
            return theElem1.first == theElem2.first && theElem1.second == theElem2.second;
        }

        bool IsValid() const { return nbNodes != 0 && nbIndices != 0; }

        void Add(const ArrayInfo& theOther)
        {
            nbNodes += theOther.nbNodes;
            nbIndices += theOther.nbIndices;
        }
    };

    //! Returns size of array of segments for mesh edges presentation.
    Standard_EXPORT static ArrayInfo GetMeshEdgesStats(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                       MeshShadedType                            theShadedType);

    //! Returns array of segments for mesh edges presentation.
    //! @param[in] theMeshGeom   chain of mesh geometries
    //! @param[in] theShadedType defines if array should be created either
    //!                          for 3D (volume) or 3D surface or 2D part of mesh
    Standard_EXPORT static Handle(Graphic3d_ArrayOfSegments) CreateMeshEdges(
        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
        MeshShadedType                            theShadedType);

    //! Returns array of segments for free mesh edges presentation.
    Standard_EXPORT static Handle(Graphic3d_ArrayOfSegments) CreateMeshFreeEdges(
        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
        MeshShadedType                            theShadedType);

    //! Returns array of segments for sharp mesh edges presentation.
    Standard_EXPORT static Handle(Graphic3d_ArrayOfSegments) CreateMeshSharpEdges(
        const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
        MeshShadedType                            theShadedType);

    //! Returns size of array of triangles for shaded presentation.
    Standard_EXPORT static ArrayInfo GetShadedMeshStats(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                                        MeshShadedType                            theShadedType,
                                                        const bool                                toDuplicate);

    //! Returns array of triangles for shaded presentation.
    //! @param[in] theMeshGeom      chain of mesh geometries
    //! @param[in] theGeomProps     map of scalar properties for each mesh geometry
    //! @param[in] theSmoothNormals if true then calculates smooth normals
    //! @param[in] theShadedType    defines if array should be created either
    //!                             for 3D (volume) or 3D surface or 2D part of mesh
    //! @param[in] theShrink        element shrinking coefficient (1.0 means no shrinking)
    Standard_EXPORT static Handle(Graphic3d_ArrayOfTriangles) CreateShadedMesh(
        const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
        const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
        bool                                          theSmoothNormals,
        MeshShadedType                                theShadedType,
        const float                                   theShrink);

    //! Returns array of points.
    //! @param[in] theMeshGeom     chain of mesh geometries
    //! @param[in] theGeomProps    map of scalar properties for each mesh geometry
    //! @param[in] toShowFreeNodes defines if free nodes should be visible
    Standard_EXPORT static Handle(Graphic3d_ArrayOfPoints) CreateNodes(
        const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
        const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
        bool                                          toShowFreeNodes);

    //! Returns array of 0D elems.
    //! @param[in] theMeshGeom   chain of mesh geometries
    //! @param[in] theGeomProps  map of scalar properties for each mesh geometry
    Standard_EXPORT static Handle(Graphic3d_ArrayOfPoints) CreateElems0d(
        const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
        const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps);

    //! Returns array of centroids.
    //! @param[in] theMeshGeom   chain of mesh geometries
    //! @param[in] theGeomProps  map of scalar properties for each mesh geometry
    //! @param[in] theShadedType defines if array should be created either
    //!                          for 3D (volume) or 3D surface or 2D part of mesh
    Standard_EXPORT static Handle(Graphic3d_ArrayOfPoints) CreateCentroids(
        const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
        const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
        const MeshShadedType                          theShadedType);

    //! Calculates UV coordinates for the value.
    Standard_EXPORT static Graphic3d_Vec2 GetUVCoords(const double theValue,
                                                      const double theMinValue,
                                                      const double theMaxValue,
                                                      const int    theNbValues);

    //! Extract mesh elements of specified type and corresponding property values to separate mesh data.
    Standard_EXPORT static bool ExtractElements(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const RWCae::MeshPropertyComponent&     theProp,
                                                InteractiveCaeMeshElementType             theType,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

    //! Extract mesh nodes and corresponding property values to separate mesh data.
    Standard_EXPORT static bool ExtractPoints(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                              const RWCae::MeshPropertyComponent&     theProp,
                                              const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

private:
    //! Returns number of nodes and indices for the mesh edges.
    template<class Elem_T>
    static inline void getMeshEdgesNb(const RWCae::MeshAdaptor<Elem_T>& theMeshAdaptor, ArrayInfo& theInfo);

    //! Adds mesh edges to the array.
    template<class Elem_T>
    static void addMeshEdges(const Handle(Graphic3d_ArrayOfSegments)&  theSegments,
                             const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                             const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Adds free mesh edges to the array.
    template<class Elem_T>
    static void addMeshFreeEdges(NCollection_Vector<Graphic3d_Vec3>&       theSegments,
                                 const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Compute merged triangulation.
    template<class Elem_T>
    static Handle(Poly_Triangulation) mergedTriangulation(const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                                          const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Adds sharp mesh edges to the array.
    template<class Elem_T>
    static void addMeshSharpEdges(NCollection_Vector<Graphic3d_Vec3>&       theSegments,
                                  const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                  const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Returns number of nodes and indices for the mesh triangles.
    template<class Elem_T>
    static void getShadedMeshElemsNb(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                                     const RWCae::MeshAdaptor<Elem_T>&       theMeshAdaptor,
                                     ArrayInfo&                                theInfo,
                                     bool                                      toDuplicate);

    //! Adds mesh triangles to the array.
    static void addShadedMesh(const Handle(CaeMeshArrayOfTriangles)&        theTris,
                              const CaeMeshTools::ArrayInfo&                theInfo,
                              MeshShadedType                                theShadedType,
                              const Handle(ViewCaeGeom::MeshGeometry)&     theMeshGeom,
                              const ViewCaeGeom::MapOfMeshGeomScalarProps& theGeomProps,
                              const bool                                    theSmoothNormals,
                              const float                                   theShrink);

    //! Adds mesh triangles to the array.
    template<class Elem_T>
    static inline void addShadedMeshT(const Handle(CaeMeshArrayOfTriangles)&          theTris,
                                      const CaeMeshTools::ArrayInfo::ElemNodeOffsets& theInfo,
                                      const RWCae::MeshAdaptor<Elem_T>&             theMeshAdaptor,
                                      const Handle(RWCae::MeshScalarPropertyBase)&  theProp,
                                      const Handle(ViewCaeGeom::MeshGeometry)&       theMeshGeom,
                                      const bool                                      theSmoothNormals,
                                      const float                                     theShrink);

    //! Adds mesh triangles to the unindexed array (nodes duplicated).
    template<class Elem_T>
    static inline void addShadedMeshSplit(const Handle(CaeMeshArrayOfTriangles)&          theTris,
                                          const CaeMeshTools::ArrayInfo::ElemNodeOffsets& theInfo,
                                          const RWCae::MeshAdaptor<Elem_T>&             theMesh,
                                          const Handle(RWCae::MeshScalarPropertyBase)&  theProp,
                                          const Handle(ViewCaeGeom::MeshGeometry)&       theMeshGeom,
                                          const bool                                      theSmoothNormals,
                                          const float                                     theShrink);

    //! Adds mesh triangles to the unindexed array (nodes duplicated).
    template<class Elem_T>
    static inline RWCae::MeshElemEntityOffset addShadedMeshSplitPart(
        const Handle(CaeMeshArrayOfTriangles)&          theTris,
        const CaeMeshTools::ArrayInfo::ElemNodeOffsets& theParts,
        const RWCae::MeshAdaptor<Elem_T>&             theMesh,
        const Handle(RWCae::MeshScalarPropertyBase)&  theProp,
        const Handle(ViewCaeGeom::MeshGeometry)&       theMeshGeom,
        const bool                                      theSmoothNormals,
        const float                                     theShrink,
        int                                             thePart,
        int                                             theVertsOffset);

    //! Adds mesh triangle to the unindexed array (nodes duplicated).
    template<class Elem_T>
    static inline void addShadedMeshTrg(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                        const Handle(CaeMeshArrayOfTriangles)&         theTempTris,
                                        const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                        const Graphic3d_Vec2d&                         theValMinMax,
                                        const Elem_T&                                  theElem,
                                        const Graphic3d_Vec3u&                         theTrg,
                                        const NCollection_Array1<Graphic3d_Vec3>&      theTriNodes,
                                        const bool                                     theSmoothNormals,
                                        const int                                      theStartVertIdx,
                                        Graphic3d_Vec3&                                theNorm,
                                        RWCae::MeshElemEntityOffset&                 theNbAddedVerts);

    //! Adds mesh triangles to the indexed array (nodes shared).
    template<class Elem_T>
    static void inline addShadedMeshMerged(const Handle(CaeMeshArrayOfTriangles)&         theTris,
                                           const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                           const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                           const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom);

    //! Adds nodes to the array if points.
    static void addNodes(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                         const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                         const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                         bool                                           toShowFreeNodes,
                         bool                                           hasUVs);

    //! Adds element centroids to the array if points.
    static int addCentroids(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                            const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                            const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom,
                            const MeshShadedType                           theShadedType);

    //! Count element centroids.
    template<class Elem_T>
    static int inline nbCentroidsT(const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Adds element centroids to the array if points.
    template<class Elem_T>
    static int inline addCentroidsT(const Handle(Graphic3d_ArrayOfPoints)&         thePnts,
                                    const RWCae::MeshAdaptor<Elem_T>&            theMesh,
                                    const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                    const Handle(ViewCaeGeom::MeshGeometry)&      theMeshGeom);

    //! Fills an array of triangles buffer with some element values.
    //! This is a workaround to sidestep OCCT requirement that buffers with deinterleaved attributes must be full.
    static void fillDeinterleavedArrayOfTriangles(const Handle(CaeMeshArrayOfTriangles)& theArray);

private:
    //! Calculate triangle normal.
    static bool getTriangleNormal(Graphic3d_Vec3& theNorm, const NCollection_Array1<Graphic3d_Vec3>& theTriNodes)
    {
        const Graphic3d_Vec3 aVec01   = theTriNodes[1] - theTriNodes[0];
        const Graphic3d_Vec3 aVec02   = theTriNodes[2] - theTriNodes[0];
        const Graphic3d_Vec3 aCross   = Graphic3d_Vec3::Cross(aVec01, aVec02);
        const float          aModulus = aCross.Modulus();
        if (aModulus == 0.0f) {
            return false; // degenerate element
        }

        theNorm = aCross / aModulus;
        return true;
    }

    //! Fetch modified nodes by its indices.
    template<class Elem_T>
    static void getAndModifyNodes(std::vector<Graphic3d_Vec3>&              theNodes,
                                  const std::vector<RWCae::MeshEntityId>& theNodesIds,
                                  const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                  const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom);

    //! Calculate center of nodes.
    static void nodesCenter(std::vector<Graphic3d_Vec3>& theNodes, Graphic3d_Vec3& theCenter);

    //! Get triangle nodes from element's nodes.
    static void getTriNodes(NCollection_Array1<Graphic3d_Vec3>& theTriNodes,
                            const std::vector<Graphic3d_Vec3>&  theElemNodes,
                            const Graphic3d_Vec3ub&             theTrgIndices,
                            const Graphic3d_Vec3&               theElemCenter,
                            const float                         theShrink)
    {
        static constexpr int n0 = 0;
        static constexpr int n1 = 1;
        static constexpr int n2 = 2;

        if (theShrink < 1.0f) {
            theTriNodes[n0] = theElemCenter + (theElemNodes[theTrgIndices[n0]] - theElemCenter) * theShrink;
            theTriNodes[n1] = theElemCenter + (theElemNodes[theTrgIndices[n1]] - theElemCenter) * theShrink;
            theTriNodes[n2] = theElemCenter + (theElemNodes[theTrgIndices[n2]] - theElemCenter) * theShrink;
        } else {
            theTriNodes[n0] = theElemNodes[theTrgIndices[n0]];
            theTriNodes[n1] = theElemNodes[theTrgIndices[n1]];
            theTriNodes[n2] = theElemNodes[theTrgIndices[n2]];
        }
    }

private:
    //! Extract components from the specified property.
    Standard_EXPORT static void extractProperty(const Handle(RWCae::MeshData)&          theMeshData,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const RWCae::MeshPropertyComponent&     theProp,
                                                Handle(RWCae::MeshScalarPropertyBase)&  theSrcComp,
                                                Handle(RWCae::MeshScalarPropertyBase)&  theDstComp);

    //! Extract mesh elements of specified type and corresponding property values to separate mesh data.
    template<class Elem_T>
    Standard_EXPORT static void extractElements(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const RWCae::MeshPropertyComponent&     theProp,
                                                const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);

    //! Add extracted mesh elements to separate mesh data.
    template<class Elem_T>
    Standard_EXPORT static void addExtractedElem(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                 const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom,
                                                 const Handle(RWCae::MeshData)&          theMeshData,
                                                 const RWCae::MeshAdaptor<Elem_T>&       theMesh,
                                                 RWCae::MeshEntityId                     theElemIndex,
                                                 int                                       theGlobalIndex,
                                                 Elem_T&                                   theElem);

    //! Extract mesh nodes and corresponding property values to separate mesh data.
    Standard_EXPORT static void extractPoints(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                              const RWCae::MeshPropertyComponent&     theProp,
                                              const Handle(ViewCaeGeom::MeshGeometry)& theGenGeom);
};

} // namespace

#endif // _CAEMESHTOOLS_HEADERFILE
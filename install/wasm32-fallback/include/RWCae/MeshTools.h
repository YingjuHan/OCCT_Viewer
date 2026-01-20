#ifndef _MESHTOOLS_HEADERFILE
#define _MESHTOOLS_HEADERFILE

#include <RWCae/MeshAdaptor.h>
#include <RWCae/MeshFieldMappingInfo.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <RWCae/MeshScalarPropertyBase.h>

#include <Message.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_StdAllocator.hxx>
#include <TCollection_AsciiString.hxx>

#include <vector>

namespace RWCae {

typedef NCollection_DataMap<TCollection_AsciiString, NCollection_Array1<Bnd_Range>> PropertyRanges;

//! Auxiliary class used for operations on mesh
class MeshTools {
public:
    //! Fill the map with quantity ranges.
    Standard_EXPORT static void fillPropertiesRange(MeshNVecProperties& theProperties, PropertyRanges& theRanges);

    //! Apply stored quantity ranges to mesh properties.
    Standard_EXPORT static void restorePropertiesRange(const MeshNVecProperties& theProperties,
                                                       const PropertyRanges&     theRanges);

    typedef std::pair<Handle(MeshData), std::vector<TCollection_AsciiString>> MeshDataBlockPair;
    typedef std::vector<MeshDataBlockPair>                                    MeshDataList;

    //! Combine list of mesh data into single mesh data.
    //! Nodes and elements vectors are appended to the corresponding output mesh data vectors.
    //! Properties are combined if they have the same name, type, timestamp.
    Standard_EXPORT static Handle(MeshData) MergeMeshData(const MeshDataList& theMeshDataList,
                                                          const bool          theToWriteProps = true);

    //! Sort mesh data nodes and elements.
    //! Mesh entities are sorted according to the space-filling Hilbert curve
    //! that is built on the whole mesh bounding box.
    //! Corresponding property values are also sorted.
    Standard_EXPORT static void SortMeshData(const Handle(MeshData)& theMeshData);

    //! Compute a ratio for transforming a given vector field into length using the mesh AABB.
    Standard_EXPORT static double ComputeVectorTrsfRatio(const Handle(MeshData)&         theMeshData,
                                                         const Handle(MeshNVecProperty)& theVecProperty);

    //! Load mesh property from reference file.
    Standard_EXPORT static bool LoadMeshProperty(const Handle(MeshData)&         theMeshData,
                                                 const Handle(MeshNVecProperty)& theProp);

    //! Dump mesh property to the file.
    Standard_EXPORT static bool DumpMeshProperty(const Handle(MeshData)&         theMeshData,
                                                 const Handle(MeshNVecProperty)& theProp,
                                                 const TCollection_AsciiString&  theFile);

    //! Convert values in components of theProp from theSourceUnits to base SI units.
    //! The value range is also converted.
    Standard_EXPORT static void ConvertMeshProperty(const Handle(MeshNVecProperty)& theProp,
                                                    const MeshMappingFieldType      thePropType,
                                                    const std::vector<std::string>& theSourceUnits);

    //! Set the measurement units to be used when displaying the property components to the users.
    //! No value conversion occurs here as all imported field values are supposed to be in base SI units
    //! and they are stored also in SI.
    Standard_EXPORT static void UpdateMeshPropertyUnits(const Handle(MeshNVecProperty)& theProp,
                                                        const MeshMappingFieldType      thePropType,
                                                        const std::vector<std::string>& theComponentUnits);

    //! Class for computing point position inside mesh element.
    //! Used algorithm for general polyhedrons from
    //! "Spherical Barycentric Coordinates", Eurographics Symposium on Geometry Processing, 2006
    template<class Element_T>
    class PointElementChecker {
        static constexpr int m_n0 = 0, m_n1 = 1, m_n2 = 2, m_n3 = 3;

    public:
        //! Constructor.
        Standard_EXPORT PointElementChecker(const MeshAdaptor<Element_T>*            theAdaptor,
                                            const Element_T&                         theElem,
                                            const Graphic3d_Vec3&                    thePnt,
                                            float*                                   theWeights,
                                            const Handle(NCollection_BaseAllocator)& theAlloc = nullptr);

        // Check if the point is inside and compute nodes weights.
        Standard_EXPORT bool Perform();

    protected:
        //! Check if input point is inside mesh element.
        bool CheckIsInside();

        //! Check if input point is on 2d mesh element.
        bool CheckIsInside2d();

        //! Check if face contains node.
        bool CheckFaceNode(MeshEntityId theNodeId, MeshElemEntityId theFace, MeshElemEntityId theNbFaceNodes);

        //! Compute nodes weights for element with planar faces.
        bool PerformPlanar();

        //! Compute vectors from the point to element's nodes.
        bool ComputeNodePntVec();

        //! Compute unit vector for the current face.
        void ComputeFaceVector();

        //! Compute angles between vectors.
        bool ComputeFaceAngles();

        //! Compute weights if the point lies on the current face.
        bool ComputePlanePointWeights();

        //! Return distance ratio of point on segment from start point to plane.
        float InversedNormPointPlaneDistance(const Graphic3d_Vec3& theStartPnt,
                                             const Graphic3d_Vec3& theDirPnt,
                                             const Graphic3d_Vec3& thePlnPnt,
                                             const Graphic3d_Vec3& thePlnNorm);

        //! Check if point lies inside triangle.
        bool CheckPntTrgIntersection(const Graphic3d_Vec3& theDir,
                                     const Graphic3d_Vec3& theTrgEdge1,
                                     const Graphic3d_Vec3& theTrgEdge2);

        //! Check if line intersects triangle.
        bool CheckLineTrgIntersection(const Graphic3d_Vec3& theStartPnt,
                                      const Graphic3d_Vec3& theEndPnt,
                                      const Graphic3d_Vec3& theTrgPnt1,
                                      const Graphic3d_Vec3& theTrgPnt2,
                                      const Graphic3d_Vec3& theTrgPnt3,
                                      Graphic3d_Vec3&       theNorm);

    protected:
        Handle(NCollection_BaseAllocator) myAlloc;

        const MeshAdaptor<Element_T>* myAdaptor;
        const Element_T*              myElem;
        Graphic3d_Vec3                myPnt;
        float*                        myWeights;
        float                         myWeightsSum = 0.0f;

        Graphic3d_Vec3 myCenter;

        MeshElemEntityId myNbNodes = 0;
        MeshElemEntityId myNbFaces = 0;

        MeshElemEntityId myCurFace        = 0;
        MeshElemEntityId myCurNbFaceNodes = 0;

        std::vector<Graphic3d_Vec3, NCollection_StdAllocator<Graphic3d_Vec3>> myNodePntVec;
        std::vector<float, NCollection_StdAllocator<float>>                   myNodePntVecLength;

        Graphic3d_Vec3 myCurFaceVec;
        float          myCurFaceVecLength = 0.0f;
        float          myCurFaceCoeff     = 1.0f;

        std::vector<float, NCollection_StdAllocator<float>> myCurAlpha1Angles;
        std::vector<float, NCollection_StdAllocator<float>> myCurAlpha2Angles;
        std::vector<float, NCollection_StdAllocator<float>> myCurThetaAngles;

        bool myIsPlanar = true;
    };

private:
    //! Convert a given property component to base SI units
    static void convertComponent(const Handle(MeshScalarPropertyBase)& theComp,
                                 const Handle(MeshNodeVecProperty)&    theNodeVecComp,
                                 const TCollection_AsciiString&        theUnits);
};

#ifdef Standard_UNUSED
    // suppress CodeCheck warnings considering template exports as 'forward declarations'
    #define RWCae_ExportTemplate
#endif
}

#endif // RWCae_MESHTOOLS_HEADERFILE

#ifndef RWCBASE_MESHDATA_HEADERFILE
#define RWCBASE_MESHDATA_HEADERFILE

#include <RWCae/MeshDataBlock.h>
#include <RWCae/MeshDataRange.h>
#include <RWCae/MeshDataVector.h>
#include <RWCae/MeshElements.h>
#include <RWCae/MeshNVecProperty.h>

#include <Bnd_Box.hxx>
#include <Graphic3d_Buffer.hxx>
#include <NCollection_DataMap.hxx>
#include <NCollection_Sequence.hxx>
#include <Poly_Triangulation.hxx>
#include <TColStd_PackedMapOfInteger.hxx>
#include <TopoDS_Face.hxx>

namespace RWCae {

//! The data source for working with CAE mesh data.
//! MeshData is defined by:
//! 1. Nodes (MeshDataVector<Graphic3d_Vec3>)
//!    - Stored as vector of 3d float coordinates
//!    - Can be added with AddNodePosition() and retrieved with Nodes() methods
//! 2. Elements (MeshElements[MeshElemNbTypes])
//!    - Stored as packed mesh elements container for each RWCae::MeshElemType
//!    - Can be added with AddElement() template method
//!    - Separate mesh element is accessed via MeshAdaptor of corresponding type
//! 3. Surface elements (MeshElements[MeshSurfElemNbTypes])
//!    - Stored as packed mesh elements container for each 2d RWCae::MeshElemType
//!    - These elements are automatically constructed as outer surface of 3d mesh elements
//! 4. Indexed elements (MeshDataVector<IndexedElement>)
//!    - Stored as array of <type, index> pairs which describe all added elements
//!      by type (RWCae::MeshElemType) and index in corresponding MeshElements container
//!    - Automatically filled within AddElement() method, so elements are stored in their adding order
//! 5. Fields (MeshNVecProperties)
//!    - Stored as array of numeric values fields (MeshNVecProperty)
//! 6. Blocks (MeshDataBlock)
//!    - Stored as tree structure that maps each mesh element to block name
//!    - Each node stores block's name, elements indices and nodes range (in indexed elements / nodes arrays),
//!      and array of child blocks
class MeshData : public Poly_Triangulation {
public:
    //! Define the memory allocation strategy.
    enum AllocStrategy {
        AllocStrategy_Resizable,        //!< resizable array
        AllocStrategy_Fixed,            //!< fixed array size (compact storage)
        AllocStrategy_ResizablePosNorm, //!< resizable array, but without extra elements (no UV, colors)
    };

    // element node indices
    static constexpr int m_n0 = 0, m_n1 = 1, m_n2 = 2, m_n3 = 3;
    static constexpr int m_defBlockSize4 = 4, m_defBlockSize256 = 256;

    typedef NCollection_DataMap<MeshEntityId, MeshEntityId> EntityMap;

    //! Element type and it's index within typed array.
    struct IndexedElement {
        MeshElemType Type;  //!< element type
        MeshEntityId Index; //!< element index within array of elements of this type

        //! Empty constructor.
        IndexedElement() : Type(MeshElemType::Tetrahedron), Index(0) {}

        //! Main constructor.
        IndexedElement(MeshElemType theType, MeshEntityId theIndex) : Type(theType), Index(theIndex) {}
    };

    //! Create a Face from the given Mesh.
    //! This method also updates cached AABB.
    //! @sa BRep_Builder::MakeFace()
    Standard_EXPORT static TopoDS_Face MakeFace(const Handle(Poly_Triangulation)& theMesh);

    //! Update a Face with the given Mesh.
    //! This method also updates cached AABB.
    //! @sa BRep_Builder::UpdateFace()
    Standard_EXPORT static void UpdateFace(const TopoDS_Face& theFace, const Handle(Poly_Triangulation)& theMesh);

    //! Add bounding box of the shape.
    //! @sa BRepBndLib::Add()
    Standard_EXPORT static void AddBoundingBox(Bnd_Box& theBox, const TopoDS_Shape& theShape);

    //! Compute an estimated origin of the point set to reduce double->float precision conversion issue.
    //! So that data model will store points in single precision relative to estimated origin,
    //! and store this origin as transformation (with double precision).
    Standard_EXPORT static gp_XYZ EstimateOrigin(const NCollection_Sequence<gp_Pnt>& thePoints,
                                                 const gp_Pnt& theCurrentOrigin = gp_Pnt(0.0, 0.0, 0.0));

public:
    //! Empty constructor.
    Standard_EXPORT explicit MeshData(const TCollection_AsciiString& theId            = TCollection_AsciiString(),
                                      AllocStrategy                  theAllocStrategy = AllocStrategy_Resizable);

    //! Destructor. Clears dependency references in contained properties.
    Standard_EXPORT ~MeshData();

    //! Clear mesh structure.
    Standard_EXPORT virtual void Clear() Standard_OVERRIDE;

    //! Returns TRUE if triangulation has some geometry.
    virtual Standard_Boolean HasGeometry() const Standard_OVERRIDE { return !myNodes.IsEmpty(); }

public:
    //! Return id.
    const TCollection_AsciiString& Id() const { return myId; }

    //! Set id.
    void SetId(const TCollection_AsciiString& theId) { myId = theId; }

    //! Get name.
    const TCollection_AsciiString& Name() const { return myName; }

    //! Set name.
    void SetName(const TCollection_AsciiString& theName) { myName = theName; }

    //! Format mesh data stats into a string.
    Standard_EXPORT void PrintStats(std::ostream& theStream) const;

    //! Return overall nodes number.
    MeshEntityId NbNodes() const { return myNodes.Length(); }

    //! Return vector of nodes.
    const MeshDataVector<Graphic3d_Vec3>& Nodes() const { return myNodes; }

    //! Return vector of nodes.
    //! Call @c SetCachedMinMax(Bnd_Box()) after changing nodes!
    MeshDataVector<Graphic3d_Vec3>& ChangeNodes() { return myNodes; }

    //! Return true if per-node colors have been assigned
    Standard_Boolean HasNodalColors() const
    {
        return myNodalColors.Length() == myNodes.Length() && !myNodalColors.IsEmpty();
    }

    //! Return vector of nodal colors (optional).
    const MeshDataVector<Graphic3d_Vec4ub>& NodalColors() const { return myNodalColors; }

    //! Return true if per-node normals have been assigned
    Standard_Boolean HasNormals() const { return myNormals.Length() == myNodes.Length() && !myNormals.IsEmpty(); }

    //! Return vector of nodal normals (optional).
    const MeshDataVector<Graphic3d_Vec3>& Normals() const { return myNormals; }

    //! Return vector of nodal normals (optional).
    MeshDataVector<Graphic3d_Vec3>& ChangeNormals() { return myNormals; }

    //! Return true if per-node UV parameters have been assigned
    Standard_Boolean HasUV() const { return myNodesUV.Length() == myNodes.Length() && !myNodesUV.IsEmpty(); }

    //! Return vector of nodal UV parameters (optional).
    const MeshDataVector<Graphic3d_Vec2>& NodesUV() const { return myNodesUV; }

    //! Return vector of nodal UV parameters (optional).
    MeshDataVector<Graphic3d_Vec2>& ChangeNodesUV() { return myNodesUV; }

    //! Return a cached bounding box. If it isn't calculated, calculate it first.
    //! Should be used consciously since it might introduce a performance penalty in case of large CAE datasets.
    //! Consider using CachedMinMax() as a faster and more straightforward alternative.
    const Bnd_Box& BoundingBox()
    {
        if (!HasCachedMinMax()) {
            UpdateCachedMinMax();
        }
        return CachedMinMax();
    }

    //! Append new vertex.
    MeshEntityId AddNodePosition(const Graphic3d_Vec3& thePosition)
    {
        myNodes.Append(thePosition);
        unsetCachedMinMax();
        return myNodes.Upper();
    }

    //! Append new vertex and check input values for NaN.
    //! Return FALSE if NaN values have been found.
    //! In case of NaN the node will be added with specified default value.
    bool AddNodePositionChecked(const gp_XYZ& thePosition, const double theDefault = 0.0)
    {
        bool   isOk = true;
        gp_XYZ aCopy(thePosition);
        if (IsNaN(aCopy.X())) {
            isOk = false;
            aCopy.SetX(theDefault);
        }
        if (IsNaN(aCopy.Y())) {
            isOk = false;
            aCopy.SetY(theDefault);
        }
        if (IsNaN(aCopy.Z())) {
            isOk = false;
            aCopy.SetZ(theDefault);
        }

        myNodes.Append(Graphic3d_Vec3((float)aCopy.X(), (float)aCopy.Y(), (float)aCopy.Z()));

        unsetCachedMinMax();

        return isOk;
    }

    //! Append new vertex.
    MeshEntityId AddNodePosition(const gp_XYZ& thePosition)
    {
        myNodes.Append(Graphic3d_Vec3((float)thePosition.X(), (float)thePosition.Y(), (float)thePosition.Z()));
        unsetCachedMinMax();
        return myNodes.Upper();
    }

    //! Set vertex value.
    void SetNodePosition(const MeshEntityId theNodeIndex, const Graphic3d_Vec3& thePosition)
    {
        myNodes.SetValue(theNodeIndex, thePosition);
        unsetCachedMinMax();
    }

    //! Set vertex value.
    void SetNodePosition(const MeshEntityId theNodeIndex, const gp_XYZ& thePosition)
    {
        myNodes.SetValue(theNodeIndex,
                         Graphic3d_Vec3((float)thePosition.X(), (float)thePosition.Y(), (float)thePosition.Z()));
        unsetCachedMinMax();
    }

    //! Append new normal.
    void AddNodeNormal(const Graphic3d_Vec3& theNormal) { myNormals.Append(theNormal); }

    //! Append new normal.
    void AddNodeNormal(const gp_XYZ& theNormal)
    {
        myNormals.Append(Graphic3d_Vec3((float)theNormal.X(), (float)theNormal.Y(), (float)theNormal.Z()));
    }

    //! Set vertex normal.
    void SetNodeNormal(const MeshEntityId theNodeIndex, const Graphic3d_Vec3& thePosition)
    {
        myNormals.SetValue(theNodeIndex, thePosition);
    }

    //! Append new UV parameters pair.
    void AddNodeUV(const Graphic3d_Vec2& theUV) { myNodesUV.Append(theUV); }

    //! Set UV parameters pair.
    void AddNodeUV(const MeshEntityId theNodeIndex, const Graphic3d_Vec2& theUV)
    {
        myNodesUV.SetValue(theNodeIndex, theUV);
    }

    //! Set vertex UV.
    void SetNodeUV(const MeshEntityId theNodeIndex, const Graphic3d_Vec2& theUV)
    {
        myNodesUV.SetValue(theNodeIndex, theUV);
    }

    //! Append new color.
    void AddNodeColor(const Graphic3d_Vec4ub& theColor) { myNodalColors.Append(theColor); }

    //! Set vertex color.
    void SetNodeColor(const MeshEntityId theNodeIndex, const Graphic3d_Vec4ub& theColor)
    {
        myNodalColors.SetValue(theNodeIndex, theColor);
    }

public:
    //! Append new element.
    template<class Elem_T>
    int AddElement(const Elem_T& theElement, bool theToAddIndexed = true)
    {
        if ((int)theElement.Type >= MeshElem3dLowerType && (int)theElement.Type <= MeshElem3dUpperType) {
            SetToUpdateReconstructedSurface(true);
        }

        myNodesToElemsUpdated = false;

        MeshElements& anElems = myElements[(int)theElement.Type];

        int aGlobalIndex = theElement.GlobalIndex();
        if (theToAddIndexed) {
            myIndexedElements.Append(IndexedElement(theElement.Type, anElems.NbElements()));
            aGlobalIndex = myIndexedElements.Upper();
        }
        anElems.AddElement(theElement, aGlobalIndex);
        return myIndexedElements.Upper();
    }

    //! Add indexed element manually.
    void AddIndexedElement(const IndexedElement& theElem) { myIndexedElements.Append(theElem); }

    //! Return indexed elements.
    const MeshDataVector<IndexedElement>& IndexedElements() const { return myIndexedElements; }

    //! Return indexed elements.
    MeshDataVector<IndexedElement>& ChangeIndexedElements() { return myIndexedElements; }

    //! Return mesh elements of specified type.
    const MeshElements& Elements(MeshElemType theType) const { return myElements[(int)theType]; }

    //! Return mesh elements of specified type.
    MeshElements& ChangeElements(MeshElemType theType) { return myElements[(int)theType]; }

    //! Return node indices for a mesh element.
    Standard_EXPORT void ElementNodes(const MeshEntityId theElementIndex, std::vector<MeshEntityId>& theNodes) const;

    //! Return the number of Indexed (0d+2d+3d) elements.
    MeshEntityId NbIndexedElems() const { return myIndexedElements.Size(); }

    //! Return the number of 0d elements.
    MeshEntityId NbElems0d() const { return NbElems(MeshElemType::Elem0d); }

    //! Return the number of 2d elements.
    MeshEntityId NbElems2d() const
    {
        return NbElems(MeshElemType::Triangle) + NbElems(MeshElemType::Triangle2nd) + NbElems(MeshElemType::Quad)
             + NbElems(MeshElemType::Quad2nd) + NbElems(MeshElemType::Polygon);
    }

    //! Return the number of Volume (3d) elements.
    MeshEntityId NbElems3d() const
    {
        return NbElems(MeshElemType::Tetrahedron) + NbElems(MeshElemType::Tetrahedron2nd)
             + NbElems(MeshElemType::Pyramid) + NbElems(MeshElemType::Pyramid2nd) + NbElems(MeshElemType::Prism)
             + NbElems(MeshElemType::Prism2nd) + NbElems(MeshElemType::Hexahedron)
             + NbElems(MeshElemType::Hexahedron2nd) + NbElems(MeshElemType::Polyhedron);
    }

    //! Return the number of elements of specified type.
    MeshEntityId NbElems(MeshElemType theType) const { return Elements(theType).NbElements(); }

public:
    //! Return the number of outer surface elements.
    MeshEntityId NbSurfElems() const
    {
        return NbSurfElems(MeshElemType::Triangle) + NbSurfElems(MeshElemType::Triangle2nd)
             + NbSurfElems(MeshElemType::Quad) + NbSurfElems(MeshElemType::Quad2nd)
             + NbSurfElems(MeshElemType::Polygon);
    }

    //! Return the number of outer surface elements of specified type.
    MeshEntityId NbSurfElems(MeshElemType theType) const { return SurfaceElements(theType).NbElements(); }

    //! Calculate (if needed) and return outer surface elements of specified type.
    Standard_EXPORT const MeshElements& SurfaceElements(MeshElemType theType) const;

    //! Return outer surface elements of specified type.
    MeshElements& ChangeSurfaceElements(MeshElemType theType)
    {
        Standard_ASSERT_RAISE((int)theType >= MeshSurfElemLowerType && (int)theType <= MeshSurfElemUpperType,
                              "Error: Wrong elements type of mesh outer surface");
        int aType = (int)theType - MeshSurfElemLowerType;
        return mySurfElements[aType];
    }

    //! Mark surface elements as needed to rebuild.
    void SetToUpdateReconstructedSurface(bool theToUpdate = true) { myAreSurfaceElementsUpdated = !theToUpdate; }

    //! Append new outer surface element.
    template<class Elem_T>
    void AddSurfaceElement(const Elem_T& theElement)
    {
        SetToUpdateReconstructedSurface(false);

        int aType = (int)theElement.Type - MeshSurfElemLowerType;

        MeshElements& anElems = mySurfElements[aType];
        anElems.AddElement(theElement);
    }

public:
    //! Compute and return adjacent elements for the specified node.
    Standard_EXPORT const MeshEntityId* NodeAdjElems(MeshEntityId theNodeIdx, MeshElemEntityId& theNbAdjElems);

    //! Compute (if needed) nodes adjacent elements.
    Standard_EXPORT void UpdateNodesAdjElems();

public:
    //! Return mesh properties.
    const MeshNVecProperties& Properties() const { return myProperties; }

    //! Return mesh properties.
    MeshNVecProperties& ChangeProperties() { return myProperties; }

    //! Return TRUE if specified property is defined within this mesh data.
    Standard_EXPORT const Handle(MeshNVecProperty)& HasProperty(const Handle(MeshNVecProperty)& theProp) const;

    //! Return property by id and optional time step (nullptr if not found).
    Standard_EXPORT const Handle(MeshNVecProperty)& GetPropertyById(const TCollection_AsciiString& theId,
                                                                    const int theTimeStep = -1) const;

    //! Return number of properties of specified type.
    Standard_EXPORT int NbProperties(MeshPropertyBase::MappingType theType) const;

    //! Return list of properties ids by property name.
    Standard_EXPORT NCollection_List<TCollection_AsciiString> GetPropertyIdsByName(
        const TCollection_AsciiString& theName) const;

    //! Return quantities combination identifier.
    int FemQuantitiesId() const { return myFemQuantitiesId; }

    //! Set quantities combination identifier.
    void SetFemQuantitiesId(int theId) { myFemQuantitiesId = theId; }

    //! Reset mesh data id static counter.
    Standard_EXPORT static void ResetStaticCounter();

public:
    //! Return the map of indices of free nodes.
    const TColStd_PackedMapOfInteger& FreeNodes() const { return myFreeNodes; }

    //! Fill in the map of free nodes (nodes that are not used by mesh elements).
    Standard_EXPORT void InitFreeNodes();

    //! Check if a node is not used by mesh elements.
    bool IsNodeFree(int theNodeIndex)
    {
        InitFreeNodes();
        return myFreeNodes.Contains(theNodeIndex);
    }

public:
    //! Return mapping between nodes ids and their indices in the nodes vector.
    EntityMap& IndexedNodes() { return myIndexedNodes; }

    //! Return mapping between elements ids and their indices in the elements vector.
    EntityMap& IndexedElems() { return myIndexedElems; }

public:
    //! Return mesh data blocks.
    const Handle(MeshDataBlock)& Blocks() const
    {
        updateBlocks(false);
        return myBlocks;
    }

    //! Set mesh data blocks.
    void SetBlocks(const Handle(MeshDataBlock)& theBlocks)
    {
        myBlocks = theBlocks;
        updateBlocks(true);
    }

protected:
    //! Return default allocator.
    static const Handle(NCollection_BaseAllocator)& defaultAllocator(MeshData::AllocStrategy)
    {
        return NCollection_BaseAllocator::CommonBaseAllocator();
    }

    //! Return default increment.
    static int defaultIncrement(MeshData::AllocStrategy theAllocStrategy, int theType)
    {
        switch (theAllocStrategy) {
            case MeshData::AllocStrategy_Resizable: return m_defBlockSize256;
            case MeshData::AllocStrategy_Fixed: return 0;
            case MeshData::AllocStrategy_ResizablePosNorm:
                return theType == Graphic3d_TOA_POS || theType == Graphic3d_TOA_NORM || theType == -1
                         ? m_defBlockSize256
                         : 0;
        }
        return 0;
    }

    //! Calculate bounding box of nodal data.
    Standard_EXPORT virtual Bnd_Box computeBoundingBox(const gp_Trsf& theTrsf) const Standard_OVERRIDE;

    //! Print elements info.
    void printElemStats(std::ostream& theStream) const;

    //! Print properties info.
    void printPropStats(std::ostream& theStream) const;

    //! Compute ranges of internal blocks, sort blocks and check for its consistency.
    Standard_EXPORT void updateBlocks(bool theForced) const;

protected:
    class CalcOriginTool;

protected:
    TCollection_AsciiString myId;   //!< id
    TCollection_AsciiString myName; //!< name

    TColStd_PackedMapOfInteger     myFreeNodes;              //!< indices of free nodes
    bool                           myToInitFreeNodes = true; //!< flag indicating that Free Nodes should be initialized
    MeshDataVector<Graphic3d_Vec3> myNodes;                  //!< vector of nodes
    MeshDataVector<Graphic3d_Vec3> myNormals;                //!< vector of nodal normals (optional)
    MeshDataVector<Graphic3d_Vec2> myNodesUV;                //!< vector of UV nodes (optional)
    MeshDataVector<Graphic3d_Vec4ub> myNodalColors;          //!< vector of nodal colors  (optional)

    MeshDataVector<IndexedElement> myIndexedElements;           //!< vector of indexed elements
    MeshElements                   myElements[MeshElemNbTypes]; //!< per-type elements

    mutable MeshElements mySurfElements[MeshSurfElemNbTypes]; //!< per-type outer surface elements
    mutable bool         myAreSurfaceElementsUpdated = false;

    std::vector<MeshEntityId> myNodesToElemsOffsets;
    std::vector<MeshEntityId> myNodesToElems;
    bool                      myNodesToElemsUpdated = false;

    MeshNVecProperties myProperties;          //!< n-components quantities
    int                myFemQuantitiesId = 0; //!< scalar quantities combination identifier

    EntityMap myIndexedNodes;
    EntityMap myIndexedElems;

    mutable Handle(MeshDataBlock) myBlocks;
};

} // namespace

#endif // _MESHDATA_HEADERFILE        

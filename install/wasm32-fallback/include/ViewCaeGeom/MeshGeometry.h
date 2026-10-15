#ifndef _MESHGEOMETRY_HEADERFILE
#define _MESHGEOMETRY_HEADERFILE

#include <ViewCaeGeom/MeshBvhData.h>

#include <RWCae/MeshAdaptor.h>
#include <RWCae/MeshData.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <RWBase/JsonHelper.h>

#include <vector>

namespace ViewCaeGeom {
typedef NCollection_DataMap<TCollection_AsciiString, RWCae::MeshPropertyComponent>    MapOfMeshGeomScalarProps;
typedef NCollection_DataMap<TCollection_AsciiString, Handle(RWCae::MeshNVecProperty)> MapOfMeshGeomNVecProps;

//! Mesh geometry type.
//! Also used as order of mesh geometry in chain of geometries.
enum class MeshGeometryType { Base = 0, Block, Deformed, Clip, Slice, Iso, Gen };

//! Class for describing a node as interpolation result of several nodes.
class MeshInterpolatedNode {
public:
    //! Constructor.
    explicit MeshInterpolatedNode(int theNbBaseNodes = 1) { myBaseNodes.reserve(theNbBaseNodes); }

    //! Add a base node to the list of nodes.
    void AddBaseNode(RWCae::MeshEntityId theNodeIdx, double theRatio)
    {
        myBaseNodes.push_back(std::make_pair(theNodeIdx, theRatio));
    }

    //! Return list of base nodes.
    const std::vector<std::pair<RWCae::MeshEntityId, double>>& BaseNodes() const { return myBaseNodes; }

    //! Return list of base nodes.
    std::vector<std::pair<RWCae::MeshEntityId, double>>& BaseNodes() { return myBaseNodes; }

    //! Hash value, for Map interface.
    Standard_EXPORT static int HashCode(const MeshInterpolatedNode& theElem, const int theUpper);

    //! Matching two instances, for Map interface.
    Standard_EXPORT static bool IsEqual(const MeshInterpolatedNode& theElem1, const MeshInterpolatedNode& theElem2);

    //! Matching two instances, for Map interface.
    Standard_EXPORT static bool IsEqualGeneral(const MeshInterpolatedNode& theElem1,
                                               const MeshInterpolatedNode& theElem2);

protected:
    static constexpr double myRatioConfusion = 1e-3;

    std::vector<std::pair<RWCae::MeshEntityId, double>> myBaseNodes;
};

//! Struct for storing mesh element edge as face-edge ids.
struct MeshElemEdge {
    RWCae::MeshElemEntityId Face    = 0;
    RWCae::MeshElemEntityId Id      = 0;
    bool                      IsValid = false;

    //! Constructor.
    MeshElemEdge() {}

    //! Constructor.
    MeshElemEdge(RWCae::MeshElemEntityId theFace, RWCae::MeshElemEntityId theId)
        : Face(theFace),
          Id(theId),
          IsValid(true)
    {
    }

    //! Hash value, for Map interface.
    static int HashCode(const MeshElemEdge& theElem, const int theUpper)
    {
        unsigned int aHashCode = ::HashCode(((unsigned)theElem.Face << 8) | (unsigned)theElem.Id, theUpper);
        return int(static_cast<unsigned int>(aHashCode & 0x7fffffff)) % theUpper + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const MeshElemEdge& theElem1, const MeshElemEdge& theElem2)
    {
        return theElem1.Face == theElem2.Face && theElem1.Id == theElem2.Id;
    }
};

//! Hasher for mesh edge (Graphic3d_Vec2u).
class EdgeHasher {
public:
    //! Hash value, for Map interface.
    static int HashCode(const Graphic3d_Vec2u& theElem, const int theUpper)
    {
        unsigned int aHashCode = (unsigned int)::HashCode(theElem.x() + theElem.y(), theUpper);
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const Graphic3d_Vec2u& theElem1, const Graphic3d_Vec2u& theElem2)
    {
        return (theElem1.x() == theElem2.x() && theElem1.y() == theElem2.y())
            || (theElem1.x() == theElem2.y() && theElem1.y() == theElem2.x());
    }
};

//! Struct for storing mesh geometry element.
struct MeshGeomElem {
    RWCae::MeshEntityId Index     = 0;
    RWCae::MeshElemType Type      = RWCae::MeshElemType::Elem0d;
    bool                  IsSurface = false;

    //! Constructor.
    MeshGeomElem() {}

    //! Constructor.
    MeshGeomElem(RWCae::MeshEntityId theIndex, RWCae::MeshElemType theType, bool theIsSurface)
        : Index(theIndex),
          Type(theType),
          IsSurface(theIsSurface)
    {
    }

    //! Hash value, for Map interface.
    static int HashCode(const MeshGeomElem& theElem, const int theUpper)
    {
        unsigned int aHashCode = 0;
        aHashCode ^= ::HashCode(theElem.Index, theUpper);
        aHashCode ^= ::HashCode((int)theElem.Type, theUpper);
        aHashCode ^= ::HashCode(theElem.IsSurface, theUpper);
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const MeshGeomElem& theElem1, const MeshGeomElem& theElem2)
    {
        return theElem1.Index == theElem2.Index && theElem1.Type == theElem2.Type
            && theElem1.IsSurface == theElem2.IsSurface;
    }
};

//! Base class for mesh geometry.
//! Each MeshGeometry has parent/child MeshGeometry so it can be combined into chain of MeshGeometry.
//! Each MeshGeometry in chain can generate a new MeshData based on parent geometries
//! or can store some modifiers for parent geometries (or both).
class MeshGeometry : public Standard_Transient {
public:
    typedef NCollection_DataMap<RWCae::MeshEntityId, std::pair<MeshGeometry*, MeshInterpolatedNode>> NodesMap;
    typedef NCollection_DataMap<MeshGeomElem, std::pair<MeshGeometry*, MeshGeomElem>, MeshGeomElem>    ElementsMap;

public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometry(const Handle(MeshBvhData)& theBvhData);

    //! Return calculated mesh data.
    Standard_EXPORT const Handle(RWCae::MeshData)& GetMeshData();

    //! Set mesh data.
    void SetMeshData(const Handle(RWCae::MeshData)& theMeshData)
    {
        myMeshData  = theMeshData;
        myIsUpdated = true;
    }

    //! Mark calculated mesh data of all mesh geometries in the chain to update.
    Standard_EXPORT void SetToUpdate();

    //! Return id of mesh geometry.
    const TCollection_AsciiString& Id() const { return myId; }

    //! Set id of mesh geometry.
    void SetId(const TCollection_AsciiString& theId) { myId = theId; }

    //! Return BVH data.
    const Handle(MeshBvhData)& GetBvhData() const { return myBvhData; }

    //! Return geometry type.
    virtual MeshGeometryType Type() const = 0;

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) = 0;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) = 0;

public:
    //! Insert a new geometry into children chain based on its order index.
    Standard_EXPORT void Append(const Handle(MeshGeometry)& theChild);

    //! Remove mesh geometry from children chain by its id.
    Standard_EXPORT void Remove(const TCollection_AsciiString& theId);

    //! Remove mesh geometry children.
    Standard_EXPORT void RemoveChildren();

    //! Return child mesh geometry of this.
    Standard_EXPORT const Handle(MeshGeometry)& GetChild() const { return myChildGeometry; }

    //! Return last mesh geometry in chain of children.
    Standard_EXPORT Handle(MeshGeometry) GetLastChild() const;

    //! Return last mesh geometry in chain of children that has 'false' IsModifier flag.
    Standard_EXPORT Handle(MeshGeometry) GetLastNotModifierChild() const;

    //! Return parent mesh geometry of this.
    Standard_EXPORT const Handle(MeshGeometry)& GetParent() const { return myParentGeometry; }

    //! Return first mesh geometry in chain of parents.
    Standard_EXPORT Handle(MeshGeometry) GetFirstParent() const;

    //! Clear auxiliary data.
    Standard_EXPORT void Clear();

    //! Reset geometry id static counter.
    Standard_EXPORT static void ResetStaticCounter();

public:
    //! Return true if this mesh geometry can modify mesh data of parent mesh geometries.
    virtual bool IsModifier() const { return false; }

    //! Apply node modifiers of all children.
    Standard_EXPORT void ModifyNode(const RWCae::MeshEntityId theNodeIdx, Graphic3d_Vec3& theNode) const;

    //! Apply accept modifiers of all children for mesh element.
    Standard_EXPORT bool AcceptElement(const RWCae::MeshEntityId theElemIdx) const;

    //! Apply accept modifiers of all children for mesh node.
    Standard_EXPORT bool AcceptNode(const RWCae::MeshEntityId theNodeIdx, const Graphic3d_Vec3& theNode) const;

public:
    //! Generate a new property from base property for mesh data for this mesh geometry.
    //! If element and nodes are specified, values will be generated only for them.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const = 0;

    //! Generate a new property from base property for mesh data for this mesh geometry.
    //! If element and nodes are specified, values will be generated only for them.
    Standard_EXPORT virtual RWCae::MeshPropertyComponent CalculatePropertyComponent(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        int                                       theCompIndex,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const;

    //! Return map of elements in mesh data of this mesh geometry to elements in mesh data of parent mesh geometry.
    const ElementsMap& GetElementsMap() const { return myElementsMap; }

    //! Return map of elements in mesh data of this mesh geometry to elements in mesh data of parent mesh geometry.
    ElementsMap& ChangeElementsMap() { return myElementsMap; }

    //! Return element in the original mesh data for the specified element in mesh data of this geometry.
    Standard_EXPORT bool OriginElement(const MeshGeomElem& theElem, RWCae::MeshEntityId& theOriginElem) const;

    //! Return map of nodes in mesh data of this mesh geometry to nodes in mesh data of parent mesh geometry.
    const NodesMap& GetNodesMap() const { return myNodesMap; }

    //! Return map of nodes in mesh data of this mesh geometry to nodes in mesh data of parent mesh geometry.
    NodesMap& ChangeNodesMap() { return myNodesMap; }

    //! Return nodes in the original mesh data for the specified node in mesh data of this geometry.
    Standard_EXPORT bool OriginNode(RWCae::MeshEntityId theNode, MeshInterpolatedNode& theOriginNodes) const;

    //! Return order index used while appending to other mesh geometry.
    int OrderIndex() const { return (int)Type(); }

protected:
    //! Called when marked to update from others mesh geometries in the chain.
    virtual void onUpdatedFromOthers() {}

    //! Nullify and mark to update this mesh data.
    virtual void setToUpdate()
    {
        myMeshData.Nullify();
        myIsUpdated = false;
    }

    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() = 0;

    //! Apply node modifier.
    Standard_EXPORT virtual void modifyNode(const RWCae::MeshEntityId theNodeIdx,
                                            const MeshInterpolatedNode* theBaseNodes,
                                            Graphic3d_Vec3&             theNode) const
    {
        (void)theNodeIdx;
        (void)theBaseNodes;
        (void)theNode;
    }

    //! Apply accept element modifier.
    Standard_EXPORT virtual bool acceptElement(const Handle(RWCae::MeshData)& theMeshData,
                                               const RWCae::MeshEntityId      theElemIdx,
                                               const MeshGeometry*              theMeshGeom) const
    {
        (void)theMeshData;
        (void)theElemIdx;
        (void)theMeshGeom;
        return true;
    }

    //! Apply accept node modifier.
    Standard_EXPORT virtual bool acceptNode(const RWCae::MeshEntityId theNodeIdx,
                                            const MeshGeometry*         theMeshGeom,
                                            const Graphic3d_Vec3&       theNode) const
    {
        (void)theNodeIdx;
        (void)theMeshGeom;
        (void)theNode;
        return true;
    }

    //! Clear auxiliary data.
    virtual void clear() { myBvhData->RemoveBvhBoxSet(this); }

    //! Generate a new property from base property.
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) calculateCommonProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem,
        const std::vector<RWCae::MeshEntityId>* theNodes,
        const RWCae::MeshData::EntityMap*       theNodesMap = nullptr) const;

    //! Generate a new property from base property using specified mesh elements.
    Standard_EXPORT void calculateCommonPropertyNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                                      const Handle(RWCae::MeshNVecProperty)&  theProp,
                                                      const std::vector<RWCae::MeshEntityId>* theNodes,
                                                      const RWCae::MeshData::EntityMap*       theNodesMap) const;

    //! Generate a new elemental property from base property using specified mesh elements.
    template<class Elem_T>
    Standard_EXPORT void calculateCommonPropertyElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                          const Handle(RWCae::MeshNVecProperty)& theProp,
                                                          const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                                          const MeshGeomElem*                      theElem) const;

    //! Generate a new scalar nodal property from base property using specified mesh elements.
    Standard_EXPORT void genScalarCommonPropNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                                  const Handle(RWCae::MeshNVecProperty)&  theProp,
                                                  const std::vector<RWCae::MeshEntityId>* theNodes,
                                                  const RWCae::MeshData::EntityMap*       theNodesMap) const;

    //! Calculate nodal value for base scalar property.
    Standard_EXPORT void calculateScalarNodalValue(const Handle(RWCae::MeshScalarPropertyBase)& theBaseComp,
                                                   const Handle(RWCae::MeshScalarPropertyBase)& theComp,
                                                   RWCae::MeshEntityId                          theNodeIdx,
                                                   RWCae::MeshEntityId                          thePropValIdx,
                                                   const RWCae::MeshData::EntityMap*            theNodesMap) const;

    //! Generate a new scalar elemental property from base property using specified mesh elements.
    template<class Elem_T>
    Standard_EXPORT void genScalarCommonPropElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                      const Handle(RWCae::MeshNVecProperty)& theProp,
                                                      const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                                      const MeshGeomElem*                      theElem) const;

    //! Generate a new node vec property from base property using specified mesh elements.
    template<class Elem_T>
    Standard_EXPORT void genNodeVecCommonProp(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                              const Handle(RWCae::MeshNVecProperty)& theProp,
                                              const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                              const MeshGeomElem*                      theElem) const;

    //! Calculate value for base nodevec property.
    template<class Elem_T>
    Standard_EXPORT void calculateNodeVecValue(const Elem_T&                               theElem,
                                               RWCae::MeshEntityId                       theBaseElemIdx,
                                               RWCae::MeshEntityId                       thePropValIdx,
                                               const Handle(RWCae::MeshNodeVecProperty)& theBaseComp,
                                               const Handle(RWCae::MeshNodeVecProperty)& theComp) const;

    //! Generate a new group nodal property from base property using specified mesh elements.
    Standard_EXPORT void genGroupCommonPropNodal(const Handle(RWCae::MeshNVecProperty)&  theBaseProp,
                                                 const Handle(RWCae::MeshNVecProperty)&  theProp,
                                                 const std::vector<RWCae::MeshEntityId>* theNodes,
                                                 const RWCae::MeshData::EntityMap*       theNodesMap) const;

    //! Generate a new group elemental property from base property using specified mesh elements.
    template<class Elem_T>
    Standard_EXPORT void genGroupCommonPropElemental(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                     const Handle(RWCae::MeshNVecProperty)& theProp,
                                                     const RWCae::MeshAdaptor<Elem_T>&      theMesh,
                                                     const MeshGeomElem*                      theElem) const;

    //! Generate a new property for element/nodes (if are specified)
    //! or return the existing one otherwise.
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) genPropValues(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem,
        const std::vector<RWCae::MeshEntityId>* theNodes) const;

    //! Update components of generated property.
    Standard_EXPORT void updateGenPropComponents(const Handle(RWCae::MeshNVecProperty)& theBaseProp,
                                                 const Handle(RWCae::MeshNVecProperty)& theProp) const;

protected:
    Handle(MeshGeometry) myParentGeometry;
    Handle(MeshGeometry) myChildGeometry;

    Handle(RWCae::MeshData) myMeshData;
    bool                      myIsUpdated;

    Handle(MeshBvhData) myBvhData;

    NodesMap    myNodesMap;
    ElementsMap myElementsMap;

    TCollection_AsciiString myId;

    bool myCanModifyNodes = false;
};

typedef NCollection_DataMap<TCollection_AsciiString, Handle(MeshGeometry)> MapOfMeshGeometries;

//! Mesh geometry for initial mesh data.
//! Should be the first mesh geometry in chain.
class MeshGeometryBase : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT MeshGeometryBase(const Handle(MeshBvhData)&       theBvhData,
                                     const Handle(RWCae::MeshData)& theMeshData);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Base; }

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override
    {
        return genPropValues(theProp, theElem, theNodes);
    }

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer&) override {}

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value&,
                                          const Handle(RWCae::MeshData)&,
                                          int,
                                          bool&) override
    {
        return true;
    }

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override {}

    //! Nullify and mark to update this mesh data.
    virtual void setToUpdate() override {}
};

//! Mesh geometry factory.
class MeshGeometryFactory {
public:
    Standard_EXPORT static Handle(MeshGeometry) Create(const Handle(RWCae::MeshData)& theMeshData,
                                                       const Handle(MeshBvhData)&       theBvhData,
                                                       MeshGeometryType                 theType);
};

//! Iterator for mesh geometry children/parents.
template<bool IsReversed>
class MeshGeometryIterator {
public:
    //! Constructor.
    MeshGeometryIterator() {}

    //! Constructor.
    explicit MeshGeometryIterator(MeshGeometry* theGeom) : myGeom(theGeom) {}

    //! Increment operator.
    inline MeshGeometryIterator<IsReversed>& operator++();

    //! Reference operator.
    MeshGeometry* operator*() const { return myGeom; }

    //! Test for iterators equality.
    bool operator==(const MeshGeometryIterator<IsReversed>& theOther) const { return myGeom == theOther.myGeom; }

    //! Test for iterators inequality.
    bool operator!=(const MeshGeometryIterator<IsReversed>& theOther) const { return myGeom != theOther.myGeom; }

    //! Returns an iterator for range-based for loop.
    MeshGeometryIterator<IsReversed> begin() const { return MeshGeometryIterator<IsReversed>(myGeom); }

    //! Returns a child iterator pointing to nothing for range-based for loop.
    MeshGeometryIterator<IsReversed> end() const { return MeshGeometryIterator<IsReversed>(); }

    //! Returns an iterator for range-based for loop.
    MeshGeometryIterator<IsReversed> cbegin() const { return MeshGeometryIterator<IsReversed>(myGeom); }

    //! Returns a child iterator pointing to nothing for range-based for loop.
    MeshGeometryIterator<IsReversed> cend() const { return MeshGeometryIterator<IsReversed>(); }

protected:
    MeshGeometry* myGeom = nullptr;
};

template<>
inline MeshGeometryIterator<false>& MeshGeometryIterator<false>::operator++()
{
    if (myGeom != nullptr) {
        myGeom = myGeom->GetChild().get();
    }
    return *this;
}

template<>
inline MeshGeometryIterator<true>& MeshGeometryIterator<true>::operator++()
{
    if (myGeom != nullptr) {
        myGeom = myGeom->GetParent().get();
    }
    return *this;
}

typedef MeshGeometryIterator<false> MeshGeomChildren;
typedef MeshGeometryIterator<true>  MeshGeomParents;

} // namespace

#endif // _MESHGEOMETRY_HEADERFILE
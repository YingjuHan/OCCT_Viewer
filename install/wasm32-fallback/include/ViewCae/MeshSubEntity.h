#ifndef ViewCAE_MESHSUBENTITY_HEADERFILE
#define ViewCAE_MESHSUBENTITY_HEADERFILE

#include <ViewCae/MeshPresentationBase.h>

#include <ViewCaeGeom/MeshGeometry.h>

#include <gp_Pnt.hxx>

namespace ViewCae {
//! Type of CAE mesh entity.
enum class MeshSubEntityType { Node, Element, Presentation };

//! Class stores information about CAE mesh sub entity (node or element).
//! It contains reference point on entity, corresponding mesh geometry and entity index in it,
//! properties values in reference point.
class MeshSubEntity {
public:
    //! Hash value, for Map interface.
    Standard_EXPORT static int HashCode(const MeshSubEntity& theKey, const int theUpperBound);

    //! Matching two instances, for Map interface.
    Standard_EXPORT static bool IsEqual(const MeshSubEntity& theKey1, const MeshSubEntity& theKey2);

public:
    //! Constructor.
    MeshSubEntity() : myRefMeshPrs(""), myRefMeshGeom(""), myRefMeshData(new RWCae::MeshData()) {}

    //! Init whole presentation entity selected by element.
    Standard_EXPORT void SetPresentationViaElem(const Handle(MeshPresentationBase)&       thePrs,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const Graphic3d_Vec3&                     thePos,
                                                ViewCaeGeom::MeshGeomElem                theElem);

    //! Init element entity.
    Standard_EXPORT void SetElem(const Handle(MeshPresentationBase)&       thePrs,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const Graphic3d_Vec3&                     thePos,
                                 ViewCaeGeom::MeshGeomElem                theElem);

    //! Init whole presentation entity selected by node.
    Standard_EXPORT void SetPresentationViaNode(const Handle(MeshPresentationBase)&       thePrs,
                                                const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                                const Graphic3d_Vec3&                     thePos,
                                                int                                       theIndex);

    //! Init node entity.
    Standard_EXPORT void SetNode(const Handle(MeshPresentationBase)&       thePrs,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const Graphic3d_Vec3&                     thePos,
                                 int                                       theIndex);

    //! Id of corresponding mesh presentation.
    const TCollection_AsciiString& RefMeshPrs() const { return myRefMeshPrs; }

    //! Id of corresponding mesh geometry.
    const TCollection_AsciiString& RefMeshGeom() const { return myRefMeshGeom; }

    //! Reference mesh data that contains information about entity in the next form:
    //! - first node is reserved for reference point
    //! - other nodes and mesh elements are reserved
    //!   for corresponding mesh entity (node/element) in the original mesh data
    //! - properties are computed for the reference point
    const Handle(RWCae::MeshData)& RefMeshData() const { return myRefMeshData; }

    //! Set reference mesh data.
    Standard_EXPORT bool SetRefMeshData(const Handle(RWCae::MeshData)& theMeshData);

    //! Index of entity inside specified mesh geometry data.
    int Index() const { return myIndex; }

    //! Type of entity.
    MeshSubEntityType EntType() const { return myEntType; }

    //! Point position on entity.
    const Graphic3d_Vec3& Pos() const { return myRefMeshData->Nodes()[0]; }

    //! Check if entity is defined.
    bool IsDefined() const { return myIndex != -1 && myRefMeshData->NbNodes() > 0; }

    //! Check if entity is a node.
    bool IsNode() const { return myEntType == MeshSubEntityType::Node; }

    //! Check if entity is an element.
    bool IsElement() const { return myEntType == MeshSubEntityType::Element; }

    //! Check if entity is a presentation.
    bool IsPresentation() const { return myEntType == MeshSubEntityType::Presentation; }

    //! Check if mesh element has surface type.
    bool IsElemSurf() const { return myIsElemSurf; }

    //! Check if original entity is computed.
    bool HasRefElem() const { return myRefMeshData->NbNodes() > 1; }

    //! Return type of mesh element.
    RWCae::MeshElemType ElemType() const { return myElemType; }

    //! Reset this entity.
    void Reset()
    {
        myRefMeshPrs  = "";
        myRefMeshGeom = "";
        myRefMeshData = new RWCae::MeshData();
        myIndex       = -1;
        myElemType    = RWCae::MeshElemType::Elem0d;
        myIsElemSurf  = false;
        myEntType     = MeshSubEntityType::Node;
    }

protected:
    //! Init element entity.
    Standard_EXPORT void setElem(const Handle(MeshPresentationBase)&       thePrs,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const Graphic3d_Vec3&                     thePos,
                                 ViewCaeGeom::MeshGeomElem                theElem);

    //! Init node entity.
    Standard_EXPORT void setNode(const Handle(MeshPresentationBase)&       thePrs,
                                 const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                 const Graphic3d_Vec3&                     thePos,
                                 int                                       theIndex);

    //! Add element to reference mesh data.
    template<class Elem_T>
    Standard_EXPORT void addElement(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom, const Elem_T& theElem);

    //! Compute original mesh element and add it to reference mesh data.
    Standard_EXPORT void addOriginElem(const Handle(MeshPresentationBase)&       thePrs,
                                       const Handle(ViewCaeGeom::MeshGeometry)& theGeom);

    //! Apply inversed deformation transformation to the specified point.
    template<class Elem_T>
    Standard_EXPORT void getPntOnNotDefElem(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                            const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                                            const Elem_T&                             theElem,
                                            const Graphic3d_Vec3&                     thePnt,
                                            Graphic3d_Vec3&                           theIniPnt);

    //! Compute property values in the specified point.
    template<class Elem_T>
    Standard_EXPORT bool computeValueInPoint(const Elem_T&                                               theElem,
                                             const std::vector<float>&                                   theWeights,
                                             const std::vector<Handle(RWCae::MeshScalarPropertyBase)>& theComps,
                                             std::vector<double>&                                        theValues);

    //! Compute properties values in the specified point.
    template<class Elem_T>
    Standard_EXPORT void computeElemPntValues(const Handle(MeshPresentationBase)&       thePrs,
                                              const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                              const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                                              RWCae::MeshEntityId                     theIndex,
                                              const Graphic3d_Vec3&                     thePnt);

    //! Compute original mesh nodes and add it to reference mesh data.
    Standard_EXPORT void addOriginNodes(const Handle(MeshPresentationBase)&       thePrs,
                                        const Handle(ViewCaeGeom::MeshGeometry)& theGeom);

    //! Compute properties values in the specified point.
    Standard_EXPORT void computeNodeValues(const Handle(MeshPresentationBase)&       thePrs,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                           RWCae::MeshEntityId                     theIndex);

    //! Create nodal property with values and add to ref mesh data.
    Standard_EXPORT void addNodalProp(const Handle(RWCae::MeshNVecProperty)& theProp,
                                      const std::vector<double>&               theValues);

    //! Add auxiliary property contains sub entity id.
    Standard_EXPORT void addIdProp();

    //! Parse auxiliary property contains sub entity id.
    Standard_EXPORT void parseIdProp();

protected:
    TCollection_AsciiString myRefMeshPrs;
    TCollection_AsciiString myRefMeshGeom;

    Handle(RWCae::MeshData) myRefMeshData;

    int                   myIndex      = -1;
    RWCae::MeshElemType myElemType   = RWCae::MeshElemType::Elem0d;
    bool                  myIsElemSurf = false;

    MeshSubEntityType myEntType = MeshSubEntityType::Node;
};

} // namespace

#endif // ViewCAE_MESHSUBENTITY_HEADERFILE

#ifndef _MESHGEOMETRYCLIP_HEADERFILE
#define _MESHGEOMETRYCLIP_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_ClipPlane.hxx>

namespace ViewCaeGeom {
//! Mesh geometry for mesh clipping.
//! It clips parent mesh geometries and generates a new mesh data with outer surface on clip plane.
class MeshGeometryClip : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometryClip(const Handle(MeshBvhData)& theBvhData);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Clip; }

    //! Return an active clip plane.
    const Handle(Graphic3d_ClipPlane)& ClipPlane() const { return myClipPlane; }

    //! Set an active clip plane.
    void SetClipPlane(const Handle(Graphic3d_ClipPlane)& theClipPlane)
    {
        myClipPlane = theClipPlane;
        clear();
        SetToUpdate();
    }

    //! Check whether elements should be cut or filtered.
    bool ToCut() const { return myToCut; }

    //! Define whether elements should be cut or filtered.
    void SetCut(bool theToCut)
    {
        myToCut = theToCut;
        clear();
        SetToUpdate();
    }

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

    //! Return true if this mesh geometry can modify mesh data of parent mesh geometries.
    virtual bool IsModifier() const override { return true; }

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override;

    //! Apply accept element modifier.
    Standard_EXPORT virtual bool acceptElement(const Handle(RWCae::MeshData)& theMeshData,
                                               const RWCae::MeshEntityId      theElemIdx,
                                               const MeshGeometry*              theMeshGeom) const override;

    //! Apply accept node modifier.
    Standard_EXPORT virtual bool acceptNode(const RWCae::MeshEntityId theNodeIdx,
                                            const MeshGeometry*         theMeshGeom,
                                            const Graphic3d_Vec3&       theNode) const override;

    //! Clear auxiliary data.
    virtual void clear() override
    {
        MeshGeometry::clear();
        myExtraClippedNodes.Clear();
    }

protected:
    Handle(Graphic3d_ClipPlane)                                           myClipPlane;
    NCollection_DataMap<Handle(MeshGeometry), TColStd_PackedMapOfInteger> myExtraClippedNodes;

    bool myToCut = false;

    bool myMeshDataIsGenerated = false;
};

} // namespace

#endif // _MESHGEOMETRYCLIP_HEADERFILE
 
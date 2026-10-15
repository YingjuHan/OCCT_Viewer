#ifndef _MESHGEOMETRYSLICE_HEADERFILE
#define _MESHGEOMETRYSLICE_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_SequenceOfHClipPlane.hxx>

namespace ViewCaeGeom {
//! Mesh geometry for mesh slice.
//! It generates a new mesh data with 2d elements.
class MeshGeometrySlice : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometrySlice(const Handle(MeshBvhData)& theBvhData);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Slice; }

    //! Return active slice planes.
    const Graphic3d_SequenceOfHClipPlane& SlicePlanes() const { return mySlicePlanes; }

    //! Set active slice planes.
    void SetSlicePlanes(const Graphic3d_SequenceOfHClipPlane& theSlicePlanes)
    {
        mySlicePlanes = theSlicePlanes;
        SetToUpdate();
    }

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override;

protected:
    Graphic3d_SequenceOfHClipPlane mySlicePlanes;
};

} // namespace

#endif // _MESHGEOMETRYSLICE_HEADERFILE
 
#ifndef _MESHGEOMETRYGEN_HEADERFILE
#define _MESHGEOMETRYGEN_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

namespace ViewCaeGeom {
//! Mesh geometry with custom mesh data.
class MeshGeometryGen : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometryGen(const Handle(MeshBvhData)& theBvhData);

    //! Destructor.
    Standard_EXPORT virtual ~MeshGeometryGen();

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Gen; }

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual RWCae::MeshPropertyComponent CalculatePropertyComponent(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        int                                       theCompIndex,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override;

    //! Return true if this mesh geometry can modify mesh data of parent mesh geometries.
    virtual bool IsModifier() const override { return false; }

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

    //! Check if custom mesh data is set.
    bool IsLoaded() const { return !myMeshData.IsNull(); }

protected:
    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override {}

protected:
};

} // namespace

#endif // _MESHGEOMETRYGEN_HEADERFILE

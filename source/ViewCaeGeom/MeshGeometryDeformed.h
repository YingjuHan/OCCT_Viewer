#ifndef _MESHGEOMETRYDEFORMED_HEADERFILE
#define _MESHGEOMETRYDEFORMED_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

namespace ViewCaeGeom {
//! Mesh geometry for mesh deformation.
//! It applies deformation to parent mesh geometries without creation of new mesh data.
class MeshGeometryDeformed : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshGeometryDeformed(const Handle(MeshBvhData)& theBvhData);

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Deformed; }

    //! Return vector property for deformation.
    const Handle(RWCae::MeshNVecProperty)& DeformedProperty() const { return myDeformedProperty; }

    //! Set vector property for deformation.
    void SetDeformedProperty(const Handle(RWCae::MeshNVecProperty)& theProp)
    {
        myDeformedProperty = theProp;
        onDeformedUpdate();
    }

    //! Return scale coefficient for deformation.
    double DeformedScaleCoeff() const { return myDeformedScaleCoeff; }

    //! Set scale coefficient for deformation.
    void SetDeformedScaleCoeff(double theCoeff)
    {
        myDeformedScaleCoeff = theCoeff;
        onDeformedUpdate();
    }

    //! Returns the effective deformation scale.
    double EffectiveScale() const { return myEffectiveScale; }

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override
    {
        return genPropValues(theProp, theElem, theNodes);
    }

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
    Standard_EXPORT virtual void generateMeshData() override {}

    //! Apply node modifier.
    Standard_EXPORT virtual void modifyNode(const RWCae::MeshEntityId theNodeIdx,
                                            const MeshInterpolatedNode* theBaseNodes,
                                            Graphic3d_Vec3&             theNode) const override;

    //! Clear auxiliary data.
    Standard_EXPORT virtual void clear() override;

    //! Called on changing of deformation parameters.
    Standard_EXPORT void onDeformedUpdate();

    //! Calculate the effective deformation scale from the scale factor, mesh AABB and field value range.
    Standard_EXPORT void updateEffectiveScale();

    //! Internal setter for the effective scale, shouldn't be called from the outside.
    void setEffectiveScale(double theScale) { myEffectiveScale = theScale; }

protected:
    Handle(RWCae::MeshNVecProperty) myDeformedProperty;
    double                            myDeformedScaleCoeff = 0.0;
    double                            myEffectiveScale     = 0.0;
};

} // namespace

#endif // _MESHGEOMETRYDEFORMED_HEADERFILE
 
#ifndef _MESHGEOMETRYQUADRIC_HEADERFILE
#define _MESHGEOMETRYQUADRIC_HEADERFILE

#include <ViewCaeGeom/MeshCutViewManager.h>

namespace ViewCaeGeom {

//! OCCT analytic intersections, tessellated for display with source FE provenance.
//! Sides contain boundary triangles, not replacement volume cells.
class MeshGeometryQuadric : public MeshGeometrySlice {
public:
    Standard_EXPORT MeshGeometryQuadric(const Handle(MeshBvhData)& theBvh,
        const MeshCutDefinition& theDefinition, MeshCutPortion thePortion,
        const MeshCutGeometryOptions& theOptions = MeshCutGeometryOptions());
    MeshGeometryType Type() const override { return MeshGeometryType::Slice; }
    Standard_EXPORT void ToJson(RWBase::JsonHelper::Writer&) override;
    bool FromJson(const RWBase::JsonHelper::Value&, const Handle(RWCae::MeshData)&, int, bool&) override
    {
        return false;
    }
    double Volume() const { return myVolume; }
    double Area() const { return myArea; }

protected:
    Standard_EXPORT void generateMeshData() override;

private:
    MeshCutDefinition myDefinition;
    MeshCutPortion myPortion;
    MeshCutGeometryOptions myOptions;
    double myVolume = 0.0;
    double myArea = 0.0;
};

}
#endif

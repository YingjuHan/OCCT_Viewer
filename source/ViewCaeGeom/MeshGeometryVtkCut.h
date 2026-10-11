#ifndef _MESHGEOMETRYVTKCUT_HEADERFILE
#define _MESHGEOMETRYVTKCUT_HEADERFILE
#include <ViewCaeGeom/MeshGeometryQuadric.h>

namespace ViewCaeGeom {
//! Common boundary cut backend for original VTK cells and native non-simplex cells.
class MeshGeometryVtkCut : public MeshGeometrySlice {
public:
    MeshGeometryVtkCut(const Handle(MeshBvhData)& theBvh, const MeshCutDefinition& theDefinition,
        MeshCutPortion thePortion, const std::shared_ptr<MeshVtkCutSource>& theSource, bool theSourceOnly = false)
        : MeshGeometrySlice(theBvh), myDefinition(theDefinition), myPortion(thePortion), mySourceOnly(theSourceOnly), mySource(theSource) {}
    void ToJson(RWBase::JsonHelper::Writer&) override;
    bool FromJson(const RWBase::JsonHelper::Value&, const Handle(RWCae::MeshData)&, int, bool&) override { return false; }
    double Volume() const { return myVolume; }
    double Area() const { return myArea; }
protected:
    void generateMeshData() override;
private:
    MeshCutDefinition myDefinition;
    MeshCutPortion myPortion;
    bool mySourceOnly;
    std::shared_ptr<MeshVtkCutSource> mySource;
    double myVolume = 0.0;
    double myArea = 0.0;
};
}
#endif

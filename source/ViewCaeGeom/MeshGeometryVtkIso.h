#ifndef _MESHGEOMETRYVTKISO_HEADERFILE
#define _MESHGEOMETRYVTKISO_HEADERFILE
#include <ViewCaeGeom/MeshCutViewManager.h>

namespace ViewCaeGeom {
//! Numerical parameter-space scalar cut. Output simplices retain original FE weights.
class MeshGeometryVtkIso : public MeshGeometrySlice {
public:
    MeshGeometryVtkIso(const Handle(MeshBvhData)& bvh, const MeshCutDefinition& definition,
        MeshCutPortion portion, const Handle(RWCae::MeshScalarPropertyBase)& values,
        const MeshCutGeometryOptions& geometry, const MeshCutIsoOptions& options)
        : MeshGeometrySlice(bvh), Definition(definition), Portion(portion), Values(values), Geometry(geometry), Options(options) {}
    void ToJson(RWBase::JsonHelper::Writer&) override;
    bool FromJson(const RWBase::JsonHelper::Value&, const Handle(RWCae::MeshData)&, int, bool&) override { return false; }
    double Volume = 0, Area = 0, FieldTolerance = 0, FieldResidual = 0, GeometryResidual = 0;
    int Depth = 0;
    uint64_t Primitives = 0;
protected:
    void generateMeshData() override;
    bool acceptElement(const Handle(RWCae::MeshData)&, RWCae::MeshEntityId, const MeshGeometry*) const override { return false; }
    bool acceptNode(RWCae::MeshEntityId, const MeshGeometry*, const Graphic3d_Vec3&) const override { return false; }
private:
    MeshCutDefinition Definition;
    MeshCutPortion Portion;
    Handle(RWCae::MeshScalarPropertyBase) Values;
    MeshCutGeometryOptions Geometry;
    MeshCutIsoOptions Options;
};
}
#endif

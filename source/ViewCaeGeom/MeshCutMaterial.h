#ifndef _MESHCUTMATERIAL_HEADERFILE
#define _MESHCUTMATERIAL_HEADERFILE
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutTrace.h>

namespace ViewCaeGeom {
//! Immutable reference intersections shared by coordinated frame candidates.
struct MeshCutMaterialCache {
    std::unique_ptr<MeshCutViewManager> Cuts;
    std::string Key;
    std::array<MeshCutResult,3> Results;
    unsigned Mask = 0;
    uint64_t ReferenceBuilds = 0, MappingBuilds = 0, MappedNodes = 0;
    Standard_EXPORT const MeshCutResult& Get(unsigned portion);
};
Standard_EXPORT bool SameMaterialTopology(const Handle(RWCae::MeshData)& a,
    const Handle(RWCae::MeshData)& b);
//! Maps cached original FE weights; never intersects the current configuration.
class MeshGeometryMaterial : public MeshGeometrySlice {
public:
    Standard_EXPORT MeshGeometryMaterial(const Handle(MeshBvhData)& bvh,
        std::shared_ptr<MeshCutMaterialCache> reference, unsigned portion,
        const Handle(MeshGeometry)& current, MeshCutViewManager& accounting);
    bool IsModifier() const override { return myPortion != 1 && !myBoundary; }
    MeshGeometryType Type() const override { return myPortion == 1 ? MeshGeometryType::Slice : MeshGeometryType::Clip; }
protected:
    Standard_EXPORT void generateMeshData() override;
    Standard_EXPORT bool acceptElement(const Handle(RWCae::MeshData)&, RWCae::MeshEntityId,
        const MeshGeometry*) const override;
    Standard_EXPORT bool acceptNode(RWCae::MeshEntityId, const MeshGeometry*, const Graphic3d_Vec3&) const override;
private:
    std::shared_ptr<MeshCutMaterialCache> myReference;
    unsigned myPortion;
    Handle(MeshGeometry) myCurrent;
    MeshCutViewManager& myAccounting;
    bool myBoundary = false;
};
//! Copies topology in indexed order, including explicit generated boundary faces.
Standard_EXPORT Handle(RWCae::MeshData) CopyCutTopology(const Handle(RWCae::MeshData)& input,
    const std::function<Graphic3d_Vec3(RWCae::MeshEntityId)>& position);
}
#endif

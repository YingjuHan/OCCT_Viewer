#ifndef _MESHCUTUNION_HEADERFILE
#define _MESHCUTUNION_HEADERFILE
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutTrace.h>
namespace ViewCaeGeom {
struct MeshCutUnionPrimitive {
    std::string CutId;
    MeshCutPortion Portion = MeshCutPortion::On;
    RWCae::MeshEntityId SourceElement = 0;
    bool Cap = false;
    int Dimension = 2;
    RWCae::MeshEntityId Primitive = 0;
};
//! Owns generated display geometry separately from every independent numerical cut.
struct MeshCutPresentation {
    std::vector<MeshCutResult> Branches;
    std::vector<MeshCutUnionPrimitive> Provenance;
    double Volume = 0;
    double Area = 0;
    bool Unified = false;
    Standard_EXPORT ~MeshCutPresentation();
    // Roots with parent/child links must be detached when the artifact expires.
    std::vector<Handle(MeshGeometry)> OwnedRoots;
};
Standard_EXPORT std::shared_ptr<MeshCutPresentation> BuildCutPresentationUnion(
    const Handle(RWCae::MeshData)& mesh, const Handle(MeshGeometry)& source,
    const Handle(MeshBvhData)& bvh, const std::vector<MeshCutResult>& cuts,
    const MeshCutGeometryOptions& options, const std::shared_ptr<MeshCutTraceSource>& topology,
    const std::shared_ptr<MeshVtkCutSource>& prepared = nullptr);
}
#endif

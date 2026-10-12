#ifndef _MESHCUTTRACE_HEADERFILE
#define _MESHCUTTRACE_HEADERFILE
#include <ViewCaeGeom/MeshGeometrySlice.h>
#include <ViewCaeGeom/MeshCutTraceOptions.h>
#include <functional>
#include <memory>
#include <map>

namespace ViewCaeGeom {
class MeshVtkCutSource;
struct MeshCutGeometryOptions;
//! One original element's FE trace, using generated section node IDs as keys.
struct MeshCutPrimitiveTrace {
    RWCae::MeshEntityId Element = 0;
    bool Ambiguous = false;
    Graphic3d_Vec3d Interior = Graphic3d_Vec3d(0);
    MeshInterpolatedNode InteriorWeights;
    std::map<RWCae::MeshEntityId,MeshInterpolatedNode> Weights;
};
//! Cached topology and original-FE lookup for one source/deformation epoch.
class MeshCutTraceSource {
public:
    Standard_EXPORT MeshCutTraceSource(const Handle(RWCae::MeshData)& mesh,
        const Handle(MeshGeometry)& source, const MeshCutGeometryOptions& options,
        const std::shared_ptr<MeshVtkCutSource>& prepared = nullptr);
    Standard_EXPORT ~MeshCutTraceSource();
    Standard_EXPORT std::vector<MeshCutPrimitiveTrace> Traces(RWCae::MeshEntityId element,
        const std::vector<MeshInterpolatedNode>& weights, const std::vector<Graphic3d_Vec3d>& positions) const;
private:
    struct Data;
    std::unique_ptr<Data> myData;
};
//! Independent generated section, retaining both traces before owner selection.
class MeshGeometryTraceSection : public MeshGeometrySlice {
public:
    Standard_EXPORT MeshGeometryTraceSection(const Handle(MeshBvhData)& bvh,
        const Handle(MeshGeometrySlice)& section, const Handle(MeshGeometry)& source,
        const std::shared_ptr<MeshCutTraceSource>& topology);
    //! Refresh FE weights/positions while preserving cached fast-slice connectivity.
    Standard_EXPORT void Refresh(const Handle(MeshGeometrySlice)& section,
        const std::shared_ptr<MeshCutTraceSource>& topology);
    const std::vector<std::vector<MeshCutPrimitiveTrace>>& ElementTraces() const { return myElementTraces; }
    const std::vector<std::vector<MeshCutPrimitiveTrace>>& LineTraces() const { return myLineTraces; }
    using SignedValue = std::function<double(const MeshCutPrimitiveTrace&)>;
    Standard_EXPORT void SetDisplaySelection(const MeshCutTraceOptions& options,
        SignedValue signedValue, double tolerance);
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) MapPhysical(const Handle(RWCae::MeshNVecProperty)& field,
        const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance, bool lines = false) const;
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)& field, const MeshGeomElem* element = nullptr,
        const std::vector<RWCae::MeshEntityId>* nodes = nullptr) const override;
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) CalculateLineProperty(
        const Handle(RWCae::MeshNVecProperty)& field) const override;
protected:
    void generateMeshData() override {}
    void setToUpdate() override {}
private:
    Handle(RWCae::MeshNVecProperty) mapTraces(const Handle(RWCae::MeshNVecProperty)& field, bool lines,
        const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance,
        const MeshGeomElem* element = nullptr, const std::vector<RWCae::MeshEntityId>* nodes = nullptr) const;
    const MeshCutPrimitiveTrace& select(const std::vector<MeshCutPrimitiveTrace>& traces,
        const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance) const;
    Handle(RWCae::MeshData) myInput;
    Handle(MeshGeometry) mySource;
    std::vector<std::vector<MeshCutPrimitiveTrace>> myElementTraces, myLineTraces;
    MeshCutTraceOptions myDisplayOptions;
    SignedValue mySignedValue;
    double mySideTolerance = 0;
};
}
#endif

#ifndef _MESHCUTTRACE_HEADERFILE
#define _MESHCUTTRACE_HEADERFILE
#include <ViewCaeGeom/MeshGeometrySlice.h>
#include <ViewCaeGeom/MeshCutTraceOptions.h>
#include <ViewCaeGeom/MeshCutCache.h>
#include <functional>
#include <memory>
#include <map>
#include <algorithm>
#include <stdexcept>

namespace ViewCaeGeom {
class MeshVtkCutSource;
struct MeshCutGeometryOptions;
struct MeshTraceWork {
    double OriginsMs=0,ClassifyMs=0,PrepareMs=0,MapsMs=0,FillMs=0,GeneralMs=0;
};
//! A primitive usually has 2–6 samples. Contiguous storage avoids a tree node
//! allocation per sample; generated node IDs remain the lookup keys.
class MeshCutTraceWeights {
    using Values=std::vector<std::pair<RWCae::MeshEntityId,MeshInterpolatedNode>>;
    Values myValues;
public:
    auto begin() { return myValues.begin(); } auto end() { return myValues.end(); }
    auto begin() const { return myValues.begin(); } auto end() const { return myValues.end(); }
    auto find(RWCae::MeshEntityId id) { return std::find_if(begin(),end(),[&](const auto& v) { return v.first==id; }); }
    auto find(RWCae::MeshEntityId id) const { return std::find_if(begin(),end(),[&](const auto& v) { return v.first==id; }); }
    const MeshInterpolatedNode& at(RWCae::MeshEntityId id) const { auto i=find(id); if(i==end()) { throw std::out_of_range("Missing primitive FE sample"); } return i->second; }
    void reserve(size_t count) { myValues.reserve(count); }
    void emplace(RWCae::MeshEntityId id,MeshInterpolatedNode value) { if(find(id)==end()) { myValues.emplace_back(id,std::move(value)); } }
    void Assign(const std::vector<RWCae::MeshEntityId>& nodes,const MeshGeometry::NodesMap& origins) { myValues.resize(nodes.size()); for(size_t i=0;i<nodes.size();++i) { myValues[i].first=nodes[i]; myValues[i].second=origins.Find(nodes[i]).second; } }
};
//! One original element's FE trace, using generated section node IDs as keys.
struct MeshCutPrimitiveTrace {
    RWCae::MeshEntityId Element = 0;
    bool Ambiguous = false;
    Graphic3d_Vec3d Interior = Graphic3d_Vec3d(0);
    MeshInterpolatedNode InteriorWeights;
    MeshCutTraceWeights Weights;
};
//! Cached topology and original-FE lookup for one source/deformation epoch.
//! Takes a dedicated source chain; detaches its children on destruction.
class MeshCutTraceSource {
public:
    Standard_EXPORT MeshCutTraceSource(const Handle(RWCae::MeshData)& mesh,
        const Handle(MeshGeometry)& source, const MeshCutGeometryOptions& options,
        const std::shared_ptr<MeshVtkCutSource>& prepared = nullptr);
    Standard_EXPORT ~MeshCutTraceSource();
    Standard_EXPORT bool IsCompact() const;
    Standard_EXPORT MeshCutTraceCacheStatistics CacheStatistics() const;
    Standard_EXPORT void PrepareInteriors(const std::vector<RWCae::MeshEntityId>& elements);
    Standard_EXPORT void UpdateInteriorTrace(MeshCutPrimitiveTrace& trace,RWCae::MeshEntityId element,
        const std::vector<RWCae::MeshEntityId>& nodes,const MeshGeometry::NodesMap& origins) const;
    //! Pure per-cell work for verified interior sections. Coordinates must be
    //! from the same deformation epoch, or null for an unmodified source.
    Standard_EXPORT void UpdateInteriorTraceDirect(MeshCutPrimitiveTrace& trace,RWCae::MeshEntityId element,
        const std::vector<RWCae::MeshEntityId>& nodes,const MeshGeometry::NodesMap& origins,
        const std::vector<Graphic3d_Vec3>* coordinates) const;
    Standard_EXPORT std::vector<MeshCutPrimitiveTrace> Traces(RWCae::MeshEntityId element,
        const std::vector<MeshInterpolatedNode>& weights, const std::vector<Graphic3d_Vec3d>& positions,
        const std::vector<RWCae::MeshEntityId>* sampleNodes = nullptr) const;
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
    //! Material mapping retains reference-side provenance, including both interface owners.
    Standard_EXPORT MeshGeometryTraceSection(const Handle(MeshBvhData)& bvh,
        const MeshGeometryTraceSection& reference, const Handle(MeshGeometrySlice)& mapped,
        const Handle(MeshGeometry)& source);
    //! Refresh FE weights/positions while preserving cached fast-slice connectivity.
    Standard_EXPORT void Refresh(const Handle(MeshGeometrySlice)& section,
        const std::shared_ptr<MeshCutTraceSource>& topology);
    const std::vector<std::vector<MeshCutPrimitiveTrace>>& ElementTraces() const { return myElementTraces; }
    const std::vector<std::vector<MeshCutPrimitiveTrace>>& LineTraces() const { return myLineTraces; }
    const MeshTraceWork& TraceWork() const { return myTraceWork; }
    using SignedValue = std::function<double(const MeshCutPrimitiveTrace&)>;
    Standard_EXPORT void SetDisplaySelection(const MeshCutTraceOptions& options,
        SignedValue signedValue, double tolerance);
    Standard_EXPORT Handle(RWCae::MeshNVecProperty) MapPhysical(const Handle(RWCae::MeshNVecProperty)& field,
        const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance, bool lines = false) const;
    Standard_EXPORT const MeshCutPrimitiveTrace& PhysicalProvenance(size_t primitive,
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
    MeshTraceWork myTraceWork;
    //! Reuse per-node FE vector capacity across large plane revisions.
    std::vector<MeshInterpolatedNode> myOriginScratch;
    struct Primitive { RWCae::MeshEntityId Element; std::vector<RWCae::MeshEntityId> Nodes; };
    std::vector<Primitive> myPrimitiveScratch;
};
}
#endif

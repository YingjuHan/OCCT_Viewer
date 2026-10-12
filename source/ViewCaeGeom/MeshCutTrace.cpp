#include <ViewCaeGeom/MeshCutTrace.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include "MeshCutFieldTools.h"
#ifdef OCCTV_USE_VTK
#include <ViewCaeGeom/MeshVtkCutSource.h>
#endif
#include <algorithm>
#include <set>
#include <tuple>
#include <cmath>
#include <stdexcept>

namespace ViewCaeGeom {
bool MeshCutTraceOptions::IsValid(size_t elements) const
{
    return Policy >= MeshCutTracePolicy::Lowest && Policy <= MeshCutTracePolicy::Region
        && (Regions.empty() || Regions.size() == elements)
        && (Policy != MeshCutTracePolicy::Region || Regions.size() == elements);
}
struct MeshCutTraceSource::Data {
    struct Boundary { std::vector<RWCae::MeshEntityId> Nodes; std::vector<RWCae::MeshEntityId> Owners; bool Trusted = false; };
    struct Cell {
        int Dimension = 0;
        Graphic3d_Vec3d Interior = Graphic3d_Vec3d(0);
        MeshInterpolatedNode InteriorWeights;
        std::vector<size_t> Boundaries;
#ifdef OCCTV_USE_VTK
        std::unique_ptr<MeshVtkCell> Owned;
        MeshVtkCell* Geometry = nullptr;
#endif
    };
    Handle(RWCae::MeshData) Mesh;
    Handle(MeshGeometry) Source;
    std::vector<Cell> Cells;
    std::vector<Boundary> Boundaries;
    std::shared_ptr<MeshVtkCutSource> Prepared;
};

MeshCutTraceSource::MeshCutTraceSource(const Handle(RWCae::MeshData)& mesh,
    const Handle(MeshGeometry)& source, const MeshCutGeometryOptions& options,
    const std::shared_ptr<MeshVtkCutSource>& prepared) : myData(new Data)
{
    auto& data = *myData; data.Mesh = mesh; data.Source = source; data.Prepared = prepared;
    data.Cells.resize(mesh->NbIndexedElems());
    using Key = std::pair<int,std::vector<RWCae::MeshEntityId>>;
    std::map<Key,size_t> keys;
#ifdef OCCTV_USE_VTK
    const double tolerance = options.Tolerance(mesh->BoundingBox().IsVoid() ? 0
        : (mesh->BoundingBox().CornerMax().XYZ()-mesh->BoundingBox().CornerMin().XYZ()).Modulus());
#else
    (void)options;
#endif
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        auto& cell = data.Cells[e]; std::vector<std::vector<RWCae::MeshEntityId>> boundaries;
#ifdef OCCTV_USE_VTK
        if (prepared && prepared->Work.PreparedCells == mesh->NbIndexedElems()) { cell.Geometry = prepared->GetCell(e).Geometry.get(); }
        else {
            cell.Owned.reset(new MeshVtkCell(mesh,e,source.get(),tolerance,options,0,false)); cell.Geometry = cell.Owned.get();
        }
        cell.Dimension = cell.Geometry->Dimension(); cell.Interior = cell.Geometry->InteriorPoint();
        if (cell.Dimension == 0) {
            // Empty/polyvertex cells have no FE interior chart or interfaces.
            std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
            for (auto n : nodes) { cell.InteriorWeights.AddBaseNode(n,1.0/nodes.size()); }
        } else { cell.InteriorWeights = cell.Geometry->Origin(cell.Interior); }
        boundaries = cell.Geometry->BoundaryNodes();
#else
        const auto& indexed = mesh->IndexedElements()[e];
        if (indexed.Type == RWCae::MeshElemType::Generic) { throw std::invalid_argument("Generic trace topology requires VTK"); }
        std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
        for (auto n : nodes) { auto p = mesh->Nodes()[n]; source->ModifyNode(n,p); cell.Interior += Graphic3d_Vec3d(p); }
        if (!nodes.empty()) { cell.Interior /= (double)nodes.size(); }
        for (auto n : nodes) { cell.InteriorWeights.AddBaseNode(n,1.0/nodes.size()); }
        cell.Dimension = indexed.Type == RWCae::MeshElemType::Elem0d ? 0 : (int)indexed.Type >= RWCae::MeshElem3dLowerType ? 3 : 2;
        if (cell.Dimension == 3) {
            RWCae::MeshAdaptorsIterator3dElems::Switch(indexed.Type,[&](const auto& adaptor) {
                const auto& element = adaptor.GetElement(indexed.Index);
                for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(element); ++f) {
                    std::vector<RWCae::MeshEntityId> face; RWCae::MeshElemType type;
                    adaptor.FaceNodes(element,f,face,type); boundaries.push_back(std::move(face));
                }
                return true;
            },mesh);
        } else if (cell.Dimension == 2) {
            const int corners = indexed.Type == RWCae::MeshElemType::Triangle2nd ? 3
                : indexed.Type == RWCae::MeshElemType::Quad2nd ? 4 : (int)nodes.size();
            for (int n = 0; n < corners; ++n) {
                std::vector<RWCae::MeshEntityId> edge = {nodes[n],nodes[(n+1)%corners]};
                if ((int)nodes.size() == 2*corners) { edge.push_back(nodes[corners+n]); }
                boundaries.push_back(std::move(edge));
            }
        }
#endif
        for (auto& boundaryNodes : boundaries) {
            std::sort(boundaryNodes.begin(),boundaryNodes.end()); const Key key(cell.Dimension,boundaryNodes);
            auto found = keys.find(key);
            if (found == keys.end()) {
                found = keys.emplace(key,data.Boundaries.size()).first;
                data.Boundaries.push_back({boundaryNodes,{},false});
            }
            data.Boundaries[found->second].Owners.push_back(e); cell.Boundaries.push_back(found->second);
        }
    }
    for (auto& boundary : data.Boundaries) {
        const auto& owners = boundary.Owners;
        if (owners.size() != 2 || owners[0] == owners[1]) { continue; }
        const auto& a = data.Cells[owners[0]]; const auto& b = data.Cells[owners[1]];
        if (a.Dimension == 3) {
#ifdef OCCTV_USE_VTK
            if (prepared && prepared->Work.PreparedCells == mesh->NbIndexedElems()) {
                const auto& faces = prepared->GetCell(owners[0]).Faces;
                for (size_t f = 0; f < a.Geometry->Faces.size(); ++f) {
                    auto nodes = a.Geometry->Faces[f].Nodes; std::sort(nodes.begin(),nodes.end());
                    if (nodes == boundary.Nodes && faces[f].Internal && faces[f].NeighborElement == owners[1]) { boundary.Trusted = true; }
                }
                continue;
            }
#endif
            if (boundary.Nodes.size() < 3) { continue; }
            Graphic3d_Vec3d points[3];
            for (int n = 0; n < 3; ++n) { auto p = mesh->Nodes()[boundary.Nodes[n]]; source->ModifyNode(boundary.Nodes[n],p); points[n] = Graphic3d_Vec3d(p); }
            auto normal = Graphic3d_Vec3d::Cross(points[1]-points[0],points[2]-points[0]);
            if (normal.Modulus() <= 1e-15) { continue; } normal /= normal.Modulus();
            boundary.Trusted = (a.Interior-points[0]).Dot(normal)*(b.Interior-points[0]).Dot(normal) < -1e-14;
        } else {
            auto p = mesh->Nodes()[boundary.Nodes.front()]; source->ModifyNode(boundary.Nodes.front(),p);
            auto x = a.Interior-Graphic3d_Vec3d(p), y = b.Interior-Graphic3d_Vec3d(p);
            if (a.Dimension == 2 && boundary.Nodes.size() >= 2) {
                auto q = mesh->Nodes()[boundary.Nodes.back()]; source->ModifyNode(boundary.Nodes.back(),q);
                auto direction = Graphic3d_Vec3d(q)-Graphic3d_Vec3d(p);
                if (direction.Modulus() <= 1e-15) { continue; } direction /= direction.Modulus();
                x -= direction*x.Dot(direction); y -= direction*y.Dot(direction);
            }
            // Opposite support prevents pairing duplicate cells on the same side.
            boundary.Trusted = x.Dot(y) < -1e-14;
        }
    }
}
MeshCutTraceSource::~MeshCutTraceSource() = default;

std::vector<MeshCutPrimitiveTrace> MeshCutTraceSource::Traces(RWCae::MeshEntityId element,
    const std::vector<MeshInterpolatedNode>& weights, const std::vector<Graphic3d_Vec3d>& positions) const
{
    const auto& data = *myData; const auto& cell = data.Cells.at(element);
    if (weights.size() != positions.size()) { throw std::invalid_argument("Trace sample size mismatch"); }
    std::vector<MeshCutPrimitiveTrace> result(1); result[0].Element = element; result[0].Interior = cell.Interior;
    result[0].InteriorWeights = cell.InteriorWeights;
    for (size_t n = 0; n < weights.size(); ++n) { result[0].Weights.emplace((RWCae::MeshEntityId)n,weights[n]); }
    std::set<RWCae::MeshEntityId> support;
    for (const auto& weight : weights) {
        for (const auto& node : weight.BaseNodes()) { if (std::abs(node.second) > 1e-6) { support.insert(node.first); } }
    }
    if (support.empty()) { return result; }
    for (auto id : cell.Boundaries) {
        const auto& boundary = data.Boundaries[id];
        if (!std::includes(boundary.Nodes.begin(),boundary.Nodes.end(),support.begin(),support.end()) || boundary.Owners.size() <= 1) { continue; }
        if (!boundary.Trusted) { result[0].Ambiguous = true; continue; }
        const auto neighbor = boundary.Owners[boundary.Owners[0] == element ? 1 : 0];
        MeshCutPrimitiveTrace other; other.Element = neighbor; other.Interior = data.Cells[neighbor].Interior;
        other.InteriorWeights = data.Cells[neighbor].InteriorWeights;
        for (size_t n = 0; n < weights.size(); ++n) {
#ifdef OCCTV_USE_VTK
            other.Weights.emplace((RWCae::MeshEntityId)n,data.Cells[neighbor].Geometry->Origin(positions[n]));
#else
            // A native inverse may leave tiny off-face weights. Those nodes do
            // not belong to the neighboring element-node tuple. Restrict the
            // trace to this trusted topology face and preserve its partition.
            double total = 0;
            for (const auto& w : weights[n].BaseNodes()) {
                if (std::binary_search(boundary.Nodes.begin(),boundary.Nodes.end(),w.first)) { total += w.second; }
            }
            if (!std::isfinite(total) || std::abs(total) <= 1e-12) { throw std::runtime_error("Invalid native interface FE partition"); }
            MeshInterpolatedNode mapped;
            for (const auto& w : weights[n].BaseNodes()) {
                if (std::binary_search(boundary.Nodes.begin(),boundary.Nodes.end(),w.first)) { mapped.AddBaseNode(w.first,w.second/total); }
            }
            other.Weights.emplace((RWCae::MeshEntityId)n,mapped);
#endif
        }
        result.push_back(std::move(other)); break;
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b) { return a.Element < b.Element; });
    return result;
}

MeshGeometryTraceSection::MeshGeometryTraceSection(const Handle(MeshBvhData)& bvh,
    const Handle(MeshGeometrySlice)& section, const Handle(MeshGeometry)& source,
    const std::shared_ptr<MeshCutTraceSource>& topology) : MeshGeometrySlice(bvh), mySource(source)
{
    const auto input = section->GetMeshData(); myInput = input; myMeshData = new RWCae::MeshData(); myIsUpdated = true;
    for (const auto& p : input->Nodes()) { myMeshData->AddNodePosition(p); }
    for (RWCae::MeshEntityId n = 0; n < input->NbNodes(); ++n) {
        MeshInterpolatedNode origin; if (!section->OriginNode(n,origin)) { throw std::runtime_error("Section node has no original FE provenance"); }
        myNodesMap.Bind(n,std::make_pair(source.get(),origin));
    }
    using VertexKey = std::vector<std::pair<RWCae::MeshEntityId,int64_t>>;
    using Key = std::tuple<RWCae::MeshEntityId,RWCae::MeshEntityId,std::vector<VertexKey>>;
    std::set<Key> faces, lines;
    auto records = [&](RWCae::MeshEntityId element,const std::vector<RWCae::MeshEntityId>& nodes,std::set<Key>& seen) {
        std::vector<MeshInterpolatedNode> weights; std::vector<Graphic3d_Vec3d> positions;
        for (auto n : nodes) { weights.push_back(myNodesMap.Find(n).second); positions.emplace_back(input->Nodes()[n]); }
        auto traces = topology->Traces(element,weights,positions);
        if (traces.size() == 2) {
            std::vector<VertexKey> vertices;
            for (size_t n = 0; n < nodes.size(); ++n) {
                VertexKey vertex;
                for (const auto& weight : traces[0].Weights.at((RWCae::MeshEntityId)n).BaseNodes()) {
                    if (std::abs(weight.second) > 1e-6) { vertex.push_back({weight.first,(int64_t)std::llround(weight.second*1e7)}); }
                }
                std::sort(vertex.begin(),vertex.end()); vertices.push_back(std::move(vertex));
            }
            std::sort(vertices.begin(),vertices.end());
            if (!seen.emplace(traces[0].Element,traces[1].Element,vertices).second) { traces.clear(); return traces; }
        }
        for (auto& trace : traces) {
            std::map<RWCae::MeshEntityId,MeshInterpolatedNode> mapped;
            for (size_t n = 0; n < nodes.size(); ++n) { mapped.emplace(nodes[n],trace.Weights.at((RWCae::MeshEntityId)n)); }
            trace.Weights = std::move(mapped);
        }
        return traces;
    };
    for (RWCae::MeshEntityId e = 0; e < input->NbIndexedElems(); ++e) {
        const auto& indexed = input->IndexedElements()[e]; RWCae::MeshEntityId origin;
        if (!section->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),origin)) { throw std::runtime_error("Section element has no original provenance"); }
        std::vector<RWCae::MeshEntityId> nodes; input->ElementNodes(e,nodes);
        auto traces = records(origin,nodes,faces); if (traces.empty()) { continue; }
        if (indexed.Type == RWCae::MeshElemType::Elem0d) { myMeshData->AddElement(RWCae::MeshElement0d(nodes.data())); }
        else if (nodes.size() == 3) { myMeshData->AddElement(RWCae::MeshElementTriangle(nodes.data())); }
        else if (nodes.size() == 4) { myMeshData->AddElement(RWCae::MeshElementQuad(nodes.data())); }
        else { myMeshData->AddElement(RWCae::MeshElementPolygon(nodes.data(),(RWCae::MeshElemEntityId)nodes.size())); }
        const auto& added = myMeshData->IndexedElements()[myMeshData->NbIndexedElems()-1];
        const auto& original = source->GetMeshData()->IndexedElements()[traces[0].Element];
        myElementsMap.Bind(MeshGeomElem(added.Index,added.Type,false),std::make_pair(source.get(),MeshGeomElem(original.Index,original.Type,false)));
        myElementTraces.push_back(std::move(traces));
    }
    for (const auto& line : section->Lines()) {
        auto traces = records(line.SourceElement,{line.Nodes[0],line.Nodes[1]},lines); if (traces.empty()) { continue; }
        myLines.push_back({line.Nodes,traces[0].Element}); myLineTraces.push_back(std::move(traces));
    }
}

void MeshGeometryTraceSection::Refresh(const Handle(MeshGeometrySlice)& section,
    const std::shared_ptr<MeshCutTraceSource>& topology)
{
    const auto input = section->GetMeshData();
    bool unchanged = input == myInput && input->NbNodes() == myMeshData->NbNodes();
    for (RWCae::MeshEntityId n = 0; unchanged && n < input->NbNodes(); ++n) {
        unchanged = input->Nodes()[n] == myMeshData->Nodes()[n];
    }
    if (unchanged) { return; }
    MeshGeometryTraceSection next(myBvhData,section,mySource,topology);
    bool reuse = input == myInput && myMeshData->NbNodes() == next.myMeshData->NbNodes()
        && myMeshData->NbIndexedElems() == next.myMeshData->NbIndexedElems() && myLines.size() == next.myLines.size();
    for (RWCae::MeshEntityId e = 0; reuse && e < myMeshData->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> a,b; myMeshData->ElementNodes(e,a); next.myMeshData->ElementNodes(e,b);
        reuse = a == b && myMeshData->IndexedElements()[e].Type == next.myMeshData->IndexedElements()[e].Type;
    }
    for (size_t l = 0; reuse && l < myLines.size(); ++l) { reuse = myLines[l].Nodes == next.myLines[l].Nodes; }
    if (reuse) {
        for (RWCae::MeshEntityId n = 0; n < input->NbNodes(); ++n) { myMeshData->SetNodePosition(n,next.myMeshData->Nodes()[n]); }
    } else { myMeshData = next.myMeshData; }
    myInput = input; myNodesMap = next.myNodesMap; myElementsMap = next.myElementsMap;
    myLines = std::move(next.myLines); myElementTraces = std::move(next.myElementTraces); myLineTraces = std::move(next.myLineTraces);
}

Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::mapTraces(
    const Handle(RWCae::MeshNVecProperty)& field, bool lines, const MeshCutTraceOptions& options,
    const SignedValue& signedValue, double tolerance, const MeshGeomElem* element,
    const std::vector<RWCae::MeshEntityId>* nodes) const
{
    if (field.IsNull() || (lines && myLineTraces.empty()) || myMeshData->NbNodes() == 0) { return nullptr; }
    const auto& records = lines ? myLineTraces : myElementTraces;
    if (!field->IsNodal() && records.empty()) { return nullptr; }
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(field->Name(),field->NbComponents(),field->MapType(),field->Type());
    if (field->HasNodeVecComponents()) { result->Init<RWCae::MeshNodeVecProperty>(); }
    else { result->Init<RWCae::MeshScalarProperty>(); }
    RWCae::MeshEntityId selected = 0;
    if (element && !field->IsNodal()) {
        if (element->IsSurface || myMeshData->NbElems(element->Type) <= element->Index) { return nullptr; }
        selected = myMeshData->Elements(element->Type).GlobalIndex(element->Index);
    }
    const int count = field->IsNodal() ? (nodes ? (int)nodes->size() : (int)myMeshData->NbNodes()) : element ? 1 : (int)records.size();
    for (int c = 0; c < field->NbComponents(); ++c) {
        const auto from = field->GetComponent(c); const auto to = result->GetComponent(c);
        if (field->HasNodeVecComponents()) { Handle(RWCae::MeshNodeVecProperty)::DownCast(to)->Init(count,0,false,false); }
        else { Handle(RWCae::MeshScalarProperty)::DownCast(to)->Init(count); }
        auto value = [&](const MeshCutPrimitiveTrace& trace,const MeshInterpolatedNode& weights) {
            if (!field->IsNodal() && !field->HasNodeVecComponents()) { return from->Value(RWCae::MeshPropertyEntityInfo(trace.Element)); }
            double sum = 0;
            for (const auto& w : weights.BaseNodes()) {
                if (w.second == 0) { continue; }
                sum += w.second*from->Value(RWCae::MeshPropertyEntityInfo(field->IsNodal() ? w.first : trace.Element,
                    field->HasNodeVecComponents() ? (int)w.first : -1));
            }
            return sum;
        };
        if (field->IsNodal()) {
            for (int n = 0; n < count; ++n) {
                const auto sourceNode = nodes ? nodes->at(n) : (RWCae::MeshEntityId)n;
                to->SetValue(RWCae::MeshPropertyEntityInfo(n),value(MeshCutPrimitiveTrace(),myNodesMap.Find(sourceNode).second));
            }
        }
        auto apply = [&](const auto& traces) {
            for (size_t e = 0; e < traces.size(); ++e) {
                if (element && !field->IsNodal() && e != selected) { continue; }
                const auto& trace = select(traces[e],options,signedValue,tolerance);
                for (const auto& sample : trace.Weights) {
                    RWCae::MeshEntityId target = sample.first;
                    if (field->IsNodal() && nodes) {
                        const auto found = std::find(nodes->begin(),nodes->end(),sample.first); if (found == nodes->end()) { continue; }
                        target = (RWCae::MeshEntityId)(found-nodes->begin());
                    }
                    to->SetValue(RWCae::MeshPropertyEntityInfo(field->IsNodal() ? target : element ? 0 : (RWCae::MeshEntityId)e,
                        field->HasNodeVecComponents() ? (int)sample.first : -1),value(trace,sample.second));
                }
            }
        };
        apply(records); if (field->IsNodal() && !lines) { apply(myLineTraces); }
        CutFieldTools::CopyMetadata(from,to);
    }
    result->SetDescription(field->Description()); result->SetTime(field->Time(),field->TimeStep());
    result->SetTimeStepDescription(field->TimeStepDescription()); result->SetTimeDisplayUnits(field->TimeDisplayUnits());
    updateGenPropComponents(field,result); return result;
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)& field, const MeshGeomElem* element, const std::vector<RWCae::MeshEntityId>* nodes) const
{
    return mapTraces(field,false,myDisplayOptions,mySignedValue,mySideTolerance,element,nodes);
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::CalculateLineProperty(const Handle(RWCae::MeshNVecProperty)& field) const
{
    return mapTraces(field,true,myDisplayOptions,mySignedValue,mySideTolerance);
}

const MeshCutPrimitiveTrace& MeshGeometryTraceSection::select(const std::vector<MeshCutPrimitiveTrace>& traces,
    const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance) const
{
    if (traces.empty()) { throw std::invalid_argument("Section primitive has no trace"); }
    if (options.Policy == MeshCutTracePolicy::Lowest) { return traces.front(); }
    for (const auto& trace : traces) {
        if (trace.Ambiguous) { throw std::invalid_argument("Trace selection requires a trusted two-sided interface"); }
    }
    if (traces.size() == 1) { return traces.front(); }
    const MeshCutPrimitiveTrace* chosen = nullptr;
    for (const auto& trace : traces) {
        bool matches = false;
        if (options.Policy == MeshCutTracePolicy::Region) { matches = options.Regions.at(trace.Element) == options.Region; }
        else {
            const double value = signedValue(trace);
            if (!std::isfinite(value) || std::abs(value) <= tolerance) { throw std::invalid_argument("Interface side is ambiguous at the source interior"); }
            matches = options.Policy == MeshCutTracePolicy::Negative ? value < 0 : value > 0;
        }
        if (matches) {
            if (chosen) { throw std::invalid_argument("Trace policy matches more than one interface source"); }
            chosen = &trace;
        }
    }
    if (!chosen) { throw std::invalid_argument("Trace policy matches no interface source"); }
    return *chosen;
}
void MeshGeometryTraceSection::SetDisplaySelection(const MeshCutTraceOptions& options,
    SignedValue signedValue, double tolerance)
{
    for (const auto& traces : myElementTraces) { select(traces,options,signedValue,tolerance); }
    for (const auto& traces : myLineTraces) { select(traces,options,signedValue,tolerance); }
    myDisplayOptions = options; mySignedValue = std::move(signedValue); mySideTolerance = tolerance;
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::MapPhysical(const Handle(RWCae::MeshNVecProperty)& field,
    const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance, bool lines) const
{
    for (const auto& traces : lines ? myLineTraces : myElementTraces) { select(traces,options,signedValue,tolerance); }
    return mapTraces(field,lines,options,signedValue,tolerance);
}
}

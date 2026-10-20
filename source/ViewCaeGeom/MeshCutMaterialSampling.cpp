#include <ViewCaeGeom/MeshCutMaterial.h>
#include <ViewCaeGeom/MeshVtkCell.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>
#include <stdexcept>

namespace ViewCaeGeom {
namespace {
using Vec = Graphic3d_Vec3d;
using MaterialId = RWCae::MeshEntityId;
Vec unit(const Vec& v)
{
    const double length = v.Modulus();
    if (!std::isfinite(length) || length <= 0) { throw std::invalid_argument("Degenerate material section direction"); }
    return v/length;
}
void finite(const Vec& p)
{
    for (int a = 0; a < 3; ++a) { if (!std::isfinite(p[a])) { throw std::invalid_argument("Nonfinite material displacement/position"); } }
}
void checkField(const MeshCutViewManager& manager, MaterialId node)
{
    const auto& field = manager.DeformationField();
    if (field.IsNull() || manager.DeformationScale() == 0) { return; }
    Vec value; field->GetVector(RWCae::MeshPropertyEntityInfo(node),value); finite(value);
}
//! One intersected FE cell. Affine nodal maps avoid inverse charts on large linear meshes.
struct Cell {
    Handle(MeshGeometry) Reference, Current;
    Handle(RWCae::MeshData) Mesh;
    MaterialId Element;
    int Dimension = 0;
    bool Affine = true;
    Vec Origin, MappedOrigin, Basis[3], MappedBasis[3], Ortho[3];
    std::unique_ptr<MeshVtkCell> Chart;
    MeshCutGeometryOptions Options;
    double Tolerance;
    Cell(const MeshCutResult& reference, const Handle(MeshGeometry)& current,
        const MeshCutViewManager& currentManager, const MeshCutViewManager& referenceManager,
        MaterialId element, double tolerance)
        : Reference(reference.Roots[1]), Current(current), Mesh(Reference->GetMeshData()), Element(element),
          Options(currentManager.GeometryOptions()), Tolerance(tolerance)
    {
        const auto& e = Mesh->IndexedElements()[element];
        if (e.Type == RWCae::MeshElemType::Generic) {
            const auto kernel = MeshCellKernel::InstantiateCell(Mesh->GenericCells().at(e.Index).Type);
            if (!kernel) { throw std::invalid_argument("Material cell has no dimension adapter"); }
            Dimension = kernel->GetCellDimension();
        } else { Dimension = e.Type == RWCae::MeshElemType::Elem0d ? 0 : (int)e.Type >= RWCae::MeshElem3dLowerType ? 3 : 2; }
        std::vector<MaterialId> nodes; Mesh->ElementNodes(element,nodes);
        std::vector<Vec> ref, cur; ref.reserve(nodes.size()); cur.reserve(nodes.size());
        double scale = 0;
        for (MaterialId n : nodes) {
            checkField(currentManager,n); checkField(referenceManager,n);
            auto a = Mesh->Nodes()[n], b = Current->GetMeshData()->Nodes()[n];
            Reference->ModifyNode(n,a); Current->ModifyNode(n,b);
            ref.emplace_back(a); cur.emplace_back(b); finite(ref.back()); finite(cur.back());
            if (ref.size() > 1) { scale = std::max(scale,(ref.back()-ref.front()).Modulus()); }
        }
        if (ref.empty()) { throw std::invalid_argument("Empty material cell"); }
        Origin = ref.front(); MappedOrigin = cur.front();
        int rank = 0;
        for (size_t n = 1; n < ref.size() && rank < Dimension; ++n) {
            auto v = ref[n]-Origin;
            for (int a = 0; a < rank; ++a) { v -= Ortho[a]*v.Dot(Ortho[a]); }
            if (v.Modulus() <= scale*1e-9) { continue; }
            Basis[rank] = ref[n]-Origin; MappedBasis[rank] = cur[n]-MappedOrigin;
            Ortho[rank++] = unit(v);
        }
        if (rank != Dimension) { throw std::invalid_argument("Degenerate reference material cell "+std::to_string(element)); }
        validate(Basis,MappedBasis);
        // With an affine nodal map FE partition of unity reproduces that map.
        // A strict residual also absorbs float-coordinate storage roundoff.
        for (size_t n = 0; n < ref.size(); ++n) {
            if ((mapAffine(ref[n])-cur[n]).Modulus() > tolerance/128) { Affine = false; break; }
        }
        if (!Affine) {
            Chart.reset(new MeshVtkCell(Mesh,element,Reference.get(),tolerance,Options,0,false));
            validateAt(Chart->InteriorPoint());
        }
    }
    Vec mapAffine(const Vec& p) const
    {
        double c[3] = {0,0,0}; Vec result = MappedOrigin;
        for (int a = Dimension-1; a >= 0; --a) {
            double rhs = (p-Origin).Dot(Ortho[a]);
            for (int b = a+1; b < Dimension; ++b) { rhs -= c[b]*Basis[b].Dot(Ortho[a]); }
            c[a] = rhs/Basis[a].Dot(Ortho[a]); result += MappedBasis[a]*c[a];
        }
        return result;
    }
    void validate(const Vec* a,const Vec* b) const
    {
        auto measure = [&](const Vec* x) {
            if (!Dimension) { return 1.; }
            if (Dimension == 1) { return x[0].Modulus(); }
            if (Dimension == 2) { return Vec::Cross(x[0],x[1]).Modulus(); }
            return x[0].Dot(Vec::Cross(x[1],x[2]));
        };
        double sa = 1, sb = 1;
        for (int d = 0; d < Dimension; ++d) { finite(a[d]); finite(b[d]); sa *= a[d].Modulus(); sb *= b[d].Modulus(); }
        const auto ra = measure(a), rb = measure(b);
        if (!std::isfinite(sa) || !std::isfinite(sb) || sa <= 0 || sb <= 0
            || std::abs(ra) <= sa*1e-9 || std::abs(rb) <= sb*1e-9 || (Dimension == 3 && std::signbit(ra) != std::signbit(rb))) {
            throw std::invalid_argument("Degenerate or inverted material cell "+std::to_string(Element));
        }
    }
    void validateAt(const Vec& p) const
    {
        if (Affine) { return; }
        const auto a = Chart->Differential(p), b = Chart->Differential(p,Current.get()); validate(a.data(),b.data());
    }
    MeshInterpolatedNode weights(const Vec& p)
    {
        if (!Chart) { Chart.reset(new MeshVtkCell(Mesh,Element,Reference.get(),Tolerance,Options,0,false)); }
        return Chart->Origin(p);
    }
    Vec map(const MeshInterpolatedNode& weights) const
    {
        Vec result(0);
        for (const auto& w : weights.BaseNodes()) {
            auto p = Current->GetMeshData()->Nodes()[w.first]; Current->ModifyNode(w.first,p); result += Vec(p)*w.second;
        }
        finite(result); return result;
    }
    Vec sample(const Vec& p)
    {
        if (Affine) { return mapAffine(p); }
        validateAt(p); return map(weights(p));
    }
    Vec coNormal(const Vec& p,const Vec& referenceNormal)
    {
        auto a = std::array<Vec,3>{{Basis[0],Basis[1],Basis[2]}}, b = std::array<Vec,3>{{MappedBasis[0],MappedBasis[1],MappedBasis[2]}};
        if (!Affine) { a = Chart->Differential(p); b = Chart->Differential(p,Current.get()); }
        if (Dimension == 3) {
            const Vec c(a[0].Dot(referenceNormal),a[1].Dot(referenceNormal),a[2].Dot(referenceNormal));
            const double det = b[0].Dot(Vec::Cross(b[1],b[2]));
            return unit((Vec::Cross(b[1],b[2])*c.x()+Vec::Cross(b[2],b[0])*c.y()+Vec::Cross(b[0],b[1])*c.z())/det);
        }
        if (Dimension == 2) {
            const double x = a[0].Dot(referenceNormal), y = a[1].Dot(referenceNormal);
            const double aa = b[0].Dot(b[0]), ab = b[0].Dot(b[1]), bb = b[1].Dot(b[1]), det = aa*bb-ab*ab;
            return unit(b[0]*((bb*x-ab*y)/det)+b[1]*((aa*y-ab*x)/det));
        }
        return Dimension == 1 ? unit(b[0])*(a[0].Dot(referenceNormal) < 0 ? -1 : 1) : referenceNormal;
    }
};
struct Triangle { std::array<MaterialId,3> Nodes; size_t Parent; };
struct Line { std::array<MaterialId,2> Nodes; size_t Parent; };
using Edge = std::pair<MaterialId,MaterialId>;
Edge edge(MaterialId a,MaterialId b) { return {std::min(a,b),std::max(a,b)}; }
}

void MeshGeometryMaterial::refineSection(const MeshCutResult& reference)
{
    const auto traced = Handle(MeshGeometryTraceSection)::DownCast(reference.Portions[1]);
    if (traced.IsNull()) { throw std::invalid_argument("Material section requires reference FE traces"); }
    const auto input = reference.Portions[1]->GetMeshData(); const auto& options = myAccounting.GeometryOptions();
    const auto box = input->BoundingBox();
    Tolerance = options.Tolerance(box.IsVoid() ? 0 : (box.CornerMax().XYZ()-box.CornerMin().XYZ()).Modulus());
    // Charge sparse scratch/charts before allocating. No current full-source node array or BVH.
    mySamplingLeases.push_back(myAccounting.ReserveDisplayBytes(65536+(uint64_t)input->NbNodes()*768+(uint64_t)(input->NbIndexedElems()+traced->Lines().size())*2048));
    const Vec normal(reference.Definition.Plane.x(),reference.Definition.Plane.y(),reference.Definition.Plane.z());
    std::unordered_map<MaterialId,std::unique_ptr<Cell>> cells;
    auto cell = [&](MaterialId owner) -> Cell& {
        auto found = cells.find(owner);
        if (found == cells.end()) { found = cells.emplace(owner,std::make_unique<Cell>(reference,myCurrent,myAccounting,*myReference->Cuts,owner,Tolerance)).first; }
        return *found->second;
    };
    std::vector<Vec> positions; positions.reserve(input->NbNodes());
    for (const auto& p : input->Nodes()) { positions.emplace_back(p); }
    std::vector<Triangle> triangles; std::vector<Line> lines;
    for (MaterialId e = 0; e < input->NbIndexedElems(); ++e) {
        for (const auto& trace : traced->ElementTraces().at(e)) { cell(trace.Element); }
        std::vector<MaterialId> nodes; input->ElementNodes(e,nodes);
        if (nodes.size() == 1) { continue; }
        for (size_t i = 1; i+1 < nodes.size(); ++i) { triangles.push_back({{{nodes[0],nodes[i],nodes[i+1]}},e}); }
    }
    for (size_t l = 0; l < traced->Lines().size(); ++l) {
        for (const auto& trace : traced->LineTraces().at(l)) { cell(trace.Element); }
        lines.push_back({traced->Lines()[l].Nodes,l});
    }
    auto owner = [&](size_t p,bool line) { return (line ? traced->LineTraces() : traced->ElementTraces()).at(p).front().Element; };
    auto cancelled = [&]() { if (myCancellation && myCancellation->Cancelled.load()) { throw std::runtime_error("Material sampling cancelled"); } };
    const int limit = std::max(options.FaceSubdivisionLimit,options.LineSubdivisionLimit);
    for (int iteration = 0; iteration <= limit; ++iteration) {
        cancelled(); std::map<Edge,MaterialId> splits; double residual = 0;
        auto request = [&](MaterialId a,MaterialId b) { splits.emplace(edge(a,b),0); };
        auto error = [&](Cell& c,MaterialId a,MaterialId b) {
            const auto p = (positions[a]+positions[b])/2;
            return (c.sample(p)-(Vec(myMeshData->Nodes()[a])+Vec(myMeshData->Nodes()[b]))/2).Modulus();
        };
        for (const auto& t : triangles) {
            auto& c = cell(owner(t.Parent,false)); if (c.Affine) { continue; }
            double local = 0;
            for (int a = 0; a < 3; ++a) {
                const MaterialId x = t.Nodes[a], y = t.Nodes[(a+1)%3]; const double e = error(c,x,y); local = std::max(local,e);
                if (e > Tolerance) { request(x,y); }
            }
            const auto center = (positions[t.Nodes[0]]+positions[t.Nodes[1]]+positions[t.Nodes[2]])/3;
            const auto mapped = (Vec(myMeshData->Nodes()[t.Nodes[0]])+Vec(myMeshData->Nodes()[t.Nodes[1]])+Vec(myMeshData->Nodes()[t.Nodes[2]]))/3;
            const double e = (c.sample(center)-mapped).Modulus(); local = std::max(local,e); residual = std::max(residual,local);
            if (e > Tolerance) { for (int a = 0; a < 3; ++a) { request(t.Nodes[a],t.Nodes[(a+1)%3]); } }
            if (local > Tolerance && iteration >= options.FaceSubdivisionLimit) { throw std::runtime_error("Material face subdivision limit reached before tolerance"); }
        }
        for (const auto& l : lines) {
            auto& c = cell(owner(l.Parent,true)); if (c.Affine) { continue; }
            const double e = error(c,l.Nodes[0],l.Nodes[1]); residual = std::max(residual,e);
            if (e > Tolerance) {
                if (iteration >= options.LineSubdivisionLimit) { throw std::runtime_error("Material line subdivision limit reached before tolerance"); }
                request(l.Nodes[0],l.Nodes[1]);
            }
        }
        if (splits.empty()) { Residual = residual; break; }
        myRefined = true; Depth = iteration+1;
        uint64_t primitives = lines.size();
        for (const auto& t : triangles) {
            unsigned count = 0;
            for (int a = 0; a < 3; ++a) { count += splits.count(edge(t.Nodes[a],t.Nodes[(a+1)%3])) ? 1 : 0; }
            primitives += count+1;
        }
        for (const auto& l : lines) { primitives += splits.count(edge(l.Nodes[0],l.Nodes[1])) ? 1 : 0; }
        if (primitives > options.MaxSourcePrimitives) { throw std::runtime_error("Material sampling primitive budget exceeded"); }
        mySamplingLeases.push_back(myAccounting.ReserveDisplayBytes((uint64_t)splits.size()*2048+(uint64_t)(triangles.size()+lines.size())*512));
        auto midpoint = [&](MaterialId a,MaterialId b,MaterialId element) -> MaterialId {
            auto& id = splits.at(edge(a,b)); if (id) { return id; }
            auto& c = cell(element); const auto p = (positions[a]+positions[b])/2;
            auto weights = c.weights(p); const Vec mapped = c.map(weights);
            id = myMeshData->AddNodePosition(Graphic3d_Vec3(mapped)); positions.push_back(p);
            myNodesMap.Bind(id,std::make_pair(myCurrent.get(),std::move(weights))); return id;
        };
        std::vector<Triangle> refined; refined.reserve(triangles.size()*2);
        auto add = [&](MaterialId a,MaterialId b,MaterialId c,size_t parent) { refined.push_back({{{a,b,c}},parent}); };
        for (const auto& t : triangles) {
            MaterialId m[3] = {0,0,0}; unsigned mask = 0;
            for (int a = 0; a < 3; ++a) { if (splits.count(edge(t.Nodes[a],t.Nodes[(a+1)%3]))) { mask |= 1u<<a; m[a] = midpoint(t.Nodes[a],t.Nodes[(a+1)%3],owner(t.Parent,false)); } }
            if (!mask) { refined.push_back(t); continue; }
            const auto& v = t.Nodes; const auto p = t.Parent;
            if (mask == 7) { add(v[0],m[0],m[2],p); add(m[0],v[1],m[1],p); add(m[2],m[1],v[2],p); add(m[0],m[1],m[2],p); }
            else if (mask == 1 || mask == 2 || mask == 4) {
                const int a = mask == 1 ? 0 : mask == 2 ? 1 : 2;
                add(v[a],m[a],v[(a+2)%3],p); add(m[a],v[(a+1)%3],v[(a+2)%3],p);
            } else {
                const int a = mask == 3 ? 0 : mask == 6 ? 1 : 2; const int b = (a+1)%3, c = (a+2)%3;
                add(v[b],m[b],m[a],p); add(v[a],m[a],v[c],p); add(m[a],m[b],v[c],p);
            }
        }
        triangles.swap(refined);
        std::vector<Line> refinedLines;
        for (const auto& l : lines) {
            if (!splits.count(edge(l.Nodes[0],l.Nodes[1]))) { refinedLines.push_back(l); continue; }
            const auto m = midpoint(l.Nodes[0],l.Nodes[1],owner(l.Parent,true));
            refinedLines.push_back({{{l.Nodes[0],m}},l.Parent}); refinedLines.push_back({{{m,l.Nodes[1]}},l.Parent});
        }
        lines.swap(refinedLines);
    }
    if (myRefined) {
        Handle(RWCae::MeshData) output = new RWCae::MeshData();
        for (const auto& p : myMeshData->Nodes()) { output->AddNodePosition(p); }
        ElementsMap elements;
        auto traces = [&](size_t parent,const std::vector<MaterialId>& nodes,bool line) {
            auto records = (line ? traced->LineTraces() : traced->ElementTraces()).at(parent);
            for (auto& record : records) {
                const auto previous = std::move(record.Weights); record.Weights = MeshCutTraceWeights(); record.Weights.reserve(nodes.size());
                for (MaterialId n : nodes) {
                    const auto old = previous.find(n);
                    record.Weights.emplace(n,old != previous.end() ? old->second : cell(record.Element).weights(positions[n]));
                }
            }
            return records;
        };
        for (const auto& t : triangles) {
            const auto local = output->NbElems(RWCae::MeshElemType::Triangle); output->AddElement(RWCae::MeshElementTriangle(t.Nodes.data()));
            const auto e = owner(t.Parent,false); const auto& original = myCurrent->GetMeshData()->IndexedElements()[e];
            elements.Bind(MeshGeomElem(local,RWCae::MeshElemType::Triangle,false),std::make_pair(myCurrent.get(),MeshGeomElem(original.Index,original.Type,false)));
            myElementTraces.push_back(traces(t.Parent,{t.Nodes.begin(),t.Nodes.end()},false));
        }
        // Preserve isolated beam/point crossings when surface primitives coexist.
        for (MaterialId e = 0; e < input->NbIndexedElems(); ++e) {
            std::vector<MaterialId> nodes; input->ElementNodes(e,nodes); if (nodes.size() != 1) { continue; }
            const auto local = output->NbElems(RWCae::MeshElemType::Elem0d); output->AddElement(RWCae::MeshElement0d(nodes.data()));
            const auto& original = myCurrent->GetMeshData()->IndexedElements()[owner(e,false)];
            elements.Bind(MeshGeomElem(local,RWCae::MeshElemType::Elem0d,false),std::make_pair(myCurrent.get(),MeshGeomElem(original.Index,original.Type,false)));
            myElementTraces.push_back(traces(e,nodes,false));
        }
        myLines.clear();
        for (const auto& l : lines) { myLines.push_back({l.Nodes,owner(l.Parent,true)}); myLineTraces.push_back(traces(l.Parent,{l.Nodes.begin(),l.Nodes.end()},true)); }
        myMeshData = output; myElementsMap.Exchange(elements);
    }
    // Normals use current facet geometry and reference orientation; no world-plane fallback.
    myNormals.resize(myMeshData->NbIndexedElems(),normal);
    for (MaterialId e = 0; e < myMeshData->NbIndexedElems(); ++e) {
        std::vector<MaterialId> nodes; myMeshData->ElementNodes(e,nodes); if (nodes.size() < 3) { continue; }
        const auto a = nodes[0], b = nodes[1], c = nodes[2];
        const auto ref = Vec::Cross(positions[b]-positions[a],positions[c]-positions[a]);
        const auto cur = Vec::Cross(Vec(myMeshData->Nodes()[b])-Vec(myMeshData->Nodes()[a]),Vec(myMeshData->Nodes()[c])-Vec(myMeshData->Nodes()[a]));
        myNormals[e] = unit(cur)*(ref.Dot(normal) < 0 ? -1 : 1);
    }
    for (size_t l = 0; l < myLines.size(); ++l) {
        const auto& line = myLines[l]; const auto p = (positions[line.Nodes[0]]+positions[line.Nodes[1]])/2;
        const auto e = myRefined ? myLineTraces[l].front().Element : owner(l,true);
        myLineNormals.push_back(cell(e).coNormal(p,normal));
    }
}
}

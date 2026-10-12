#include <ViewCaeGeom/MeshGeometryVtkIso.h>
#include <ViewCaeGeom/MeshVtkCell.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace {
using Vec = Graphic3d_Vec3d;
using Simplex = std::array<Vec,4>;
using Key = std::array<double,3>;
Key key(const Vec& q) { return {{q.x(),q.y(),q.z()}}; }
struct Sample { Vec Q, P; ViewCaeGeom::MeshInterpolatedNode Origin; double F; };
double determinant(const Vec& a, const Vec& b, const Vec& c) { return a.Dot(Vec::Cross(b,c)); }
std::vector<Simplex> subdivide(const std::vector<Simplex>& cells, int dimension, uint64_t budget)
{
    const size_t factor = dimension == 3 ? 8 : dimension == 2 ? 4 : 2;
    if (cells.size() > budget/factor) { throw std::runtime_error("Isosurface refinement primitive budget exceeded"); }
    std::vector<Simplex> output; output.reserve(cells.size()*factor);
    for (auto s : cells) {
        if (dimension == 3) {
            // Split the central octahedron along its shortest parameter diagonal.
            // A fixed diagonal can grow aspect ratios under repeated refinement.
            const double abcd = (s[0]+s[1]-s[2]-s[3]).SquareModulus();
            const double acbd = (s[0]+s[2]-s[1]-s[3]).SquareModulus();
            const double adbc = (s[0]+s[3]-s[1]-s[2]).SquareModulus();
            if (acbd < abcd && acbd <= adbc) { std::swap(s[1],s[2]); }
            else if (adbc < abcd) { std::swap(s[1],s[3]); }
        }
        const Vec ab = (s[0]+s[1])*0.5;
        if (dimension == 1) { output.push_back({{s[0],ab}}); output.push_back({{ab,s[1]}}); continue; }
        const Vec ac = (s[0]+s[2])*0.5, bc = (s[1]+s[2])*0.5;
        if (dimension == 2) {
            output.push_back({{s[0],ab,ac}}); output.push_back({{ab,s[1],bc}});
            output.push_back({{ac,bc,s[2]}}); output.push_back({{ab,bc,ac}}); continue;
        }
        const Vec ad = (s[0]+s[3])*0.5, bd = (s[1]+s[3])*0.5, cd = (s[2]+s[3])*0.5;
        output.push_back({{s[0],ab,ac,ad}}); output.push_back({{ab,s[1],bc,bd}});
        output.push_back({{ac,bc,s[2],cd}}); output.push_back({{ad,bd,cd,s[3]}});
        output.push_back({{ab,ac,ad,cd}}); output.push_back({{ab,ac,bc,cd}});
        output.push_back({{ab,bc,bd,cd}}); output.push_back({{ab,ad,bd,cd}});
    }
    return output;
}
void unique(std::vector<Sample>& points)
{
    std::vector<Sample> output;
    for (const auto& p : points) {
        if (std::none_of(output.begin(),output.end(),[&](const Sample& q) { return (p.Q-q.Q).Modulus() < 1e-12; })) { output.push_back(p); }
    }
    points = std::move(output);
}
void order(std::vector<Sample>& points)
{
    unique(points); if (points.size() < 3) { return; }
    Vec center(0); for (const auto& p : points) { center += p.P; } center /= (double)points.size();
    Vec normal = Vec::Cross(points[1].P-points[0].P,points[2].P-points[0].P);
    if (normal.Modulus() < 1e-20) { return; }
    normal /= normal.Modulus(); Vec x = points[0].P-center; x /= x.Modulus(); const Vec y = Vec::Cross(normal,x);
    std::sort(points.begin(),points.end(),[&](const Sample& a, const Sample& b) {
        return std::atan2((a.P-center).Dot(y),(a.P-center).Dot(x)) < std::atan2((b.P-center).Dot(y),(b.P-center).Dot(x));
    });
}
}

namespace ViewCaeGeom {
void MeshGeometryVtkIso::ToJson(RWBase::JsonHelper::Writer&)
{
    throw std::logic_error("Isosurface geometry must be saved through MeshCutViewManager");
}

void MeshGeometryVtkIso::generateMeshData()
{
    // Publish only after all source cells and all budgets have passed.
    Handle(RWCae::MeshData) output = new RWCae::MeshData(); NodesMap nodesMap; ElementsMap elementsMap;
    std::vector<MeshSliceLine> lines;
    const auto source = myParentGeometry->GetFirstParent(); const auto mesh = source->GetMeshData();
    double volume = 0, area = 0, residual = 0, geometryResidual = 0, fieldTolerance = 0;
    uint64_t primitives = 0; int finalDepth = 0;
    for (RWCae::MeshEntityId index = 0; index < mesh->NbIndexedElems(); ++index) {
        MeshVtkCell cell(mesh,index,source.get(),1e-6,Geometry,0,false);
        const int dimension = cell.Dimension();
        if (dimension < 1 || dimension > 3) { throw std::invalid_argument("Isosurface requires a curve, surface or volume chart"); }
        auto simplices = cell.ParameterSimplices();
        std::map<Key,Sample> samples;
        auto evaluate = [&](const Vec& q) -> Sample {
            const auto cached = samples.find(key(q)); if (cached != samples.end()) { return cached->second; }
            Sample s; s.Q = q; s.Origin = cell.EvaluateParameter(q,s.P); long double value = 0;
            for (const auto& base : s.Origin.BaseNodes()) { value += (long double)base.second*Values->Value(RWCae::MeshPropertyEntityInfo(base.first)); }
            s.F = (double)(value-(long double)Definition.IsoValue);
            if (!std::isfinite(s.F)) { throw std::runtime_error("Isosurface field exceeds finite evaluation range"); }
            if (samples.size() >= Options.MaxPrimitives*4) { throw std::runtime_error("Isosurface sample budget exceeded"); }
            samples.emplace(key(q),s); return s;
        };
        std::vector<RWCae::MeshEntityId> originalNodes; mesh->ElementNodes(index,originalNodes);
        double scale = 0; Vec low(INFINITY), high(-INFINITY);
        for (auto node : originalNodes) {
            scale = std::max(scale,std::abs(Values->Value(RWCae::MeshPropertyEntityInfo(node))));
            auto p = mesh->Nodes()[node]; source->ModifyNode(node,p);
            for (int a = 0; a < 3; ++a) { low[a] = std::min(low[a],(double)p[a]); high[a] = std::max(high[a],(double)p[a]); }
        }
        const double tolerance = std::max(Options.AbsoluteTolerance,Options.RelativeTolerance*scale);
        if (!std::isfinite(tolerance)) { throw std::runtime_error("Isosurface field tolerance exceeds finite range"); }
        fieldTolerance = std::max(fieldTolerance,tolerance);
        const double geometricTolerance = Geometry.Tolerance((high-low).Modulus());
        int depth = 0;
        for (;;) {
            if (simplices.size() > Options.MaxPrimitives-primitives) { throw std::runtime_error("Isosurface primitive budget exceeded"); }
            double fieldError = 0, geometricError = 0;
            for (const auto& simplex : simplices) {
                std::array<Sample,4> vertices; Vec q(0), p(0); double f = 0;
                for (int a = 0; a <= dimension; ++a) { vertices[a] = evaluate(simplex[a]); q += simplex[a]; p += vertices[a].P; f += vertices[a].F/(dimension+1); }
                auto check = [&](const Vec& parameter, const Vec& position, double linear) {
                    const auto s = evaluate(parameter); fieldError = std::max(fieldError,std::abs(s.F-linear));
                    geometricError = std::max(geometricError,(s.P-position).Modulus());
                };
                check(q/(dimension+1),p/(dimension+1),f);
                for (int a = 0; a <= dimension; ++a) { for (int b = a+1; b <= dimension; ++b) {
                    check((vertices[a].Q+vertices[b].Q)*0.5,(vertices[a].P+vertices[b].P)*0.5,(vertices[a].F+vertices[b].F)*0.5);
                } }
            }
            if (depth >= Options.MinimumDepth && fieldError <= tolerance*0.25 && geometricError <= geometricTolerance*0.25) {
                residual = std::max(residual,fieldError); geometryResidual = std::max(geometryResidual,geometricError); break;
            }
            if (depth >= Options.SubdivisionLimit) { throw std::runtime_error("Isosurface sampled residual exceeds subdivision limit: field "
                +std::to_string(fieldError)+", geometry "+std::to_string(geometricError)); }
            simplices = subdivide(simplices,dimension,Options.MaxPrimitives-primitives); ++depth;
        }
        finalDepth = std::max(finalDepth,depth); primitives += simplices.size();
        const auto& original = mesh->IndexedElements()[index]; const MeshGeomElem origin(original.Index,original.Type,false);
        std::map<Key,RWCae::MeshEntityId> nodeIds;
        std::set<std::array<RWCae::MeshEntityId,3>> emittedTriangles;
        std::set<std::array<RWCae::MeshEntityId,2>> emittedLines;
        std::set<RWCae::MeshEntityId> emittedPoints;
        auto budget = [&]() {
            if (output->NbIndexedElems()+lines.size() >= Options.MaxPrimitives || output->NbNodes() >= Options.MaxPrimitives*4) {
                throw std::runtime_error("Isosurface output primitive budget exceeded");
            }
        };
        auto addNode = [&](const Sample& s) {
            if ((Portion == MeshCutPortion::Negative && s.F > tolerance)
                || (Portion == MeshCutPortion::Positive && s.F < -tolerance)) {
                throw std::runtime_error("Isosurface portion node exceeds field tolerance");
            }
            const auto found = nodeIds.find(key(s.Q)); if (found != nodeIds.end()) { return found->second; }
            for (int a = 0; a < 3; ++a) { if (!std::isfinite((float)s.P[a])) { throw std::runtime_error("Isosurface exceeds float coordinate storage"); } }
            const auto id = output->AddNodePosition(Graphic3d_Vec3((float)s.P.x(),(float)s.P.y(),(float)s.P.z()));
            nodesMap.Bind(id,std::make_pair(source.get(),s.Origin)); nodeIds.emplace(key(s.Q),id); return id;
        };
        auto bind = [&](RWCae::MeshElemType type) { elementsMap.Bind(MeshGeomElem(output->NbElems(type)-1,type,false),std::make_pair(source.get(),origin)); };
        auto triangle = [&](const Sample& a, const Sample& b, const Sample& c) {
            const double measure = Vec::Cross(b.P-a.P,c.P-a.P).Modulus()*0.5;
            if (measure <= 1e-20) { return; }
            const RWCae::MeshEntityId ids[] = {addNode(a),addNode(b),addNode(c)};
            std::array<RWCae::MeshEntityId,3> sorted = {{ids[0],ids[1],ids[2]}}; std::sort(sorted.begin(),sorted.end());
            if (!emittedTriangles.insert(sorted).second) { return; }
            if (Portion == MeshCutPortion::On && dimension == 3) {
                const auto center = evaluate((a.Q+b.Q+c.Q)/3);
                if (std::abs(center.F) > tolerance || (center.P-(a.P+b.P+c.P)/3).Modulus() > geometricTolerance) {
                    throw std::runtime_error("Isosurface facet residual exceeds tolerance");
                }
                residual = std::max(residual,std::abs(center.F));
                geometryResidual = std::max(geometryResidual,(center.P-(a.P+b.P+c.P)/3).Modulus());
            }
            budget();
            output->AddElement(RWCae::MeshElementTriangle(ids)); bind(RWCae::MeshElemType::Triangle); area += measure;
        };
        auto line = [&](const Sample& a, const Sample& b) {
            std::array<RWCae::MeshEntityId,2> ids = {{addNode(a),addNode(b)}}; auto sorted = ids; std::sort(sorted.begin(),sorted.end());
            if (!emittedLines.insert(sorted).second) { return; } budget(); lines.push_back({ids,index});
        };
        auto root = [&](Sample a, Sample b) {
            if (a.F == 0) { return a; } if (b.F == 0) { return b; }
            for (int iteration = 0; iteration < 64; ++iteration) {
                const auto middle = evaluate((a.Q+b.Q)*0.5);
                if (std::abs(middle.F) <= std::min(tolerance*1e-4,1e-10*std::max(1.0,scale))) { return middle; }
                if ((a.F < 0) == (middle.F < 0)) { a = middle; } else { b = middle; }
            }
            throw std::runtime_error("Isosurface edge root did not converge");
        };
        for (const auto& simplex : simplices) {
            std::vector<Sample> vertices, cap;
            for (int a = 0; a <= dimension; ++a) { vertices.push_back(evaluate(simplex[a])); }
            if (dimension == 3 && std::all_of(vertices.begin(),vertices.end(),[](const Sample& s) { return s.F == 0; })) {
                throw std::invalid_argument("Volumetric constant-level isosurface plateau is unsupported");
            }
            for (int a = 0; a <= dimension; ++a) {
                if (vertices[a].F == 0) { cap.push_back(vertices[a]); }
                for (int b = a+1; b <= dimension; ++b) {
                    if ((vertices[a].F < 0 && vertices[b].F > 0) || (vertices[a].F > 0 && vertices[b].F < 0)) { cap.push_back(root(vertices[a],vertices[b])); }
                }
            }
            order(cap);
            auto clip = [&](const std::vector<Sample>& polygon) {
                std::vector<Sample> result; const double sign = Portion == MeshCutPortion::Negative ? -1 : 1;
                for (size_t a = 0; a < polygon.size(); ++a) {
                    const auto& u = polygon[a]; const auto& v = polygon[(a+1)%polygon.size()];
                    if (sign*u.F >= 0) { result.push_back(u); }
                    if ((u.F < 0 && v.F > 0) || (u.F > 0 && v.F < 0)) { result.push_back(root(u,v)); }
                }
                unique(result); return result;
            };
            if (Portion == MeshCutPortion::On) {
                if (dimension == 1) {
                    if (cap.size() == 2) { line(cap[0],cap[1]); }
                    else for (const auto& s : cap) {
                        const RWCae::MeshEntityId ids[] = {addNode(s)}; if (!emittedPoints.insert(ids[0]).second) { continue; }
                        budget(); output->AddElement(RWCae::MeshElement0d(ids)); bind(RWCae::MeshElemType::Elem0d);
                    }
                } else if (dimension == 2) {
                    if (cap.size() == 2) { line(cap[0],cap[1]); }
                    else if (cap.size() == 3) { triangle(cap[0],cap[1],cap[2]); }
                } else { for (size_t a = 1; a+1 < cap.size(); ++a) { triangle(cap[0],cap[a],cap[a+1]); } }
                continue;
            }
            if (dimension == 1) {
                auto points = clip(vertices); if (points.size() >= 2) { line(points[0],points[1]); } continue;
            }
            if (dimension == 2) {
                auto polygon = clip(vertices); for (size_t a = 1; a+1 < polygon.size(); ++a) { triangle(polygon[0],polygon[a],polygon[a+1]); } continue;
            }
            std::vector<std::vector<Sample>> faces;
            const int ids[4][3] = {{0,2,1},{0,1,3},{1,2,3},{2,0,3}};
            for (const auto& face : ids) { auto polygon = clip({vertices[face[0]],vertices[face[1]],vertices[face[2]]}); if (polygon.size() >= 3) { faces.push_back(std::move(polygon)); } }
            if (cap.size() >= 3 && std::none_of(faces.begin(),faces.end(),[&](const std::vector<Sample>& face) {
                return face.size() == cap.size() && std::all_of(cap.begin(),cap.end(),[&](const Sample& s) {
                    return std::any_of(face.begin(),face.end(),[&](const Sample& f) { return (f.Q-s.Q).Modulus() < 1e-12; });
                });
            })) { faces.push_back(cap); }
            std::vector<Sample> corners; for (const auto& face : faces) { corners.insert(corners.end(),face.begin(),face.end()); } unique(corners);
            if (corners.size() < 4) { continue; }
            Vec center(0); for (const auto& s : corners) { center += s.Q; } auto middle = evaluate(center/(double)corners.size());
            for (auto& face : faces) {
                // A stable fan gives the same diagonal on both sides of a shared simplex face.
                const auto first = std::min_element(face.begin(),face.end(),[](const Sample& a, const Sample& b) { return key(a.Q) < key(b.Q); });
                std::rotate(face.begin(),first,face.end());
                for (size_t a = 1; a+1 < face.size(); ++a) {
                    const double measure = std::abs(determinant(face[0].P-middle.P,face[a].P-middle.P,face[a+1].P-middle.P))/6;
                    if (measure <= 1e-22) { continue; }
                    const RWCae::MeshEntityId nodes[] = {addNode(middle),addNode(face[0]),addNode(face[a]),addNode(face[a+1])};
                    budget();
                    output->AddElement(RWCae::MeshElementTetrahedron(nodes)); bind(RWCae::MeshElemType::Tetrahedron); volume += measure;
                }
            }
        }
    }
    myMeshData = output; myNodesMap = nodesMap; myElementsMap = elementsMap; myLines = std::move(lines);
    Volume = volume; Area = area; FieldTolerance = fieldTolerance; FieldResidual = residual; GeometryResidual = geometryResidual;
    Depth = finalDepth; Primitives = primitives;
}
}

#include <ViewCaeGeom/MeshVtkCutSource.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tuple>
#include <limits>

namespace {
gp_Pnt point(const Graphic3d_Vec3d& p) { return gp_Pnt(p.x(),p.y(),p.z()); }
std::pair<double,double> interval(const Graphic3d_Vec3d& low, const Graphic3d_Vec3d& high,
    const ViewCaeGeom::MeshCutDefinition& cut)
{
    if (cut.Shape == ViewCaeGeom::MeshCutShape::Plane) {
        double minimum = cut.Plane.w(), maximum = minimum;
        for (int a = 0; a < 3; ++a) {
            const double x = cut.Plane[a]*low[a], y = cut.Plane[a]*high[a];
            minimum += std::min(x,y); maximum += std::max(x,y);
        }
        return {minimum,maximum};
    }
    if (cut.Shape == ViewCaeGeom::MeshCutShape::Sphere) {
        double nearSquared = 0, farSquared = 0;
        for (int a = 0; a < 3; ++a) {
            const double x = low[a]-cut.Center[a], y = high[a]-cut.Center[a];
            nearSquared += x > 0 ? x*x : y < 0 ? y*y : 0;
            farSquared += std::max(x*x,y*y);
        }
        return {std::sqrt(nearSquared)-cut.Radius,std::sqrt(farSquared)-cut.Radius};
    }
    auto d = (low+high)*0.5-cut.Center; d -= cut.Axis*d.Dot(cut.Axis);
    const double radius = (high-low).Modulus()*0.5;
    return {std::max(0.0,d.Modulus()-radius)-cut.Radius,d.Modulus()+radius-cut.Radius};
}
Graphic3d_Vec3d vector(const gp_Pnt& p) { return Graphic3d_Vec3d(p.X(),p.Y(),p.Z()); }
}

namespace ViewCaeGeom {
void MeshVtkCutSource::Prepare(const Handle(RWCae::MeshData)& mesh, MeshGeometry* source)
{
    if (myReady) { return; }
    Bnd_Box nodalBounds;
    double storageResolution = 0;
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        auto p = mesh->Nodes()[n]; source->ModifyNode(n,p);
        nodalBounds.Add(point(Graphic3d_Vec3d(p)));
        for (int a = 0; a < 3; ++a) {
            storageResolution = std::max(storageResolution,(double)(std::nextafter(std::abs(p[a]),std::numeric_limits<float>::infinity())-std::abs(p[a])));
        }
    }
    const double scale = nodalBounds.IsVoid() ? 0 : (nodalBounds.CornerMax().XYZ()-nodalBounds.CornerMin().XYZ()).Modulus();
    const double tolerance = myOptions.Tolerance(scale);
    std::vector<Cell> cells;
    cells.reserve(mesh->NbIndexedElems());
    int commonDepth = 0;
    uint64_t preparedPrimitives = 0;
    for (RWCae::MeshEntityId index = 0; index < mesh->NbIndexedElems(); ++index) {
        Cell cell; cell.Geometry = std::make_unique<MeshVtkCell>(mesh,index,source,tolerance,myOptions);
        if (cell.Geometry->Dimension() == 3) { commonDepth = std::max(commonDepth,cell.Geometry->FaceDepth); }
        preparedPrimitives += cell.Geometry->Triangles.size()+cell.Geometry->Segments.size()+cell.Geometry->Points.size();
        if (preparedPrimitives > myOptions.MaxSourcePrimitives) { throw std::runtime_error("VTK source primitive budget exceeded"); }
        cells.push_back(std::move(cell));
    }
    // Synchronous correctness reference: one curved-face level for this epoch avoids T-junctions.
    // Later memory/performance policy can partition connected components without changing topology.
    if (commonDepth) {
        for (RWCae::MeshEntityId index = 0; index < cells.size(); ++index) {
            if (cells[index].Geometry->Dimension() == 3) {
                const auto& before = *cells[index].Geometry;
                preparedPrimitives -= before.Triangles.size()+before.Segments.size()+before.Points.size();
                cells[index].Geometry = std::make_unique<MeshVtkCell>(mesh,index,source,tolerance,myOptions,commonDepth);
                const auto& after = *cells[index].Geometry;
                preparedPrimitives += after.Triangles.size()+after.Segments.size()+after.Points.size();
                if (preparedPrimitives > myOptions.MaxSourcePrimitives) { throw std::runtime_error("VTK source primitive budget exceeded"); }
            }
        }
    }
    Handle(BoxSet) boxes = new BoxSet();
    boxes->SetSize(mesh->NbIndexedElems());
    using FaceKey = std::tuple<int,std::vector<RWCae::MeshEntityId>,std::vector<std::vector<RWCae::MeshEntityId>>>;
    using Reference = std::pair<RWCae::MeshEntityId,size_t>;
    std::map<FaceKey,std::vector<Reference>> adjacency;
    Bnd_Box bounds;
    const double epsilon = std::max(1e-7,tolerance*1e-5);
    uint64_t primitives = 0;
    for (RWCae::MeshEntityId index = 0; index < mesh->NbIndexedElems(); ++index) {
        auto& prepared = cells[index];
        const auto& cell = *prepared.Geometry;
        primitives += cell.Triangles.size()+cell.Segments.size()+cell.Points.size();
        if (primitives > myOptions.MaxSourcePrimitives) { throw std::runtime_error("VTK source primitive budget exceeded"); }
        for (const auto& t : cell.Triangles) { for (const auto& p : t) { prepared.Bounds.Add(point(p)); } }
        for (const auto& s : cell.Segments) { for (const auto& p : s) { prepared.Bounds.Add(point(p)); } }
        for (const auto& p : cell.Points) { prepared.Bounds.Add(point(p)); }
        if (!cell.AnalyticBounds.IsVoid()) { prepared.Bounds.Add(cell.AnalyticBounds); }
        if (!prepared.Bounds.IsVoid()) {
            bounds.Add(prepared.Bounds);
            boxes->Add(index,BVH_Box<double,3>(vector(prepared.Bounds.CornerMin()),vector(prepared.Bounds.CornerMax())));
        }
        prepared.Faces.resize(cell.Faces.size());
        for (size_t f = 0; f < cell.Faces.size(); ++f) {
            const auto& topology = cell.Faces[f];
            auto& face = prepared.Faces[f]; face.Owner = index;
            if (!topology.NbTriangles) { continue; }
            const auto& t = cell.Triangles[topology.FirstTriangle];
            auto normal = Graphic3d_Vec3d::Cross(t[1]-t[0],t[2]-t[0]);
            if (normal.Modulus() <= 1e-15) { continue; }
            normal /= normal.Modulus(); face.Plane = gp_Pln(point(t[0]),gp_Dir(normal.x(),normal.y(),normal.z()));
            face.Planar = true;
            for (size_t n = topology.FirstTriangle; n < topology.FirstTriangle+topology.NbTriangles; ++n) {
                for (const auto& p : cell.Triangles[n]) { face.Planar &= face.Plane.Distance(point(p)) <= epsilon; }
            }
            if (cell.Dimension() == 3) {
                auto nodes = topology.Nodes; std::sort(nodes.begin(),nodes.end());
                const int type = topology.Type == 8 ? 9 : topology.Type;
                auto edges = topology.Edges;
                for (auto& edge : edges) { std::sort(edge.begin(),edge.end()); }
                std::sort(edges.begin(),edges.end());
                adjacency[{type,std::move(nodes),std::move(edges)}].push_back({index,f});
            }
        }
    }
    uint64_t unmerged = 0;
    for (const auto& entry : adjacency) {
        const auto& owners = entry.second;
        if (owners.size() <= 1) { continue; }
        if (owners.size() != 2 || owners[0].first == owners[1].first) { ++unmerged; continue; }
        auto& a = cells[owners[0].first]; auto& b = cells[owners[1].first];
        auto& af = a.Faces[owners[0].second]; auto& bf = b.Faces[owners[1].second];
        const auto& at = a.Geometry->Faces[owners[0].second];
        const auto& bt = b.Geometry->Faces[owners[1].second];
        bool opposite = false;
        if ((!af.Planar || !bf.Planar) || !at.StraightSided || !bt.StraightSided) {
            using VertexKey = std::array<int64_t,3>;
            using TriangleKey = std::array<VertexKey,3>;
            const auto origin = a.Geometry->Triangles[at.FirstTriangle][0];
            double faceExtent = 0;
            for (size_t t = at.FirstTriangle; t < at.FirstTriangle+at.NbTriangles; ++t) {
                for (const auto& p : a.Geometry->Triangles[t]) { faceExtent = std::max(faceExtent,(p-origin).Modulus()); }
            }
            const double keyTolerance = std::max(epsilon,faceExtent*1e-12);
            auto triangulation = [&](const Cell& cell, const MeshVtkCell::Face& topology) {
                std::vector<TriangleKey> keys;
                for (size_t t = topology.FirstTriangle; t < topology.FirstTriangle+topology.NbTriangles; ++t) {
                    TriangleKey key;
                    for (int n = 0; n < 3; ++n) {
                        const auto p = cell.Geometry->Triangles[t][n]-origin;
                        for (int axis = 0; axis < 3; ++axis) { key[n][axis] = (int64_t)std::llround(p[axis]/keyTolerance); }
                    }
                    std::sort(key.begin(),key.end()); keys.push_back(key);
                }
                std::sort(keys.begin(),keys.end()); return keys;
            };
            const auto ap = a.Geometry->InteriorPoint(), bp = b.Geometry->InteriorPoint();
            opposite = at.NbTriangles && triangulation(a,at) == triangulation(b,bt);
            for (size_t t = at.FirstTriangle; opposite && t < at.FirstTriangle+at.NbTriangles; ++t) {
                const auto& triangle = a.Geometry->Triangles[t];
                auto normal = Graphic3d_Vec3d::Cross(triangle[1]-triangle[0],triangle[2]-triangle[0]);
                if (normal.Modulus() <= 1e-15) { opposite = false; break; }
                normal /= normal.Modulus(); const auto center = (triangle[0]+triangle[1]+triangle[2])/3;
                opposite = (ap-center).Dot(normal)*(bp-center).Dot(normal) < -epsilon*epsilon;
            }
        } else {
            const auto normal = af.Plane.Axis().Direction();
            const Graphic3d_Vec3d direction(normal.X(),normal.Y(),normal.Z()), origin = vector(af.Plane.Location());
            auto support = [&](const Cell& cell) {
                double minimum = INFINITY, maximum = -INFINITY;
                for (const auto& t : cell.Geometry->Triangles) {
                    for (const auto& p : t) {
                        const double d = (p-origin).Dot(direction); minimum = std::min(minimum,d); maximum = std::max(maximum,d);
                    }
                }
                return std::make_pair(minimum,maximum);
            };
            const auto x = support(a), y = support(b);
            opposite = (x.first >= -epsilon && x.second > epsilon && y.second <= epsilon && y.first < -epsilon)
                || (y.first >= -epsilon && y.second > epsilon && x.second <= epsilon && x.first < -epsilon);
        }
        if (!opposite) { ++unmerged; continue; }
        af.Internal = bf.Internal = true; af.Owner = bf.Owner = std::min(owners[0].first,owners[1].first);
        af.NeighborElement = owners[1].first; af.NeighborFace = owners[1].second;
        bf.NeighborElement = owners[0].first; bf.NeighborFace = owners[0].second;
    }
    if (boxes->Size()) { boxes->Build(); }
    myCells = std::move(cells); myBoxes = boxes; myBounds = bounds; myTolerance = tolerance; myReady = true;
    ++Work.SourcePreparations; Work.PreparedCells = myCells.size(); Work.BvhBuilds += boxes->Size() ? 1 : 0;
    Work.UnmergedFaceGroups = unmerged;
    Work.SourcePrimitives = primitives; Work.FaceSubdivisionDepth = commonDepth;
    Work.GeometryTolerance = tolerance; Work.StorageResolution = storageResolution;
    for (const auto& cell : myCells) {
        Work.NumericalCells += cell.Geometry->SourceApproximate ? 1 : 0;
        bool analytic = !cell.Geometry->Curve.IsNull() || !cell.Geometry->Surface.IsNull() || !cell.Geometry->BoundaryCurves.empty();
        for (const auto& face : cell.Geometry->Faces) { analytic |= !face.Surface.IsNull(); }
        Work.AnalyticCells += analytic ? 1 : 0;
        Work.AnalyticResidual = std::max(Work.AnalyticResidual,cell.Geometry->AnalyticResidual);
    }
}

std::array<MeshVtkCutSource::InterfaceTrace,2> MeshVtkCutSource::InterfaceTraces(
    RWCae::MeshEntityId element, size_t faceIndex, const Graphic3d_Vec3d& position) const
{
    if (!myReady) { throw std::logic_error("Interface traces require a prepared source"); }
    for (int a = 0; a < 3; ++a) {
        if (!std::isfinite(position[a])) { throw std::invalid_argument("Interface point must be finite"); }
    }
    const auto& cell = myCells.at(element);
    const auto& face = cell.Faces.at(faceIndex);
    if (!face.Internal) { throw std::invalid_argument("Interface traces require verified two-sided topology"); }
    const auto& neighbor = myCells.at(face.NeighborElement);
    const auto& reciprocal = neighbor.Faces.at(face.NeighborFace);
    if (!reciprocal.Internal || reciprocal.NeighborElement != element || reciprocal.NeighborFace != faceIndex) {
        throw std::logic_error("Interface topology is not reciprocal");
    }
    const auto& topology = cell.Geometry->Faces.at(faceIndex);
    const double tolerance = std::max(1e-7,myTolerance*2);
    bool onFace = false;
    if (!topology.Surface.IsNull()) {
        GeomAPI_ProjectPointOnSurf projection(point(position),topology.Surface,0,1,0,1);
        onFace = projection.NbPoints() && projection.LowerDistance() <= tolerance;
    } else {
        auto triangle=std::make_unique<MeshCellKernel>(5);
        const double p[] = {position.x(),position.y(),position.z()};
        for (size_t t = topology.FirstTriangle; !onFace && t < topology.FirstTriangle+topology.NbTriangles; ++t) {
            for (int n = 0; n < 3; ++n) {
                const auto& q = cell.Geometry->Triangles[t][n]; triangle->GetPoints()->SetPoint(n,q.x(),q.y(),q.z());
            }
            double closest[3], parametric[3], distance, weights[3]; int sub = 0;
            const int valid = triangle->EvaluatePosition(p,closest,sub,parametric,distance,weights);
            onFace = valid >= 0 && std::isfinite(distance) && distance <= tolerance*tolerance;
        }
    }
    if (!onFace) { throw std::invalid_argument("Point is outside the prepared interface face"); }
    std::array<InterfaceTrace,2> result = {{
        {element,faceIndex,cell.Geometry->InteriorPoint(),cell.Geometry->Origin(position)},
        {face.NeighborElement,face.NeighborFace,neighbor.Geometry->InteriorPoint(),neighbor.Geometry->Origin(position)}
    }};
    if (result[0].Element > result[1].Element) { std::swap(result[0],result[1]); }
    return result;
}

std::vector<RWCae::MeshEntityId> MeshVtkCutSource::Query(const MeshCutDefinition& cut, MeshCutPortion portion, bool sourceOnly)
{
    std::vector<RWCae::MeshEntityId> result;
    if (!myReady || !myBoxes->Size()) { return result; }
    auto accepted = [&](const Graphic3d_Vec3d& low, const Graphic3d_Vec3d& high) {
        ++Work.BoxTests;
        if (sourceOnly) { return true; }
        const auto range = interval(low,high,cut);
        return portion == MeshCutPortion::On ? range.first <= 1e-7 && range.second >= -1e-7
            : portion == MeshCutPortion::Negative ? range.first <= 1e-7 : range.second >= -1e-7;
    };
    const auto& tree = myBoxes->BVH();
    std::vector<int> pending = {0};
    while (!pending.empty()) {
        const int node = pending.back(); pending.pop_back();
        if (!accepted(tree->MinPoint(node),tree->MaxPoint(node))) { continue; }
        if (!tree->IsOuter(node)) {
            pending.push_back(tree->BegPrimitive(node)); pending.push_back(tree->EndPrimitive(node));
        } else {
            for (int p = tree->BegPrimitive(node); p <= tree->EndPrimitive(node); ++p) {
                const auto id = myBoxes->Element(p); const auto& box = myCells[id].Bounds;
                if (accepted(vector(box.CornerMin()),vector(box.CornerMax()))) { result.push_back(id); }
            }
        }
    }
    std::sort(result.begin(),result.end()); Work.CandidateCells += result.size(); return result;
}

const TopoDS_Shape& MeshVtkCutSource::Shape(RWCae::MeshEntityId index)
{
    auto& prepared = myCells.at(index);
    if (!prepared.Shape.IsNull()) { return prepared.Shape; }
    const auto& cell = *prepared.Geometry;
    BRep_Builder builder; TopoDS_Compound compound; builder.MakeCompound(compound);
    BRepBuilderAPI_Sewing sewing(cell.SourceApproximate ? std::max(1e-7,myTolerance*0.1) : 1e-7);
    std::vector<PrimitiveFace> primitiveFaces;
    size_t faceIndex = 0;
    auto analyticFace = [](const Handle(Geom_BezierSurface)& surface) {
        const std::array<gp_Pnt,4> corners = {{surface->Value(0,0),surface->Value(1,0),surface->Value(1,1),surface->Value(0,1)}};
        auto normal = Graphic3d_Vec3d::Cross(vector(corners[1])-vector(corners[0]),vector(corners[3])-vector(corners[0]));
        const double epsilon = 1e-7;
        bool planar = normal.Modulus() > epsilon*epsilon;
        if (planar) { normal /= normal.Modulus(); }
        for (int u = 1; planar && u <= surface->NbUPoles(); ++u) {
            for (int v = 1; planar && v <= surface->NbVPoles(); ++v) {
                planar = std::abs((vector(surface->Pole(u,v))-vector(corners[0])).Dot(normal)) <= epsilon;
            }
        }
        for (int edge = 0; planar && edge < 4; ++edge) {
            const auto start = vector(corners[edge]), end = vector(corners[(edge+1)%4]);
            const auto direction = end-start; const double length = direction.Modulus();
            const int count = edge%2 ? surface->NbVPoles() : surface->NbUPoles();
            for (int n = 1; planar && n <= count; ++n) {
                const auto p = vector(edge == 0 ? surface->Pole(n,1) : edge == 1 ? surface->Pole(surface->NbUPoles(),n)
                    : edge == 2 ? surface->Pole(count-n+1,surface->NbVPoles()) : surface->Pole(1,count-n+1));
                const auto d = p-start;
                if (length <= epsilon) { planar = d.Modulus() <= epsilon; }
                else {
                    const double t = d.Dot(direction)/(length*length);
                    planar = Graphic3d_Vec3d::Cross(d,direction).Modulus()/length <= epsilon && t >= -1e-7 && t <= 1+1e-7;
                }
            }
        }
        if (planar) {
            BRepBuilderAPI_MakePolygon polygon;
            for (int n = 0; n < 4; ++n) { if (!n || corners[n].Distance(corners[n-1]) > epsilon) { polygon.Add(corners[n]); } }
            polygon.Close(); return BRepBuilderAPI_MakeFace(polygon.Wire()).Face();
        }
        auto face = BRepBuilderAPI_MakeFace(surface,0,1,0,1,1e-7).Face();
        if (!BRepCheck_Analyzer(face).IsValid()) { throw std::runtime_error("Invalid analytic VTK face"); }
        return face;
    };
    if (!cell.Surface.IsNull()) { builder.Add(compound,analyticFace(cell.Surface)); }
    if (!cell.BoundaryCurves.empty()) {
        BRepBuilderAPI_MakeWire wire;
        for (const auto& curve : cell.BoundaryCurves) { wire.Add(BRepBuilderAPI_MakeEdge(curve,0,1).Edge()); }
        const auto face = BRepBuilderAPI_MakeFace(wire.Wire()).Face();
        if (!BRepCheck_Analyzer(face).IsValid()) { throw std::runtime_error("Invalid analytic quadratic-polygon boundary"); }
        builder.Add(compound,face);
    }
    for (size_t f = 0; f < cell.Faces.size(); ++f) {
        if (!cell.Faces[f].Surface.IsNull()) {
            const auto face = analyticFace(cell.Faces[f].Surface);
            sewing.Add(face); primitiveFaces.push_back({face,f});
        }
    }
    for (size_t t = 0; t < cell.Triangles.size(); ++t) {
        while (faceIndex+1 < cell.Faces.size() && t >= cell.Faces[faceIndex+1].FirstTriangle) { ++faceIndex; }
        if (!cell.Surface.IsNull() || !cell.BoundaryCurves.empty() || (cell.Dimension() == 3 && !cell.Faces[faceIndex].Surface.IsNull())) { continue; }
        const auto& triangle = cell.Triangles[t];
        if (Graphic3d_Vec3d::Cross(triangle[1]-triangle[0],triangle[2]-triangle[0]).Modulus() < 1e-15) { continue; }
        const auto wire = BRepBuilderAPI_MakePolygon(point(triangle[0]),point(triangle[1]),point(triangle[2]),true).Wire();
        const auto face = BRepBuilderAPI_MakeFace(wire).Face();
        if (cell.Dimension() == 3) { sewing.Add(face); primitiveFaces.push_back({face,faceIndex}); }
        else { builder.Add(compound,face); }
    }
    if (!cell.Curve.IsNull()) { builder.Add(compound,BRepBuilderAPI_MakeEdge(cell.Curve,0,1).Edge()); }
    for (const auto& segment : cell.Segments) {
        if (!cell.Curve.IsNull()) { break; }
        if ((segment[0]-segment[1]).Modulus() > 1e-7) { builder.Add(compound,BRepBuilderAPI_MakeEdge(point(segment[0]),point(segment[1])).Edge()); }
    }
    if (cell.Dimension() == 3) {
        sewing.Perform(); BRepBuilderAPI_MakeSolid solid;
        for (TopExp_Explorer shell(sewing.SewedShape(),TopAbs_SHELL); shell.More(); shell.Next()) { solid.Add(TopoDS::Shell(shell.Current())); }
        auto shape = solid.Solid();
        if (!BRepLib::OrientClosedSolid(shape) || !BRepCheck_Analyzer(shape).IsValid()) {
            throw std::runtime_error("VTK cell boundary is not a valid closed orientable solid");
        }
        for (auto& face : primitiveFaces) { face.Shape = sewing.IsModified(face.Shape) ? sewing.Modified(face.Shape) : face.Shape; }
        prepared.Shape = shape; prepared.PrimitiveFaces = std::move(primitiveFaces);
    } else { prepared.Shape = compound; }
    ++Work.ShapeBuilds; return prepared.Shape;
}
}

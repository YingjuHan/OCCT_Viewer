#include <ViewCaeGeom/MeshGeometryQuadric.h>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <RWCae/MeshAdaptorsIterator.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <map>

namespace {
using Point = Graphic3d_Vec3d;

gp_Pnt point(const Point& p) { return gp_Pnt(p.x(), p.y(), p.z()); }
Point vector(const gp_Pnt& p) { return Point(p.X(), p.Y(), p.Z()); }

struct SourceCell {
    ViewCaeGeom::MeshGeometry* Geometry;
    ViewCaeGeom::MeshGeomElem Element;
    std::vector<RWCae::MeshEntityId> Nodes;
    std::vector<Point> Positions;
    double Scale = 0.0;

    ViewCaeGeom::MeshInterpolatedNode origin(const Point& p) const
    {
        const Point a = Positions[1]-Positions[0], b = Positions[2]-Positions[0], d = p-Positions[0];
        double weights[4] = {};
        if (Nodes.size() == 4) {
            const Point c = Positions[3]-Positions[0];
            const double determinant = a.Dot(Point::Cross(b,c));
            if (std::abs(determinant) <= Scale*Scale*Scale*1e-12) {
                throw std::invalid_argument("Curved cut encountered a degenerate tetrahedron");
            }
            weights[1] = d.Dot(Point::Cross(b,c))/determinant;
            weights[2] = a.Dot(Point::Cross(d,c))/determinant;
            weights[3] = a.Dot(Point::Cross(b,d))/determinant;
        } else {
            const double aa = a.Dot(a), bb = b.Dot(b), ab = a.Dot(b);
            const double determinant = aa*bb-ab*ab;
            if (determinant <= std::pow(Scale,4)*1e-12) {
                throw std::invalid_argument("Curved cut encountered a degenerate triangle");
            }
            weights[1] = (bb*d.Dot(a)-ab*d.Dot(b))/determinant;
            weights[2] = (aa*d.Dot(b)-ab*d.Dot(a))/determinant;
        }
        weights[0] = 1-weights[1]-weights[2]-weights[3];
        ViewCaeGeom::MeshInterpolatedNode result((int)Nodes.size());
        for (size_t n = 0; n < Nodes.size(); ++n) {
            if (!std::isfinite(weights[n]) || weights[n] < -1e-5 || weights[n] > 1+1e-5) {
                throw std::runtime_error("Curved intersection is outside its source element");
            }
            result.AddBaseNode(Nodes[n], weights[n]);
        }
        return result;
    }
};

TopoDS_Face triangle(const Point& a, const Point& b, const Point& c)
{
    BRepBuilderAPI_MakePolygon wire(point(a), point(b), point(c), true);
    BRepBuilderAPI_MakeFace face(wire.Wire());
    if (!face.IsDone()) { throw std::runtime_error("Cannot build curved-cut source face"); }
    return face.Face();
}

TopoDS_Shape cellShape(const SourceCell& cell)
{
    const auto& p = cell.Positions;
    // Validate FE coordinates before OCCT constructs a shape.
    cell.origin(p[0]);
    if (p.size() == 3) { return triangle(p[0],p[1],p[2]); }
    BRepBuilderAPI_Sewing sewing;
    sewing.Add(triangle(p[0],p[2],p[1]));
    sewing.Add(triangle(p[0],p[1],p[3]));
    sewing.Add(triangle(p[1],p[2],p[3]));
    sewing.Add(triangle(p[2],p[0],p[3]));
    sewing.Perform();
    if (sewing.SewedShape().ShapeType() != TopAbs_SHELL) {
        throw std::runtime_error("Cannot sew curved-cut source tetrahedron");
    }
    auto solid = BRepBuilderAPI_MakeSolid(TopoDS::Shell(sewing.SewedShape())).Solid();
    if (!BRepLib::OrientClosedSolid(solid) || !BRepCheck_Analyzer(solid).IsValid()) {
        throw std::runtime_error("Invalid curved-cut source tetrahedron");
    }
    return solid;
}

template<class Operation>
TopoDS_Shape boolean(const TopoDS_Shape& cell, const TopoDS_Shape& tool)
{
    Operation operation;
    TopTools_ListOfShape arguments, tools;
    arguments.Append(cell); tools.Append(tool);
    operation.SetArguments(arguments); operation.SetTools(tools);
    operation.SetNonDestructive(true);
    operation.SetRunParallel(false);
    operation.Build();
    if (!operation.IsDone() || operation.HasErrors()) {
        throw std::runtime_error("OCCT curved-cut intersection failed");
    }
    return operation.Shape();
}
}

namespace ViewCaeGeom {
MeshGeometryQuadric::MeshGeometryQuadric(const Handle(MeshBvhData)& bvh,
    const MeshCutDefinition& definition, MeshCutPortion portion, const MeshCutGeometryOptions& options)
    : MeshGeometrySlice(bvh), myDefinition(definition), myPortion(portion), myOptions(options) {}

void MeshGeometryQuadric::ToJson(RWBase::JsonHelper::Writer&)
{
    throw std::logic_error("Curved cuts must be saved through MeshCutViewManager");
}

void MeshGeometryQuadric::generateMeshData()
{
    // Publish only a fully successful result, so a failed calculation can be retried.
    Handle(RWCae::MeshData) output = new RWCae::MeshData();
    NodesMap nodesMap;
    ElementsMap elementsMap;
    std::vector<MeshSliceLine> lines;
    double volume = 0.0, area = 0.0;
    auto source = myParentGeometry->GetFirstParent();
    const auto mesh = source->GetMeshData();
    std::map<std::array<RWCae::MeshEntityId,3>,unsigned> faceCounts;
    const RWCae::MeshAdaptor<RWCae::MeshElementTetrahedron> tetrahedra(*mesh);
    constexpr int faces[4][3] = {{0,2,1},{0,1,3},{1,2,3},{2,0,3}};
    for (RWCae::MeshEntityId e = 0; e < tetrahedra.NbElements(); ++e) {
        const auto& element = tetrahedra.GetElement(e);
        for (const auto& face : faces) {
            std::array<RWCae::MeshEntityId,3> key = {{element[face[0]],element[face[1]],element[face[2]]}};
            std::sort(key.begin(),key.end()); ++faceCounts[key];
        }
    }
    const auto& definition = myDefinition;
    Bnd_Box bounds;
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        auto p = mesh->Nodes()[n]; source->ModifyNode(n,p);
        bounds.Add(gp_Pnt(p.x(),p.y(),p.z()));
    }
    const double scale = bounds.IsVoid() ? 0 : (bounds.CornerMax().XYZ()-bounds.CornerMin().XYZ()).Modulus();
    const double deflection = std::min(myOptions.Tolerance(scale),definition.Radius*0.01);
    const gp_Dir axis(definition.Axis.x(),definition.Axis.y(),definition.Axis.z());
    TopoDS_Shape tool;
    if (definition.Shape == MeshCutShape::Sphere) {
        tool = BRepPrimAPI_MakeSphere(point(definition.Center),definition.Radius).Shape();
    } else {
        double minimum = INFINITY, maximum = -INFINITY;
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            auto p = mesh->Nodes()[n]; source->ModifyNode(n,p);
            const double t = (Point(p)-definition.Center).Dot(definition.Axis);
            minimum = std::min(minimum,t); maximum = std::max(maximum,t);
        }
        if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
            throw std::invalid_argument("Cylinder requires finite source coordinates");
        }
        const double margin = std::max({maximum-minimum,definition.Radius,1.0})*0.1;
        const auto start = definition.Center+definition.Axis*(minimum-margin);
        tool = BRepPrimAPI_MakeCylinder(gp_Ax2(point(start),axis),definition.Radius,
            maximum-minimum+2*margin).Shape();
    }
    TopoDS_Shell boundary;
    if (definition.Shape == MeshCutShape::Sphere) {
        boundary = TopoDS::Shell(TopExp_Explorer(tool,TopAbs_SHELL).Current());
    } else {
        // Exclude finite construction caps: the manager defines an infinite cylinder.
        BRep_Builder builder;
        builder.MakeShell(boundary);
        for (TopExp_Explorer f(tool,TopAbs_FACE); f.More(); f.Next()) {
            if (BRepAdaptor_Surface(TopoDS::Face(f.Current())).GetType() == GeomAbs_Cylinder) {
                builder.Add(boundary,f.Current());
            }
        }
    }

    auto process = [&](const auto& adaptor) {
        for (RWCae::MeshEntityId e = 0; e < adaptor.NbElements(); ++e) {
            const auto& element = adaptor.GetElement(e);
            if (!source->AcceptElement(element.GlobalIndex())) { continue; }
            SourceCell cell{source.get(),MeshGeomElem(e,adaptor.GetElementType(),false),{},{},0.0};
            for (RWCae::MeshElemEntityId n = 0; n < element.NbNodes(); ++n) {
                cell.Nodes.push_back(element[n]);
                auto p = mesh->Nodes()[element[n]]; source->ModifyNode(element[n],p);
                for (int a = 0; a < 3; ++a) {
                    if (!std::isfinite(p[a])) { throw std::invalid_argument("Curved cut requires finite coordinates"); }
                }
                cell.Positions.emplace_back(p);
                cell.Scale = std::max(cell.Scale,(Point(p)-cell.Positions[0]).Modulus());
            }
            const auto shape = cellShape(cell);
            const auto result = myPortion == MeshCutPortion::Positive ? boolean<BRepAlgoAPI_Cut>(shape,tool)
                : myPortion == MeshCutPortion::Negative ? boolean<BRepAlgoAPI_Common>(shape,tool)
                : cell.Nodes.size() == 3 ? boolean<BRepAlgoAPI_Section>(shape,boundary)
                : boolean<BRepAlgoAPI_Common>(shape,boundary);
            GProp_GProps properties;
            BRepGProp::SurfaceProperties(result,properties); area += properties.Mass();
            if (myPortion != MeshCutPortion::On && cell.Nodes.size() == 4) {
                BRepGProp::VolumeProperties(result,properties); volume += properties.Mass();
            }
            auto addNode = [&](const gp_Pnt& p) {
                const auto id = output->AddNodePosition(Graphic3d_Vec3((float)p.X(),(float)p.Y(),(float)p.Z()));
                nodesMap.Bind(id,std::make_pair(cell.Geometry,cell.origin(vector(p))));
                return id;
            };
            BRepMesh_IncrementalMesh mesher(result,deflection,false,0.25,false);
            if (!mesher.IsDone()) { throw std::runtime_error("Cannot tessellate curved intersection"); }
            for (TopExp_Explorer f(result,TopAbs_FACE); f.More(); f.Next()) {
                const auto face = TopoDS::Face(f.Current());
                if (cell.Nodes.size() == 4 && myPortion != MeshCutPortion::On) {
                    const BRepAdaptor_Surface surface(face);
                    bool internal = false;
                    if (surface.GetType() == GeomAbs_Plane) {
                        for (const auto& indices : faces) {
                            std::array<RWCae::MeshEntityId,3> key = {{cell.Nodes[indices[0]],cell.Nodes[indices[1]],cell.Nodes[indices[2]]}};
                            std::sort(key.begin(),key.end());
                            if (faceCounts[key] <= 1) { continue; }
                            const auto plane = surface.Plane();
                            const double tolerance = std::max(1e-7,cell.Scale*1e-7);
                            if (plane.Distance(point(cell.Positions[indices[0]])) <= tolerance
                                && plane.Distance(point(cell.Positions[indices[1]])) <= tolerance
                                && plane.Distance(point(cell.Positions[indices[2]])) <= tolerance) {
                                internal = true; break;
                            }
                        }
                    }
                    if (internal) {
                        BRepGProp::SurfaceProperties(face,properties); area -= properties.Mass();
                        continue;
                    }
                }
                TopLoc_Location location;
                const auto triangulation = BRep_Tool::Triangulation(face,location);
                if (triangulation.IsNull()) { throw std::runtime_error("Missing curved-cut face triangulation"); }
                std::vector<RWCae::MeshEntityId> ids;
                for (int n = 1; n <= triangulation->NbNodes(); ++n) {
                    ids.push_back(addNode(triangulation->Node(n).Transformed(location.Transformation())));
                }
                for (int t = 1; t <= triangulation->NbTriangles(); ++t) {
                    int a,b,c; triangulation->Triangle(t).Get(a,b,c);
                    if (face.Orientation() == TopAbs_REVERSED) { std::swap(b,c); }
                    const RWCae::MeshEntityId vertices[] = {ids[a-1],ids[b-1],ids[c-1]};
                    output->AddElement(RWCae::MeshElementTriangle(vertices));
                    elementsMap.Bind(MeshGeomElem(output->NbElems(RWCae::MeshElemType::Triangle)-1,
                        RWCae::MeshElemType::Triangle,false),std::make_pair(cell.Geometry,cell.Element));
                }
            }
            if (myPortion == MeshCutPortion::On && cell.Nodes.size() == 3) {
                RWCae::MeshEntityId origin = 0;
                if (!source->OriginElement(cell.Element,origin)) { throw std::runtime_error("Missing section provenance"); }
                for (TopExp_Explorer edge(result,TopAbs_EDGE); edge.More(); edge.Next()) {
                    BRepAdaptor_Curve curve(TopoDS::Edge(edge.Current()));
                    GCPnts_QuasiUniformDeflection samples(curve,deflection);
                    if (!samples.IsDone()) { throw std::runtime_error("Cannot discretize curved section line"); }
                    RWCae::MeshEntityId previous = 0;
                    for (int n = 1; n <= samples.NbPoints(); ++n) {
                        const auto id = addNode(samples.Value(n));
                        if (n > 1) { lines.push_back({{previous,id},origin}); }
                        previous = id;
                    }
                }
            }
        }
    };
    // Unsupported types are rejected by the manager, never silently linearized.
    process(RWCae::MeshAdaptor<RWCae::MeshElementTriangle>(*mesh));
    process(RWCae::MeshAdaptor<RWCae::MeshElementTetrahedron>(*mesh));
    myMeshData = output;
    myNodesMap = nodesMap; myElementsMap = elementsMap; myLines = std::move(lines);
    myVolume = volume; myArea = area;
}
}

#include <ViewCaeGeom/MeshGeometryVtkCut.h>
#include <ViewCaeGeom/MeshVtkCutSource.h>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_MapOfShape.hxx>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
gp_Pnt point(const Graphic3d_Vec3d& p) { return gp_Pnt(p.x(),p.y(),p.z()); }
Graphic3d_Vec3d vector(const gp_Pnt& p) { return Graphic3d_Vec3d(p.X(),p.Y(),p.Z()); }
void addFaces(TopTools_MapOfShape& faces, const TopoDS_Shape& shape)
{
    if (shape.IsNull()) { return; }
    if (shape.ShapeType() == TopAbs_FACE) { faces.Add(shape); }
    else { for (TopExp_Explorer f(shape,TopAbs_FACE); f.More(); f.Next()) { faces.Add(f.Current()); } }
}
template<class Operation>
TopoDS_Shape intersect(const TopoDS_Shape& cell, const TopoDS_Shape& tool,
    const std::vector<TopoDS_Shape>& internal, TopTools_MapOfShape& hidden, ViewCaeGeom::MeshCutStatistics& work)
{
    Operation operation;
    TopTools_ListOfShape arguments, tools; arguments.Append(cell); tools.Append(tool);
    operation.SetArguments(arguments); operation.SetTools(tools); operation.SetNonDestructive(true);
    operation.Build();
    ++work.BooleanOperations;
    if (!operation.IsDone() || operation.HasErrors()) { throw std::runtime_error("OCCT VTK-cell intersection failed"); }
    for (const auto& face : internal) {
        addFaces(hidden,face);
        for (const auto& modified : operation.Modified(face)) { addFaces(hidden,modified); }
    }
    return operation.Shape();
}
}

namespace ViewCaeGeom {
void MeshGeometryVtkCut::ToJson(RWBase::JsonHelper::Writer&)
{
    throw std::logic_error("General VTK cuts must be saved through MeshCutViewManager");
}

void MeshGeometryVtkCut::generateMeshData()
{
    Handle(RWCae::MeshData) output = new RWCae::MeshData();
    NodesMap nodesMap; ElementsMap elementsMap;
    std::vector<MeshSliceLine> lines;
    const auto source = myParentGeometry->GetFirstParent();
    const auto mesh = source->GetMeshData();
    if (mesh->NbNodes() == 0) {
        myMeshData = output; myNodesMap.Clear(); myElementsMap.Clear(); myLines.clear(); myVolume = 0; myArea = 0;
        return;
    }
    const auto& definition = myDefinition;
    mySource->Prepare(mesh,source.get());
    const double tolerance = mySource->Tolerance();
    const double toolDeflection = mySourceOnly || definition.Shape == MeshCutShape::Plane ? tolerance
        : std::min(tolerance,definition.Radius*0.01);
    TopoDS_Shape tool, boundary;
    if (!mySourceOnly && definition.Shape == MeshCutShape::Plane) {
        const auto& p = definition.Plane;
        const gp_Pln plane(p.x(),p.y(),p.z(),p.w());
        boundary = BRepBuilderAPI_MakeFace(plane).Face();
        tool = BRepPrimAPI_MakeHalfSpace(TopoDS::Face(boundary),plane.Location().Translated(gp_Vec(plane.Axis().Direction())*-1)).Solid();
    } else if (!mySourceOnly && definition.Shape == MeshCutShape::Sphere) {
        tool = BRepPrimAPI_MakeSphere(point(definition.Center),definition.Radius).Shape();
        boundary = TopExp_Explorer(tool,TopAbs_SHELL).Current();
    } else if (!mySourceOnly && definition.Shape == MeshCutShape::Cylinder) {
        double minimum = INFINITY, maximum = -INFINITY;
        const auto& sourceBounds = mySource->Bounds();
        if (sourceBounds.IsVoid()) {
            myMeshData = output; myNodesMap.Clear(); myElementsMap.Clear(); myLines.clear(); myVolume = 0; myArea = 0; return;
        }
        const auto low = vector(sourceBounds.CornerMin()), high = vector(sourceBounds.CornerMax());
        minimum = maximum = -definition.Center.Dot(definition.Axis);
        for (int a = 0; a < 3; ++a) {
            const double x = definition.Axis[a]*low[a], y = definition.Axis[a]*high[a];
            minimum += std::min(x,y); maximum += std::max(x,y);
        }
        const double margin = std::max({maximum-minimum,definition.Radius,1.0});
        const auto start = definition.Center+definition.Axis*(minimum-margin);
        tool = BRepPrimAPI_MakeCylinder(gp_Ax2(point(start),gp_Dir(definition.Axis.x(),definition.Axis.y(),definition.Axis.z())),
            definition.Radius,maximum-minimum+2*margin).Shape();
        BRep_Builder builder; TopoDS_Shell shell; builder.MakeShell(shell);
        for (TopExp_Explorer f(tool,TopAbs_FACE); f.More(); f.Next()) {
            if (BRepAdaptor_Surface(TopoDS::Face(f.Current())).GetType() == GeomAbs_Cylinder) { builder.Add(shell,f.Current()); }
        }
        boundary = shell;
    }
    double volume = 0, area = 0;
    for (auto index : mySource->Query(definition,myPortion,mySourceOnly)) {
        auto& prepared = mySource->GetCell(index);
        const auto& cell = *prepared.Geometry;
        const auto& original = mesh->IndexedElements()[index];
        const MeshGeomElem origin(original.Index,original.Type,false);
        auto addNode = [&](const gp_Pnt& p) {
            for (double coordinate : {p.X(),p.Y(),p.Z()}) {
                if (!std::isfinite((float)coordinate)) { throw std::runtime_error("VTK cut coordinate exceeds float mesh storage"); }
            }
            const auto id = output->AddNodePosition(Graphic3d_Vec3((float)p.X(),(float)p.Y(),(float)p.Z()));
            nodesMap.Bind(id,std::make_pair(source.get(),cell.Origin(vector(p)))); return id;
        };
        auto addPoint = [&](const gp_Pnt& p) {
            const RWCae::MeshEntityId id[] = {addNode(p)};
            output->AddElement(RWCae::MeshElement0d(id));
            elementsMap.Bind(MeshGeomElem(output->NbElems(RWCae::MeshElemType::Elem0d)-1,RWCae::MeshElemType::Elem0d,false),
                std::make_pair(source.get(),origin));
        };
        for (const auto& p : cell.Points) {
            double distance = 0;
            if (definition.Shape == MeshCutShape::Plane) {
                const auto& plane = definition.Plane;
                distance = plane.x()*p.x()+plane.y()*p.y()+plane.z()*p.z()+plane.w();
            } else {
                auto d = p-definition.Center;
                if (definition.Shape == MeshCutShape::Cylinder) { d -= definition.Axis*d.Dot(definition.Axis); }
                distance = d.Modulus()-definition.Radius;
            }
            if (mySourceOnly || (myPortion == MeshCutPortion::On ? std::abs(distance) <= 1e-7
                : myPortion == MeshCutPortion::Negative ? distance <= 1e-7 : distance >= -1e-7)) { addPoint(point(p)); }
        }
        if (cell.Dimension() == 0) { continue; }
        // Bound the tessellated cell, not just its original high-order control points.
        const auto& cellBox = prepared.Bounds;
        if (cellBox.IsVoid()) { throw std::runtime_error("VTK cell has no geometric boundary"); }
        double minimum = INFINITY, maximum = -INFINITY;
        if (!mySourceOnly) {
            const auto low = vector(cellBox.CornerMin()), high = vector(cellBox.CornerMax());
            if (definition.Shape == MeshCutShape::Plane) {
                minimum = maximum = definition.Plane.w();
                for (int a = 0; a < 3; ++a) {
                    const double x = definition.Plane[a]*low[a], y = definition.Plane[a]*high[a];
                    minimum += std::min(x,y); maximum += std::max(x,y);
                }
            } else if (definition.Shape == MeshCutShape::Sphere) {
                double nearSquared = 0, farSquared = 0;
                for (int a = 0; a < 3; ++a) {
                    const double x = low[a]-definition.Center[a], y = high[a]-definition.Center[a];
                    nearSquared += x > 0 ? x*x : y < 0 ? y*y : 0;
                    farSquared += std::max(x*x,y*y);
                }
                minimum = std::sqrt(nearSquared)-definition.Radius; maximum = std::sqrt(farSquared)-definition.Radius;
            } else {
                auto d = (low+high)*0.5-definition.Center; d -= definition.Axis*d.Dot(definition.Axis);
                const double radius = (high-low).Modulus()*0.5;
                minimum = std::max(0.0,d.Modulus()-radius)-definition.Radius;
                maximum = d.Modulus()+radius-definition.Radius;
            }
            if ((myPortion == MeshCutPortion::On && (minimum > 1e-7 || maximum < -1e-7))
                || (myPortion == MeshCutPortion::Negative && minimum > 1e-7)
                || (myPortion == MeshCutPortion::Positive && maximum < -1e-7)) { continue; }
        }
        const auto& shape = mySource->Shape(index);
        std::vector<TopoDS_Shape> internal;
        for (const auto& primitive : prepared.PrimitiveFaces) {
            const auto& face = prepared.Faces.at(primitive.FaceIndex);
            if (face.Internal && (mySourceOnly || myPortion != MeshCutPortion::On || face.Owner != index)) { internal.push_back(primitive.Shape); }
        }
        TopTools_MapOfShape hidden;
        const bool uncut = mySourceOnly || (myPortion == MeshCutPortion::Negative && maximum < -1e-7)
            || (myPortion == MeshCutPortion::Positive && minimum > 1e-7);
        const auto result = uncut ? shape : myPortion == MeshCutPortion::Positive ? intersect<BRepAlgoAPI_Cut>(shape,tool,internal,hidden,mySource->Work)
            : myPortion == MeshCutPortion::On && cell.Dimension() < 3 ? intersect<BRepAlgoAPI_Section>(shape,boundary,internal,hidden,mySource->Work)
            : intersect<BRepAlgoAPI_Common>(shape,myPortion == MeshCutPortion::On ? boundary : tool,internal,hidden,mySource->Work);
        if (uncut) { for (const auto& face : internal) { addFaces(hidden,face); } }
        GProp_GProps properties;
        BRepGProp::SurfaceProperties(result,properties,1e-9); area += properties.Mass();
        if (cell.Dimension() == 3 && (mySourceOnly || myPortion != MeshCutPortion::On)) {
            BRepGProp::VolumeProperties(result,properties,1e-9); volume += properties.Mass();
        }
        BRepMesh_IncrementalMesh mesher(result,toolDeflection,false,0.25,false);
        for (TopExp_Explorer f(result,TopAbs_FACE); f.More(); f.Next()) {
            const auto face = TopoDS::Face(f.Current()); TopLoc_Location location;
            if (hidden.Contains(face)) {
                BRepGProp::SurfaceProperties(face,properties,1e-9); area -= properties.Mass(); ++mySource->Work.FilteredFaces; continue;
            }
            const auto triangles = BRep_Tool::Triangulation(face,location);
            if (triangles.IsNull()) { throw std::runtime_error("Missing VTK-cell boundary triangulation"); }
            std::vector<RWCae::MeshEntityId> ids;
            for (int n = 1; n <= triangles->NbNodes(); ++n) { ids.push_back(addNode(triangles->Node(n).Transformed(location.Transformation()))); }
            for (int t = 1; t <= triangles->NbTriangles(); ++t) {
                int a,b,c; triangles->Triangle(t).Get(a,b,c);
                if (face.Orientation() == TopAbs_REVERSED) { std::swap(b,c); }
                const RWCae::MeshEntityId nodes[] = {ids[a-1],ids[b-1],ids[c-1]}; output->AddElement(RWCae::MeshElementTriangle(nodes));
                elementsMap.Bind(MeshGeomElem(output->NbElems(RWCae::MeshElemType::Triangle)-1,RWCae::MeshElemType::Triangle,false),
                    std::make_pair(source.get(),origin));
            }
        }
        const bool hasFaces = TopExp_Explorer(result,TopAbs_FACE).More();
        if (!hasFaces) {
            const bool hasEdges = TopExp_Explorer(result,TopAbs_EDGE).More();
            for (TopExp_Explorer e(result,TopAbs_EDGE); e.More(); e.Next()) {
                BRepAdaptor_Curve curve(TopoDS::Edge(e.Current())); GCPnts_QuasiUniformDeflection sample(curve,toolDeflection);
                if (!sample.IsDone()) { throw std::runtime_error("Missing VTK-cell curve discretization"); }
                RWCae::MeshEntityId previous = 0;
                for (int n = 1; n <= sample.NbPoints(); ++n) {
                    const auto id = addNode(sample.Value(n));
                    if (n > 1) { lines.push_back({{previous,id},index}); }
                    previous = id;
                }
            }
            if (!hasEdges) {
                for (TopExp_Explorer v(result,TopAbs_VERTEX); v.More(); v.Next()) { addPoint(BRep_Tool::Pnt(TopoDS::Vertex(v.Current()))); }
            }
        }
    }
    myMeshData = output; myNodesMap = nodesMap; myElementsMap = elementsMap; myLines = std::move(lines);
    myVolume = volume; myArea = area;
}
}

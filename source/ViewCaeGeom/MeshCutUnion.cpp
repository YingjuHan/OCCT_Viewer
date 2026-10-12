#include <ViewCaeGeom/MeshCutUnion.h>
#include <ViewCaeGeom/MeshCutFieldTools.h>
#include <gp_Pln.hxx>
#include <RWCae/MeshAdaptorsIterator.h>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#ifdef OCCTV_USE_VTK
#include <ViewCaeGeom/MeshVtkCutSource.h>
#endif
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace ViewCaeGeom {
MeshCutPresentation::~MeshCutPresentation()
{
    for (const auto& root : OwnedRoots) { if (!root.IsNull()) { root->RemoveChildren(); } }
}
namespace {
using Vec = Graphic3d_Vec3d;
gp_Pnt point(const Vec& p) { return gp_Pnt(p.x(),p.y(),p.z()); }
Vec vector(const gp_Pnt& p) { return Vec(p.X(),p.Y(),p.Z()); }
std::vector<TopoDS_Shape> primitives(const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    std::vector<TopoDS_Shape> result; TopTools_MapOfShape seen;
    if (!shape.IsNull() && shape.ShapeType() == type) { result.push_back(shape); seen.Add(shape); }
    for (TopExp_Explorer it(shape,type); it.More(); it.Next()) {
        if (seen.Add(it.Current())) { result.push_back(it.Current()); }
    }
    return result;
}
TopoDS_Shape compound(const std::vector<TopoDS_Shape>& shapes)
{
    BRep_Builder builder; TopoDS_Compound result; builder.MakeCompound(result);
    for (const auto& shape : shapes) { if (!shape.IsNull()) { builder.Add(result,shape); } } return result;
}
TopoDS_Face polygon(const std::vector<Vec>& vertices)
{
    BRepBuilderAPI_MakePolygon wire;
    for (const auto& p : vertices) { wire.Add(point(p)); } wire.Close();
    BRepBuilderAPI_MakeFace face(wire.Wire());
    if (!face.IsDone()) { throw std::runtime_error("Cannot construct union source face"); } return face.Face();
}
TopoDS_Shape nativeShape(const Handle(RWCae::MeshData)& mesh, RWCae::MeshEntityId e, const Handle(MeshGeometry)& modifier)
{
    const auto& indexed = mesh->IndexedElements()[e];
    std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
    auto position = [&](RWCae::MeshEntityId n) {
        auto p = mesh->Nodes()[n]; if (!modifier.IsNull()) { modifier->ModifyNode(n,p); } return Vec(p);
    };
    if (indexed.Type == RWCae::MeshElemType::Elem0d) {
        std::vector<TopoDS_Shape> vertices;
        for (auto n : nodes) { vertices.push_back(BRepBuilderAPI_MakeVertex(point(position(n))).Vertex()); }
        return compound(vertices);
    }
    if ((int)indexed.Type < RWCae::MeshElem3dLowerType) {
        std::vector<Vec> vertices; for (auto n : nodes) { vertices.push_back(position(n)); } return polygon(vertices);
    }
    BRepBuilderAPI_Sewing sewing(1e-7);
    RWCae::MeshAdaptorsIterator3dElems::Switch(indexed.Type,[&](const auto& adaptor) {
        const auto& element = adaptor.GetElement(indexed.Index);
        for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(element); ++f) {
            std::vector<RWCae::MeshEntityId> face; RWCae::MeshElemType type;
            adaptor.FaceNodes(element,f,face,type);
            // Native fast geometry is piecewise planar; retain the same corner fan.
            for (size_t n = 1; n+1 < face.size(); ++n) { sewing.Add(polygon({position(face[0]),position(face[n]),position(face[n+1])})); }
        }
        return true;
    },mesh);
    sewing.Perform(); BRepBuilderAPI_MakeSolid maker;
    for (const auto& shell : primitives(sewing.SewedShape(),TopAbs_SHELL)) { maker.Add(TopoDS::Shell(shell)); }
    auto solid = maker.Solid();
    if (!BRepLib::OrientClosedSolid(solid) || !BRepCheck_Analyzer(solid).IsValid()) {
        throw std::runtime_error("Union requires a closed orientable source cell");
    }
    return solid;
}

struct Owner {
    size_t Cut = 0;
    MeshCutPortion Portion = MeshCutPortion::Negative;
    RWCae::MeshEntityId Element = 0;
    bool Cap = false;
};
struct TaggedPrimitive { TopoDS_Shape Shape; Owner Origin; };
struct TaggedShape { TopoDS_Shape Shape; std::vector<TaggedPrimitive> Tags; };
TopAbs_ShapeEnum boundaryType(const TopoDS_Shape& shape)
{
    if (!primitives(shape,TopAbs_FACE).empty()) { return TopAbs_FACE; }
    if (!primitives(shape,TopAbs_EDGE).empty()) { return TopAbs_EDGE; } return TopAbs_VERTEX;
}
TaggedShape tagged(const TopoDS_Shape& shape, const Owner& owner)
{
    TaggedShape result; result.Shape = shape;
    for (const auto& primitive : primitives(shape,boundaryType(shape))) { result.Tags.push_back({primitive,owner}); } return result;
}
bool preferred(const Owner& a, const Owner& b)
{
    return std::make_tuple(a.Portion != MeshCutPortion::On,a.Cut,(int)a.Portion,a.Element)
         < std::make_tuple(b.Portion != MeshCutPortion::On,b.Cut,(int)b.Portion,b.Element);
}
template<class Operation> TaggedShape boolean(const TaggedShape& a, const TaggedShape& b)
{
    if (a.Tags.empty()) { return std::is_same<Operation,BRepAlgoAPI_Fuse>::value ? b : TaggedShape(); }
    if (b.Tags.empty()) { return std::is_same<Operation,BRepAlgoAPI_Common>::value ? TaggedShape() : a; }
    Operation operation; TopTools_ListOfShape arguments, tools;
    arguments.Append(a.Shape); tools.Append(b.Shape);
    operation.SetArguments(arguments); operation.SetTools(tools); operation.SetNonDestructive(true);
    operation.SetToFillHistory(true); operation.Build();
    if (!operation.IsDone() || operation.HasErrors()) { throw std::runtime_error("Presentation OR boolean failed"); }
    TaggedShape result; result.Shape = operation.Shape();
    for (const auto& output : primitives(result.Shape,boundaryType(result.Shape))) {
        bool found = false; Owner owner;
        auto consider = [&](const TaggedPrimitive& input) {
            bool descendant = input.Shape.IsSame(output);
            for (const auto& modified : operation.Modified(input.Shape)) { descendant |= modified.IsSame(output); }
            for (const auto& generated : operation.Generated(input.Shape)) { descendant |= generated.IsSame(output); }
            if (descendant && (!found || preferred(input.Origin,owner))) { found = true; owner = input.Origin; }
        };
        for (const auto& tag : a.Tags) { consider(tag); }
        for (const auto& tag : b.Tags) { consider(tag); }
        if (!found) { throw std::runtime_error("Presentation union lost primitive history"); }
        result.Tags.push_back({output,owner});
    }
    return result;
}
TaggedShape fuse(const std::vector<TaggedShape>& shapes)
{
    std::vector<const TaggedShape*> inputs;
    for (const auto& shape : shapes) { if (!shape.Tags.empty()) { inputs.push_back(&shape); } }
    if (inputs.empty()) { return {}; }
    if (inputs.size() == 1) { return *inputs[0]; }
    if (inputs.size() == 2) { return boolean<BRepAlgoAPI_Fuse>(*inputs[0],*inputs[1]); }
    BRepAlgoAPI_Fuse operation; TopTools_ListOfShape arguments, tools;
    arguments.Append(inputs[0]->Shape);
    for (size_t n = 1; n < inputs.size(); ++n) { tools.Append(inputs[n]->Shape); }
    operation.SetArguments(arguments); operation.SetTools(tools); operation.SetNonDestructive(true);
    operation.SetToFillHistory(true); operation.Build();
    if (!operation.IsDone() || operation.HasErrors()) { throw std::runtime_error("Presentation OR fuse failed"); }
    TaggedShape result; result.Shape = operation.Shape();
    for (const auto& output : primitives(result.Shape,boundaryType(result.Shape))) {
        bool found = false; Owner owner;
        for (const auto input : inputs) { for (const auto& tag : input->Tags) {
            bool descendant = tag.Shape.IsSame(output);
            for (const auto& modified : operation.Modified(tag.Shape)) { descendant |= modified.IsSame(output); }
            for (const auto& generated : operation.Generated(tag.Shape)) { descendant |= generated.IsSame(output); }
            if (descendant && (!found || preferred(tag.Origin,owner))) { owner = tag.Origin; found = true; }
        } }
        if (!found) { throw std::runtime_error("Presentation OR fuse lost primitive history"); }
        result.Tags.push_back({output,owner});
    }
    return result;
}
TopoDS_Shape cuttingTool(const MeshCutDefinition& cut, const Bnd_Box& bounds)
{
    if (cut.Shape == MeshCutShape::Plane) {
        const auto p = cut.Plane; const gp_Pln plane(p.x(),p.y(),p.z(),p.w());
        const auto face = BRepBuilderAPI_MakeFace(plane).Face();
        return BRepPrimAPI_MakeHalfSpace(face,plane.Location().Translated(gp_Vec(plane.Axis().Direction())*-1)).Solid();
    }
    if (cut.Shape == MeshCutShape::Sphere) { return BRepPrimAPI_MakeSphere(point(cut.Center),cut.Radius).Shape(); }
    if (cut.Shape == MeshCutShape::Cylinder) {
        const double scale = bounds.IsVoid() ? 1 : (bounds.CornerMax().XYZ()-bounds.CornerMin().XYZ()).Modulus();
        const auto center = bounds.IsVoid() ? cut.Center : (vector(bounds.CornerMax())+vector(bounds.CornerMin()))*0.5;
        const double along = (center-cut.Center).Dot(cut.Axis), margin = std::max({scale,cut.Radius,1.0});
        return BRepPrimAPI_MakeCylinder(gp_Ax2(point(cut.Center+cut.Axis*(along-margin)),gp_Dir(cut.Axis.x(),cut.Axis.y(),cut.Axis.z())),cut.Radius,2*margin).Shape();
    }
    throw std::logic_error("Isosurface unions use the existing numerical volume primitives");
}

struct CellEvaluator {
    std::vector<RWCae::MeshEntityId> Nodes;
    std::vector<Vec> Positions;
    RWCae::MeshElemType Type = RWCae::MeshElemType::Elem0d;
    int Dimension = 0;
#ifdef OCCTV_USE_VTK
    std::unique_ptr<MeshVtkCell> Owned;
    MeshVtkCell* Vtk = nullptr;
#endif
    MeshInterpolatedNode origin(const Vec& p) const
    {
#ifdef OCCTV_USE_VTK
        if (Dimension) { return Vtk->Origin(p); }
#endif
        MeshInterpolatedNode result;
        if (!Dimension) {
            if (!Nodes.empty()) {
                size_t nearest = 0;
                for (size_t n = 1; n < Nodes.size(); ++n) {
                    if ((Positions[n]-p).SquareModulus() < (Positions[nearest]-p).SquareModulus()) { nearest = n; }
                }
                result.AddBaseNode(Nodes[nearest],1);
            }
            return result;
        }
        if (Type == RWCae::MeshElemType::Triangle || Type == RWCae::MeshElemType::Tetrahedron) {
            const auto a = Positions[1]-Positions[0], b = Positions[2]-Positions[0], d = p-Positions[0];
            double w[4] = {};
            if (Dimension == 3) {
                const auto c = Positions[3]-Positions[0]; const double determinant = a.Dot(Vec::Cross(b,c));
                if (std::abs(determinant) <= 1e-24) { throw std::runtime_error("Degenerate union FE source"); }
                w[1] = d.Dot(Vec::Cross(b,c))/determinant; w[2] = a.Dot(Vec::Cross(d,c))/determinant; w[3] = a.Dot(Vec::Cross(b,d))/determinant;
            } else {
                const double aa = a.Dot(a), ab = a.Dot(b), bb = b.Dot(b), determinant = aa*bb-ab*ab;
                if (determinant <= 1e-24) { throw std::runtime_error("Degenerate union FE source"); }
                w[1] = (bb*d.Dot(a)-ab*d.Dot(b))/determinant; w[2] = (aa*d.Dot(b)-ab*d.Dot(a))/determinant;
            }
            w[0] = 1-w[1]-w[2]-w[3];
            for (size_t n = 0; n < Nodes.size(); ++n) { result.AddBaseNode(Nodes[n],w[n]); } return result;
        }
        // Native linear chart cells use their own shape functions; no coordinate welding.
        auto weights = [&](const Vec& q) {
            std::vector<double> w(Nodes.size(),0);
            const double r = q.x(), s = q.y(), t = q.z();
            if (Type == RWCae::MeshElemType::Quad || Type == RWCae::MeshElemType::Hexahedron) {
                constexpr int signs[8][3] = {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
                for (size_t n = 0; n < w.size(); ++n) { w[n] = (1+signs[n][0]*r)*(1+signs[n][1]*s)*0.25*(Dimension == 3 ? (1+signs[n][2]*t)*0.5 : 1); }
            } else if (Type == RWCae::MeshElemType::Prism) {
                const double triangle[] = {1-r-s,r,s}; for (int n = 0; n < 6; ++n) { w[n] = triangle[n%3]*(n < 3 ? 1-t : t); }
            } else if (Type == RWCae::MeshElemType::Pyramid) {
                w = {(1-r)*(1-s)*(1-t)*0.25,(1+r)*(1-s)*(1-t)*0.25,(1+r)*(1+s)*(1-t)*0.25,(1-r)*(1+s)*(1-t)*0.25,t};
            } else { throw std::invalid_argument("This union FE source requires VTK chart evaluation"); }
            return w;
        };
        auto position = [&](const Vec& q) { const auto w = weights(q); Vec value(0); for (size_t n = 0; n < w.size(); ++n) { value += Positions[n]*w[n]; } return value; };
        Vec q(0); if (Type == RWCae::MeshElemType::Prism) { q = Vec(1.0/3,1.0/3,0.5); } if (Type == RWCae::MeshElemType::Pyramid) { q.z() = 0.25; }
        for (int iteration = 0; iteration < 40; ++iteration) {
            const auto d = p-position(q); if (d.Modulus() <= 1e-8) { break; }
            Vec derivative[3]; const double h = 1e-5;
            for (int axis = 0; axis < Dimension; ++axis) { auto low = q, high = q; low[axis] -= h; high[axis] += h; derivative[axis] = (position(high)-position(low))/(2*h); }
            if (Dimension == 3) {
                const double determinant = derivative[0].Dot(Vec::Cross(derivative[1],derivative[2]));
                if (std::abs(determinant) <= 1e-24) { throw std::runtime_error("Singular union FE chart"); }
                q += Vec(d.Dot(Vec::Cross(derivative[1],derivative[2])),derivative[0].Dot(Vec::Cross(d,derivative[2])),derivative[0].Dot(Vec::Cross(derivative[1],d)))/determinant;
            } else {
                const double aa = derivative[0].Dot(derivative[0]), bb = derivative[1].Dot(derivative[1]), ab = derivative[0].Dot(derivative[1]), determinant = aa*bb-ab*ab;
                if (determinant <= 1e-24) { throw std::runtime_error("Singular union FE chart"); }
                q.x() += (bb*d.Dot(derivative[0])-ab*d.Dot(derivative[1]))/determinant;
                q.y() += (aa*d.Dot(derivative[1])-ab*d.Dot(derivative[0]))/determinant;
            }
        }
        if ((position(q)-p).Modulus() > 1e-5) { throw std::runtime_error("Union FE inverse did not converge"); }
        const auto w = weights(q); for (size_t n = 0; n < w.size(); ++n) { result.AddBaseNode(Nodes[n],w[n]); } return result;
    }
};

class UnionGeometry : public MeshGeometrySlice {
public:
    UnionGeometry(const Handle(MeshBvhData)& bvh, const Handle(MeshGeometry)& source)
        : MeshGeometrySlice(bvh), Source(source) { myMeshData = new RWCae::MeshData(); myIsUpdated = true; }
    RWCae::MeshEntityId node(const Vec& p, const CellEvaluator& cell)
    {
        for (double value : {p.x(),p.y(),p.z()}) { if (!std::isfinite((float)value)) { throw std::runtime_error("Union coordinate exceeds mesh storage"); } }
        const auto n = myMeshData->AddNodePosition(Graphic3d_Vec3(p)); myNodesMap.Bind(n,std::make_pair(Source.get(),cell.origin(p))); return n;
    }
    void bind(const Owner& owner)
    {
        const auto& added = myMeshData->IndexedElements()[myMeshData->NbIndexedElems()-1];
        const auto& original = Source->GetMeshData()->IndexedElements()[owner.Element];
        myElementsMap.Bind(MeshGeomElem(added.Index,added.Type,false),std::make_pair(Source.get(),MeshGeomElem(original.Index,original.Type,false)));
        ElementOwners.push_back(owner);
    }
    void triangle(const std::array<Vec,3>& p, const Owner& owner, const CellEvaluator& cell)
    {
        if (Vec::Cross(p[1]-p[0],p[2]-p[0]).Modulus() <= 1e-18) { return; }
        const RWCae::MeshEntityId nodes[] = {node(p[0],cell),node(p[1],cell),node(p[2],cell)};
        myMeshData->AddElement(RWCae::MeshElementTriangle(nodes)); bind(owner);
    }
    void line(const Vec& a, const Vec& b, const Owner& owner, const CellEvaluator& cell)
    {
        if ((a-b).Modulus() <= 1e-12) { return; }
        myLines.push_back({{node(a,cell),node(b,cell)},owner.Element}); LineOwners.push_back(owner);
    }
    void vertex(const Vec& p, const Owner& owner, const CellEvaluator& cell)
    {
        const RWCae::MeshEntityId n[] = {node(p,cell)}; myMeshData->AddElement(RWCae::MeshElement0d(n)); bind(owner);
    }
    std::vector<Owner> ElementOwners, LineOwners;
protected:
    void generateMeshData() override {}
    void setToUpdate() override {}
private:
    Handle(MeshGeometry) Source;
};

std::vector<double> isoNodes(const Handle(RWCae::MeshData)& mesh, const MeshCutDefinition& cut)
{
    if (cut.Shape != MeshCutShape::Isosurface) { return {}; }
    Handle(RWCae::MeshNVecProperty) field;
    for (const auto& candidate : mesh->Properties()) {
        if (candidate->Name() == cut.IsoFieldName.c_str() && candidate->IsNodal() == cut.IsoFieldNodal && candidate->TimeStep() == cut.IsoTimeStep) { field = candidate; }
    }
    if (field.IsNull()) { throw std::invalid_argument("Missing union isosurface field"); }
    const auto component = field->GetRawComponent(cut.IsoComponent);
    std::vector<double> values(mesh->NbNodes(),0); std::vector<unsigned> counts(values.size(),0);
    if (field->IsNodal()) { for (size_t n = 0; n < values.size(); ++n) { values[n] = component->Value(RWCae::MeshPropertyEntityInfo(n)); } }
    else {
        for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
            std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes); std::sort(nodes.begin(),nodes.end()); nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());
            const double value = component->Value(RWCae::MeshPropertyEntityInfo(e));
            if (!std::isfinite(value)) { throw std::invalid_argument("Union cutting field must be finite"); }
            for (auto n : nodes) { values[n] = CutFieldTools::Mean(values[n],value,++counts[n]); }
        }
    }
    return values;
}
double signedCut(const MeshCutDefinition& cut, const Vec& p, const MeshInterpolatedNode& weights, const std::vector<double>& iso)
{
    if (cut.Shape == MeshCutShape::Plane) { return cut.Plane.x()*p.x()+cut.Plane.y()*p.y()+cut.Plane.z()*p.z()+cut.Plane.w(); }
    if (cut.Shape == MeshCutShape::Isosurface) {
        double value = 0; for (const auto& w : weights.BaseNodes()) { if (w.second != 0) { value += w.second*iso.at(w.first); } } return value-cut.IsoValue;
    }
    auto radial = p-cut.Center; if (cut.Shape == MeshCutShape::Cylinder) { radial -= cut.Axis*radial.Dot(cut.Axis); } return radial.Modulus()-cut.Radius;
}

void identifyCaps(TaggedShape& shape, const MeshCutResult& cut, const CellEvaluator& cell,
    const std::vector<double>& iso, double geometryTolerance)
{
    const auto& definition = cut.Definition;
    const double tolerance = definition.Shape == MeshCutShape::Isosurface ? std::max(1e-10,cut.IsoFieldTolerance) : std::max(1e-7,geometryTolerance);
    for (auto& tag : shape.Tags) {
        bool cap = false;
        if (tag.Shape.ShapeType() == TopAbs_FACE && (definition.Shape == MeshCutShape::Sphere || definition.Shape == MeshCutShape::Cylinder)) {
            BRepAdaptor_Surface surface(TopoDS::Face(tag.Shape));
            if (definition.Shape == MeshCutShape::Sphere && surface.GetType() == GeomAbs_Sphere) {
                const auto sphere = surface.Sphere(); cap = std::abs(sphere.Radius()-definition.Radius) <= tolerance && (vector(sphere.Location())-definition.Center).Modulus() <= tolerance;
            } else if (definition.Shape == MeshCutShape::Cylinder && surface.GetType() == GeomAbs_Cylinder) {
                const auto cylinder = surface.Cylinder(); auto offset = vector(cylinder.Location())-definition.Center; offset -= definition.Axis*offset.Dot(definition.Axis);
                cap = std::abs(cylinder.Radius()-definition.Radius) <= tolerance && offset.Modulus() <= tolerance
                    && std::abs(std::abs(vector(gp_Pnt(cylinder.Axis().Direction().XYZ())).Dot(definition.Axis))-1) <= 1e-7;
            }
        } else {
            const auto vertices = primitives(tag.Shape,TopAbs_VERTEX); cap = !vertices.empty();
            for (const auto& vertex : vertices) {
                const auto p = vector(BRep_Tool::Pnt(TopoDS::Vertex(vertex)));
                const auto weights = definition.Shape == MeshCutShape::Isosurface ? cell.origin(p) : MeshInterpolatedNode();
                cap &= std::abs(signedCut(definition,p,weights,iso)) <= tolerance;
            }
        }
        if (cap) { tag.Origin.Cap = true; if (definition.Visibility&2) { tag.Origin.Portion = MeshCutPortion::On; } }
    }
}

// Replacement simplices from one numerical iso cell are a conforming partition.
// Cancel only equal generated topology faces within that original cell, then sew
// their boundary. Actual cross-cut overlap is still split by the OR booleans.
TaggedShape isoRegion(const Handle(MeshGeometry)& geometry, RWCae::MeshEntityId sourceId, const Owner& owner)
{
    const auto data = geometry->GetMeshData();
    struct Face { std::vector<RWCae::MeshEntityId> Nodes; unsigned Count = 0; };
    std::map<std::vector<RWCae::MeshEntityId>,Face> faces;
    std::vector<TopoDS_Shape> surfaces;
    bool volume = false;
    for (RWCae::MeshEntityId e = 0; e < data->NbIndexedElems(); ++e) {
        const auto& indexed = data->IndexedElements()[e]; RWCae::MeshEntityId original = 0;
        if (!geometry->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),original) || original != sourceId) { continue; }
        if ((int)indexed.Type >= RWCae::MeshElem3dLowerType) {
            volume = true;
            RWCae::MeshAdaptorsIterator3dElems::Switch(indexed.Type,[&](const auto& adaptor) {
                const auto& element = adaptor.GetElement(indexed.Index);
                for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(element); ++f) {
                    std::vector<RWCae::MeshEntityId> nodes; RWCae::MeshElemType type; adaptor.FaceNodes(element,f,nodes,type);
                    auto key = nodes; std::sort(key.begin(),key.end()); auto& face = faces[key]; face.Nodes = nodes;
                    if (++face.Count > 2) { throw std::runtime_error("Non-manifold iso replacement partition"); }
                }
                return true;
            },data);
        } else { surfaces.push_back(nativeShape(data,e,nullptr)); }
    }
    BRepBuilderAPI_Sewing sewing(1e-7);
    for (const auto& entry : faces) { if (entry.second.Count == 1) {
        std::vector<Vec> positions; for (auto n : entry.second.Nodes) { positions.emplace_back(data->Nodes()[n]); }
        sewing.Add(polygon(positions));
    } }
    for (const auto& surface : surfaces) { sewing.Add(surface); }
    if (!faces.empty() || !surfaces.empty()) { sewing.Perform(); }
    std::vector<TopoDS_Shape> shapes;
    if (volume) {
        for (const auto& shell : primitives(sewing.SewedShape(),TopAbs_SHELL)) {
            auto solid = BRepBuilderAPI_MakeSolid(TopoDS::Shell(shell)).Solid();
            if (!BRepLib::OrientClosedSolid(solid) || !BRepCheck_Analyzer(solid).IsValid()) {
                throw std::runtime_error("Invalid iso replacement region boundary");
            }
            shapes.push_back(solid);
        }
        if (!faces.empty() && shapes.empty()) { throw std::runtime_error("Iso replacement region did not close"); }
    } else if (!surfaces.empty()) { shapes.push_back(sewing.SewedShape()); }
    const auto slice = Handle(MeshGeometrySlice)::DownCast(geometry);
    if (!slice.IsNull()) { for (const auto& line : slice->Lines()) {
        if (line.SourceElement == sourceId) { shapes.push_back(BRepBuilderAPI_MakeEdge(point(Vec(data->Nodes()[line.Nodes[0]])),point(Vec(data->Nodes()[line.Nodes[1]]))).Edge()); }
    } }
    return tagged(compound(shapes),owner);
}

std::vector<TaggedShape> sectionSeeds(const MeshCutResult& cut, size_t index, RWCae::MeshEntityId sourceId, int dimension)
{
    std::vector<TaggedShape> result; const auto section = Handle(MeshGeometrySlice)::DownCast(cut.Portions[1]);
    if (section.IsNull()) { return result; } const auto data = section->GetMeshData();
    const Owner owner{index,MeshCutPortion::On,sourceId,true};
    for (RWCae::MeshEntityId e = 0; e < data->NbIndexedElems(); ++e) {
        const auto& indexed = data->IndexedElements()[e]; RWCae::MeshEntityId original = 0;
        if (!section->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),original) || original != sourceId) { continue; }
        std::vector<RWCae::MeshEntityId> nodes; data->ElementNodes(e,nodes);
        if (indexed.Type == RWCae::MeshElemType::Elem0d) {
            if (dimension == 0) { result.push_back(tagged(BRepBuilderAPI_MakeVertex(point(Vec(data->Nodes()[nodes[0]]))).Vertex(),owner)); }
        } else if (dimension == 2) {
            std::vector<Vec> vertices; for (auto n : nodes) { vertices.emplace_back(data->Nodes()[n]); } result.push_back(tagged(polygon(vertices),owner));
        }
    }
    if (dimension == 1) {
        for (const auto& line : section->Lines()) {
            if (line.SourceElement == sourceId) { result.push_back(tagged(BRepBuilderAPI_MakeEdge(point(Vec(data->Nodes()[line.Nodes[0]])),point(Vec(data->Nodes()[line.Nodes[1]]))).Edge(),owner)); }
        }
    }
    // Within one independent section the primitives already form a non-overlapping
    // tessellation. Keep its history in one argument rather than pairwise fusing it.
    if (result.size() > 1) {
        TaggedShape joined; std::vector<TopoDS_Shape> shapes;
        for (const auto& piece : result) { shapes.push_back(piece.Shape); joined.Tags.insert(joined.Tags.end(),piece.Tags.begin(),piece.Tags.end()); }
        joined.Shape = compound(shapes); return {joined};
    }
    return result;
}

TaggedShape lowerBoundary(const TaggedShape& retained, int dimension)
{
    TaggedShape result; std::vector<TopoDS_Shape> shapes; TopTools_IndexedMapOfShape seen;
    std::vector<int> counts; std::vector<Owner> owners;
    for (const auto& tag : retained.Tags) {
        for (const auto& primitive : primitives(tag.Shape,dimension == 2 ? TopAbs_EDGE : TopAbs_VERTEX)) {
            const auto index = seen.Add(primitive);
            if (index > (int)counts.size()) { counts.push_back(0); owners.push_back(tag.Origin); }
            ++counts[index-1];
        }
    }
    for (int n = 1; n <= seen.Extent(); ++n) {
        if (counts[n-1] == 1) { shapes.push_back(seen(n)); result.Tags.push_back({seen(n),owners[n-1]}); }
    }
    result.Shape = compound(shapes); return result;
}
}

std::shared_ptr<MeshCutPresentation> BuildCutPresentationUnion(
    const Handle(RWCae::MeshData)& mesh, const Handle(MeshGeometry)& source,
    const Handle(MeshBvhData)& bvh, const std::vector<MeshCutResult>& cuts,
    const MeshCutGeometryOptions& options, const std::shared_ptr<MeshCutTraceSource>& topology,
    const std::shared_ptr<MeshVtkCutSource>& prepared)
{
    auto artifact = std::make_shared<MeshCutPresentation>(); artifact->Branches = cuts;
    std::sort(artifact->Branches.begin(),artifact->Branches.end(),[](const auto& a,const auto& b) { return a.Definition.Id < b.Definition.Id; });
    artifact->OwnedRoots.push_back(source);
    if (cuts.empty()) { return artifact; }
    // A single section has no retained-region boundary or cross-cut cap union.
    // Its trace section already deduplicates a coincident trusted source face.
    if(cuts.size()==1 && cuts.front().Definition.Visibility==2 && cuts.front().Definition.Shape==MeshCutShape::Plane) {
        const auto& cut=cuts.front(); const auto data=cut.Portions[1]->GetMeshData();
        for(RWCae::MeshEntityId e=0;e<data->NbIndexedElems();++e) {
            std::vector<RWCae::MeshEntityId> nodes; data->ElementNodes(e,nodes);
            if(data->IndexedElements()[e].Type!=RWCae::MeshElemType::Elem0d) {
                for(size_t n=1;n+1<nodes.size();++n) { artifact->Area+=Vec::Cross(Vec(data->Nodes()[nodes[n]])-Vec(data->Nodes()[nodes[0]]),Vec(data->Nodes()[nodes[n+1]])-Vec(data->Nodes()[nodes[0]])).Modulus()/2; }
            }
            const auto owner=cut.PhysicalProvenance ? cut.PhysicalProvenance(e,false).Element : 0;
            artifact->Provenance.push_back({cut.Definition.Id,MeshCutPortion::On,owner,true,data->IndexedElements()[e].Type==RWCae::MeshElemType::Elem0d?0:2,e});
        }
        const auto slice=Handle(MeshGeometrySlice)::DownCast(cut.Portions[1]);
        if(!slice.IsNull()) { for(size_t l=0;l<slice->Lines().size();++l) { artifact->Provenance.push_back({cut.Definition.Id,MeshCutPortion::On,slice->Lines()[l].SourceElement,true,1,(RWCae::MeshEntityId)l}); } }
        artifact->Unified=true; return artifact;
    }
    artifact->Unified = true;
    std::vector<CellEvaluator> cells(mesh->NbIndexedElems()); Bnd_Box bounds;
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        auto& cell = cells[e]; const auto& indexed = mesh->IndexedElements()[e]; cell.Type = indexed.Type; mesh->ElementNodes(e,cell.Nodes);
        for (auto n : cell.Nodes) { auto p = mesh->Nodes()[n]; source->ModifyNode(n,p); cell.Positions.emplace_back(p); bounds.Add(point(Vec(p))); }
    }
    const double tolerance = options.Tolerance(bounds.IsVoid() ? 0 : (bounds.CornerMax().XYZ()-bounds.CornerMin().XYZ()).Modulus());
#ifdef OCCTV_USE_VTK
    auto sourceShapes = prepared ? prepared : std::make_shared<MeshVtkCutSource>(1,options);
    const bool needsSourceShapes = std::any_of(cuts.begin(),cuts.end(),[](const auto& cut) {
        const auto shape = cut.Definition.Shape;
        return shape != MeshCutShape::Isosurface
            && ((cut.Definition.Visibility&5) || shape == MeshCutShape::Sphere || shape == MeshCutShape::Cylinder);
    });
    if (needsSourceShapes) { sourceShapes->Prepare(mesh,source.get()); }
#else
    (void)prepared;
#endif
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        auto& cell = cells[e];
#ifdef OCCTV_USE_VTK
        if (sourceShapes->Work.PreparedCells == mesh->NbIndexedElems()) { cell.Vtk = sourceShapes->GetCell(e).Geometry.get(); }
        else { cell.Owned.reset(new MeshVtkCell(mesh,e,source.get(),tolerance,options,0,false)); cell.Vtk = cell.Owned.get(); }
        cell.Dimension = cell.Vtk->Dimension();
#else
        cell.Dimension = cell.Type == RWCae::MeshElemType::Elem0d ? 0 : (int)cell.Type >= RWCae::MeshElem3dLowerType ? 3 : 2;
#endif
    }
    std::vector<TopoDS_Shape> bodies(cells.size());
    auto body = [&](RWCae::MeshEntityId e) -> const TopoDS_Shape& {
        if (bodies[e].IsNull()) {
            if (cells[e].Dimension == 0) {
                std::vector<TopoDS_Shape> vertices;
                for (const auto& p : cells[e].Positions) { vertices.push_back(BRepBuilderAPI_MakeVertex(point(p)).Vertex()); }
                bodies[e] = compound(vertices); return bodies[e];
            }
#ifdef OCCTV_USE_VTK
            if (sourceShapes->Work.PreparedCells == mesh->NbIndexedElems()) { bodies[e] = sourceShapes->Shape(e); }
            else
#endif
            { bodies[e] = nativeShape(mesh,e,source); }
        }
        return bodies[e];
    };
    const size_t count = artifact->Branches.size(); std::vector<std::vector<double>> iso(count);
    std::vector<TopoDS_Shape> tools(count);
    for (size_t c = 0; c < count; ++c) {
        const auto& cut = artifact->Branches[c].Definition;
        if (cut.Shape == MeshCutShape::Isosurface) { iso[c] = isoNodes(mesh,cut); }
        else { tools[c] = cuttingTool(cut,bounds); }
    }
    std::vector<TaggedShape> retained(cells.size());
    for (RWCae::MeshEntityId e = 0; e < cells.size(); ++e) {
        std::vector<TaggedShape> branches;
        for (size_t c = 0; c < count; ++c) {
            const auto& cut = artifact->Branches[c];
            for (int part : {0,2}) {
                if (!(cut.Definition.Visibility&(1u<<part))) { continue; }
                const Owner owner{c,(MeshCutPortion)part,e,false}; TaggedShape branch;
                if (cut.Definition.Shape == MeshCutShape::Isosurface) {
                    std::vector<TaggedShape> fragments; const auto geometry = cut.Portions[part];
                    if (geometry->IsModifier() && cut.Roots[part]->AcceptElement(e)) { fragments.push_back(tagged(body(e),owner)); }
                    fragments.push_back(isoRegion(geometry,e,owner));
                    branch = fuse(fragments); identifyCaps(branch,cut,cells[e],iso[c],options.AbsoluteTolerance);
                } else {
                    const auto original = tagged(body(e),owner);
                    Owner cap = owner; cap.Cap = true; if (cut.Definition.Visibility&2) { cap.Portion = MeshCutPortion::On; }
                    const auto tool = tagged(tools[c],cap);
                    branch = part == 0 ? boolean<BRepAlgoAPI_Common>(original,tool) : boolean<BRepAlgoAPI_Cut>(original,tool);
                    identifyCaps(branch,cut,cells[e],iso[c],options.AbsoluteTolerance);
                }
                const auto required = cells[e].Dimension == 3 ? TopAbs_SOLID : cells[e].Dimension == 2 ? TopAbs_FACE : cells[e].Dimension == 1 ? TopAbs_EDGE : TopAbs_VERTEX;
                if (!primitives(branch.Shape,required).empty()) { branches.push_back(std::move(branch)); }
            }
        }
        retained[e] = fuse(branches);
        if (cells[e].Dimension == 3 && !retained[e].Tags.empty()) {
            GProp_GProps volume; BRepGProp::VolumeProperties(retained[e].Shape,volume); artifact->Volume += volume.Mass();
        }
    }
    std::vector<std::array<Handle(UnionGeometry),3>> outputs(count);
    for (size_t c = 0; c < count; ++c) {
        auto& cut = artifact->Branches[c];
        cut.PhysicalFields = {}; cut.PhysicalLineField.Nullify(); cut.PhysicalProvenance = nullptr;
        cut.LineField.Nullify(); cut.Fields = {};
        cut.Roots = {}; cut.Portions = {}; cut.BoundaryMesh = true; cut.PresentationOnly = true;
        for (int part = 0; part < 3; ++part) {
            if (!(cut.Definition.Visibility&(1u<<part))) { continue; }
            outputs[c][part] = new UnionGeometry(bvh,source);
        }
    }
    auto emit = [&](const TaggedShape& shape, int dimension, const CellEvaluator& cell) {
        if (shape.Tags.empty()) { return; }
        double deflection = tolerance;
        for (const auto& tag : shape.Tags) {
            const auto& definition = artifact->Branches[tag.Origin.Cut].Definition;
            if (tag.Origin.Cap && (definition.Shape == MeshCutShape::Sphere || definition.Shape == MeshCutShape::Cylinder)) {
                deflection = std::min(deflection,definition.Radius*0.01);
            }
        }
        BRepMesh_IncrementalMesh mesher(shape.Shape,deflection,false,0.25,false);
        for (const auto& tag : shape.Tags) {
            auto geometry = outputs[tag.Origin.Cut][(int)tag.Origin.Portion];
            if (geometry.IsNull()) { throw std::logic_error("Union ownership selected a hidden portion"); }
            if (dimension == 2) {
                const auto face = TopoDS::Face(tag.Shape); TopLoc_Location location;
                const auto triangulation = BRep_Tool::Triangulation(face,location);
                if (triangulation.IsNull()) {
                    throw std::runtime_error("Union face meshing failed for cut "
                        +artifact->Branches[tag.Origin.Cut].Definition.Id+", source "
                        +std::to_string(tag.Origin.Element)+", mask "
                        +std::to_string(artifact->Branches[tag.Origin.Cut].Definition.Visibility)
                        +", status "+std::to_string(mesher.GetStatusFlags()));
                }
                GProp_GProps area; BRepGProp::SurfaceProperties(face,area); artifact->Area += area.Mass();
                for (int t = 1; t <= triangulation->NbTriangles(); ++t) {
                    int a,b,c; triangulation->Triangle(t).Get(a,b,c); if (face.Orientation() == TopAbs_REVERSED) { std::swap(b,c); }
                    geometry->triangle({{vector(triangulation->Node(a).Transformed(location.Transformation())),vector(triangulation->Node(b).Transformed(location.Transformation())),vector(triangulation->Node(c).Transformed(location.Transformation()))}},tag.Origin,cell);
                }
            } else if (dimension == 1) {
                BRepAdaptor_Curve curve(TopoDS::Edge(tag.Shape)); GCPnts_QuasiUniformDeflection points(curve,tolerance);
                if (!points.IsDone()) { throw std::runtime_error("Union curve discretization failed"); }
                for (int n = 2; n <= points.NbPoints(); ++n) { geometry->line(vector(points.Value(n-1)),vector(points.Value(n)),tag.Origin,cell); }
            } else { geometry->vertex(vector(BRep_Tool::Pnt(TopoDS::Vertex(tag.Shape))),tag.Origin,cell); }
        }
    };
    for (RWCae::MeshEntityId e = 0; e < cells.size(); ++e) {
        const auto& cell = cells[e]; const auto& visible = retained[e];
        TopTools_IndexedMapOfShape volumeFaces; std::vector<unsigned> faceUses;
        if (cell.Dimension == 3) {
            // Fuse may return a compsolid whose two solids share a curved
            // face. Count topology within this original cell, not coordinates.
            for (const auto& solid : primitives(visible.Shape,TopAbs_SOLID)) {
                for (const auto& face : primitives(solid,TopAbs_FACE)) {
                    const int index = volumeFaces.Add(face);
                    if (index > (int)faceUses.size()) { faceUses.push_back(0); }
                    ++faceUses[index-1];
                }
            }
        }
        TaggedShape boundary; std::vector<TopoDS_Shape> faces;
        for (const auto& tag : visible.Tags) {
            if (cell.Dimension == 3 && (tag.Shape.Orientation() == TopAbs_INTERNAL
                || tag.Shape.Orientation() == TopAbs_EXTERNAL)) { continue; }
            const int faceIndex = volumeFaces.FindIndex(tag.Shape);
            if (faceIndex && faceUses[faceIndex-1] > 1) { continue; }
            TaggedShape fragment{tag.Shape,{tag}};
            if (cell.Dimension == 3 && !fragment.Tags.empty()) {
                std::vector<MeshInterpolatedNode> weights; std::vector<Vec> positions;
                for (const auto& vertex : primitives(tag.Shape,TopAbs_VERTEX)) { const auto p = vector(BRep_Tool::Pnt(TopoDS::Vertex(vertex))); positions.push_back(p); weights.push_back(cell.origin(p)); }
                if (!weights.empty()) {
                    const auto traces = topology->Traces(e,weights,positions);
                    if (traces.size() == 2) {
                        const auto neighbor = traces[0].Element == e ? traces[1].Element : traces[0].Element;
                        if (!retained[neighbor].Tags.empty()) { fragment = boolean<BRepAlgoAPI_Cut>(fragment,tagged(retained[neighbor].Shape,tag.Origin)); }
                    }
                }
            }
            for (const auto& piece : fragment.Tags) { boundary.Tags.push_back(piece); faces.push_back(piece.Shape); }
        }
        boundary.Shape = compound(faces);
        std::array<TaggedShape,3> on;
        for (int d = 0; d < 3; ++d) {
            std::vector<TaggedShape> seeds;
            for (size_t c = 0; c < count; ++c) {
                // Independent section results were copied before generated display replacement.
                const auto found = std::find_if(cuts.begin(),cuts.end(),[&](const auto& cut) { return cut.Definition.Id == artifact->Branches[c].Definition.Id; });
                // A retained side already supplies its cap with On ownership.
                if ((found->Definition.Visibility&5) && cell.Dimension == 3 && d == 2) { continue; }
                if (cell.Dimension == 3 && d == 2
                    && (found->Definition.Shape == MeshCutShape::Sphere || found->Definition.Shape == MeshCutShape::Cylinder)
                    && (found->Definition.Visibility&2)) {
                    // Curved On-only faces must retain the OCCT cutting
                    // surface. Rebuilding its float section triangles as BRep
                    // faces collapses the small triangles at sphere poles.
                    const Owner owner{c,MeshCutPortion::On,e,false};
                    auto section = boolean<BRepAlgoAPI_Common>(tagged(body(e),owner),tagged(tools[c],owner));
                    identifyCaps(section,*found,cell,iso[c],options.AbsoluteTolerance);
                    TaggedShape caps; std::vector<TopoDS_Shape> faces;
                    for (const auto& tag : section.Tags) {
                        if (tag.Origin.Cap && tag.Shape.ShapeType() == TopAbs_FACE) {
                            caps.Tags.push_back(tag); faces.push_back(tag.Shape);
                        }
                    }
                    caps.Shape = compound(faces); seeds.push_back(std::move(caps)); continue;
                }
                auto pieces = sectionSeeds(*found,c,e,d); seeds.insert(seeds.end(),pieces.begin(),pieces.end());
            }
            on[d] = fuse(seeds);
            if (!visible.Tags.empty() && !on[d].Tags.empty()) {
                auto outside = boolean<BRepAlgoAPI_Cut>(on[d],visible);
                auto exposed = cell.Dimension == 3 ? boundary
                    : cell.Dimension >= 1 ? lowerBoundary(visible,cell.Dimension) : visible;
                on[d] = boolean<BRepAlgoAPI_Fuse>(outside,boolean<BRepAlgoAPI_Common>(on[d],exposed));
            }
        }
        if (cell.Dimension >= 2) {
            emit(boolean<BRepAlgoAPI_Fuse>(boundary,on[2]),2,cell);
            emit(on[1],1,cell); emit(on[0],0,cell);
        } else {
            emit(boolean<BRepAlgoAPI_Fuse>(visible,on[cell.Dimension]),cell.Dimension,cell);
            if (cell.Dimension == 1) { emit(on[0],0,cell); }
        }
    }
    for (size_t c = 0; c < count; ++c) {
        auto& cut = artifact->Branches[c];
        for (int part = 0; part < 3; ++part) {
            const auto raw = outputs[c][part]; if (raw.IsNull()) { continue; }
            Handle(MeshGeometry) rawRoot = new MeshGeometryBase(bvh,mesh); rawRoot->Append(raw); artifact->OwnedRoots.push_back(rawRoot);
            cut.Roots[part] = rawRoot; cut.Portions[part] = raw;
            if (part == 1) {
                Handle(MeshGeometryTraceSection) traced = new MeshGeometryTraceSection(bvh,raw,source,topology);
                const auto definition = cut.Definition; const auto values = iso[c];
                traced->SetDisplaySelection(definition.DisplayTrace,[definition,values](const MeshCutPrimitiveTrace& trace) { return signedCut(definition,trace.Interior,trace.InteriorWeights,values); },
                    cut.TraceTolerance);
                Handle(MeshGeometry) root = new MeshGeometryBase(bvh,mesh); root->Append(traced); artifact->OwnedRoots.push_back(root);
                cut.Roots[part] = root; cut.Portions[part] = traced;
            }
            cut.Fields[part] = cut.Portions[part]->CalculateProperty(cut.SourceField);
            if (part == 1) { cut.LineField = Handle(MeshGeometrySlice)::DownCast(cut.Portions[part])->CalculateLineProperty(cut.SourceField); }
            std::map<std::vector<RWCae::MeshEntityId>,Owner> elementOwners;
            for (size_t primitive = 0; primitive < raw->ElementOwners.size(); ++primitive) {
                std::vector<RWCae::MeshEntityId> nodes; raw->GetMeshData()->ElementNodes((RWCae::MeshEntityId)primitive,nodes);
                std::sort(nodes.begin(),nodes.end()); elementOwners.emplace(nodes,raw->ElementOwners[primitive]);
            }
            const auto displayed = cut.Portions[part]->GetMeshData();
            for (RWCae::MeshEntityId primitive = 0; primitive < displayed->NbIndexedElems(); ++primitive) {
                std::vector<RWCae::MeshEntityId> nodes; displayed->ElementNodes(primitive,nodes); std::sort(nodes.begin(),nodes.end());
                const auto& owner = elementOwners.at(nodes); const auto type = displayed->IndexedElements()[primitive].Type;
                artifact->Provenance.push_back({cut.Definition.Id,(MeshCutPortion)part,owner.Element,owner.Cap,type == RWCae::MeshElemType::Elem0d ? 0 : 2,(RWCae::MeshEntityId)primitive});
            }
            std::map<std::array<RWCae::MeshEntityId,2>,Owner> lineOwners;
            for (size_t primitive = 0; primitive < raw->LineOwners.size(); ++primitive) { lineOwners.emplace(raw->Lines()[primitive].Nodes,raw->LineOwners[primitive]); }
            const auto slice = Handle(MeshGeometrySlice)::DownCast(cut.Portions[part]);
            for (size_t primitive = 0; primitive < slice->Lines().size(); ++primitive) {
                const auto& owner = lineOwners.at(slice->Lines()[primitive].Nodes); artifact->Provenance.push_back({cut.Definition.Id,(MeshCutPortion)part,owner.Element,owner.Cap,1,(RWCae::MeshEntityId)primitive});
            }
        }
    }
    return artifact;
}
}

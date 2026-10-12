#include <ViewCaeGeom/MeshCutUnion.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <ViewCae/InteractiveCaeCutView.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#ifdef OCCTV_USE_VTK
#include <vtkGenericCell.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkIdList.h>
#endif
#ifdef near
#undef near
#endif
using namespace ViewCaeGeom;
static void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}
static void near(double actual, double expected, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > 2e-5) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
static Handle(RWCae::MeshData) cube()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(0,1,0),
        Graphic3d_Vec3(0,0,1),Graphic3d_Vec3(1,0,1),Graphic3d_Vec3(1,1,1),Graphic3d_Vec3(0,1,1)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3,4,5,6,7}; mesh->AddElement(RWCae::MeshElementHexahedron(nodes)); return mesh;
}
static void basic()
{
    auto mesh = cube(); MeshCutViewManager manager(mesh);
    require(manager.AddPlane("a",Graphic3d_Vec4d(1,0,0,-0.4)),"Add a");
    manager.SetVisibility("a",1);
    auto single = manager.EvaluatePresentation(); near(single->Volume,0.4,"Single volume"); near(single->Area,3.6,"Single area");
    require(manager.Duplicate("a","b"),"Duplicate");
    auto duplicate = manager.EvaluatePresentation(); near(duplicate->Volume,0.4,"Duplicate volume"); near(duplicate->Area,3.6,"Duplicate area");
    require(duplicate->Unified && !duplicate->Provenance.empty(),"Union provenance missing");
    for (const auto& p : duplicate->Provenance) { require(p.CutId == "a" && p.SourceElement == 0,"Unstable duplicate owner"); }
    manager.SetPlane("b",Graphic3d_Vec4d(0,1,0,-0.3));
    auto crossing = manager.EvaluatePresentation(); near(crossing->Volume,0.58,"OR volume"); near(crossing->Area,5.16,"OR boundary area");
    manager.SetVisibility("a",7); manager.SetVisibility("b",7);
    auto all = manager.EvaluatePresentation(); near(all->Volume,1,"Full volume"); near(all->Area,6,"Internal caps must disappear");
    auto independent = manager.Evaluate("a"); require(!independent.PresentationOnly && !independent.Portions[1].IsNull(),"Independent section changed");
    bool rejected = false;
    try { MeshCutIntegration::HeatFlux(all->Branches[0],{{0,1,2}}); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"Union accepted for physical integration");
    manager.SetVisibility("a",2); manager.SetVisibility("b",2);
    auto sheets = manager.EvaluatePresentation(); near(sheets->Volume,0,"Sheets volume"); near(sheets->Area,2,"Crossing sheets area");
    manager.SetPlane("b",Graphic3d_Vec4d(1,0,0,-0.4));
    auto same = manager.EvaluatePresentation(); near(same->Area,1,"Duplicate sheet area");
    manager.SetVisibility("a",1); manager.SetVisibility("b",4);
    const auto touching = manager.EvaluatePresentation(); near(touching->Volume,1,"Touching plane volumes"); near(touching->Area,6,"Touching plane internal cap");
    manager.SetPlane("b",Graphic3d_Vec4d(1,0,0,-0.6));
    const auto separated = manager.EvaluatePresentation(); near(separated->Volume,0.8,"Separated plane volumes"); near(separated->Area,7.2,"Separated exposed caps");
    manager.SetPlane("a",Graphic3d_Vec4d(1,1,0,-1)); manager.SetPlane("b",Graphic3d_Vec4d(0,0,1,-0.4)); manager.SetVisibility("a",3); manager.SetVisibility("b",3);
    const auto oblique = manager.EvaluatePresentation(); near(oblique->Volume,0.7,"Oblique OR volume"); near(oblique->Area,4.8+0.6*std::sqrt(2.0),"Oblique exposed area");
}
// Independent rectangular occupancy oracle: no OCCT or cut backend operations.
static void masks()
{
    MeshCutViewManager manager(cube()); manager.AddPlane("x",Graphic3d_Vec4d(1,0,0,-0.4)); manager.AddPlane("y",Graphic3d_Vec4d(0,1,0,-0.3));
    const double dx[] = {0.4,0.6}, dy[] = {0.3,0.7};
    for (unsigned x = 0; x < 8; ++x) { for (unsigned y = 0; y < 8; ++y) {
        manager.SetVisibility("x",x); manager.SetVisibility("y",y);
        bool kept[2][2]; double volume = 0, perimeter = 0, sheets = 0;
        for (int i = 0; i < 2; ++i) { for (int j = 0; j < 2; ++j) {
            kept[i][j] = (x&(i ? 4 : 1)) || (y&(j ? 4 : 1)); if (kept[i][j]) { volume += dx[i]*dy[j]; }
        } }
        for (int i = 0; i < 2; ++i) { for (int j = 0; j < 2; ++j) { if (kept[i][j]) {
            perimeter += dy[j]*(i == 0 || !kept[i-1][j]); perimeter += dy[j]*(i == 1 || !kept[i+1][j]);
            perimeter += dx[i]*(j == 0 || !kept[i][j-1]); perimeter += dx[i]*(j == 1 || !kept[i][j+1]);
        } } }
        if (x&2) { for (int j = 0; j < 2; ++j) { if (!kept[0][j] && !kept[1][j]) { sheets += dy[j]; } } }
        if (y&2) { for (int i = 0; i < 2; ++i) { if (!kept[i][0] && !kept[i][1]) { sheets += dx[i]; } } }
        const auto result = manager.EvaluatePresentation();
        near(result->Volume,volume,"Mask volume"); near(result->Area,2*volume+perimeter+sheets,"Mask exposed area");
    } }
}
static void adjacent(bool connected)
{
    auto mesh = cube();
    const RWCae::MeshEntityId interfaceNodes[] = {1,2,6,5};
    const Graphic3d_Vec3 positions[] = {Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(2,0,0),Graphic3d_Vec3(2,1,0),Graphic3d_Vec3(1,1,0),
        Graphic3d_Vec3(1,0,1),Graphic3d_Vec3(2,0,1),Graphic3d_Vec3(2,1,1),Graphic3d_Vec3(1,1,1)};
    RWCae::MeshEntityId nodes[8];
    for (int n = 0; n < 8; ++n) {
        const int face = n == 0 ? 0 : n == 3 ? 1 : n == 7 ? 2 : n == 4 ? 3 : -1;
        nodes[n] = connected && face >= 0 ? interfaceNodes[face] : mesh->AddNodePosition(positions[n]);
    }
    mesh->AddElement(RWCae::MeshElementHexahedron(nodes));
    Handle(RWCae::MeshNVecProperty) jump = new RWCae::MeshNVecProperty("jump",1,RWCae::MeshPropertyBase::MappingType::Elemental);
    jump->Init<RWCae::MeshNodeVecProperty>(); auto values = Handle(RWCae::MeshNodeVecProperty)::DownCast(jump->GetComponent(0)); values->Init(2,0,false,false);
    for (RWCae::MeshEntityId e = 0; e < 2; ++e) {
        std::vector<RWCae::MeshEntityId> ids; mesh->ElementNodes(e,ids);
        for (auto n : ids) { const auto p = mesh->Nodes()[n]; values->SetValue(RWCae::MeshPropertyEntityInfo(e,n),100*e+p.y()+2*p.z(),false); }
    }
    jump->UpdateComponents();
    MeshCutViewManager manager(mesh); manager.AddPlane("a",Graphic3d_Vec4d(0,1,0,-0.4)); manager.SetVisibility("a",1); manager.Duplicate("a","b");
    const auto result = manager.EvaluatePresentation(); near(result->Volume,0.8,"Adjacent volume");
    near(result->Area,connected ? 6.4 : 7.2,"Trusted shared boundary or disconnected separation");
    manager.SetPlane("a",Graphic3d_Vec4d(1,0,0,-1)); manager.SetPlane("b",Graphic3d_Vec4d(1,0,0,-1));
    manager.SetVisibility("a",2); manager.SetVisibility("b",2);
    near(manager.EvaluatePresentation()->Area,connected ? 1 : 2,"Shared On ownership");
    if (connected) {
        MeshCutTraceOptions trace; trace.Policy = MeshCutTracePolicy::Region; trace.Region = 20; trace.Regions = {10,20};
        require(manager.SetTraceOptions("a",false,trace),"Region display trace");
        auto cloud = manager.EvaluatePresentation(jump);
        for (const auto& branch : cloud->Branches) {
            const auto data = branch.Portions[1]->GetMeshData(); const auto field = branch.Fields[1];
            for (RWCae::MeshEntityId e = 0; e < data->NbIndexedElems(); ++e) {
                std::vector<RWCae::MeshEntityId> ids; data->ElementNodes(e,ids);
                for (auto n : ids) { const auto p = data->Nodes()[n]; near(field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,n)),100+p.y()+2*p.z(),"Union selected material trace"); }
            }
        }
    } else {
        const auto cloud = manager.EvaluatePresentation(jump); bool seen[2] = {false,false};
        for (const auto& branch : cloud->Branches) {
            const auto data = branch.Portions[1]->GetMeshData(); const auto field = branch.Fields[1];
            for (RWCae::MeshEntityId e = 0; e < data->NbIndexedElems(); ++e) {
                const auto& indexed = data->IndexedElements()[e]; RWCae::MeshEntityId source = 0; branch.Portions[1]->OriginElement(MeshGeomElem(indexed.Index,indexed.Type,false),source); seen[source] = true;
                std::vector<RWCae::MeshEntityId> ids; data->ElementNodes(e,ids);
                for (auto n : ids) { const auto p = data->Nodes()[n]; near(field->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e,n)),100*source+p.y()+2*p.z(),"Disconnected material trace"); }
            }
        }
        require(seen[0] && seen[1],"Disconnected material source welded");
    }
    manager.SetPlane("b",Graphic3d_Vec4d(0,1,0,-2)); manager.SetVisibility("b",1);
    near(manager.EvaluatePresentation()->Area,connected ? 10 : 12,"Covered interface sheet must be internal");
}
static Handle(RWCae::MeshNVecProperty) nodal(const Handle(RWCae::MeshData)& mesh, int components = 1)
{
    Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("nodal",components,RWCae::MeshPropertyBase::MappingType::Nodal);
    field->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < components; ++c) {
        auto values = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetRawComponent(c)); values->Init(mesh->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto p = mesh->Nodes()[n]; values->SetValue(RWCae::MeshPropertyEntityInfo(n),components == 1 ? p.x()+2*p.y()+3*p.z() : c+2);
        }
    }
    field->UpdateComponents(); return field;
}
static void fieldsAndDeformation()
{
    auto mesh = cube(); MeshCutViewManager manager(mesh);
    manager.AddPlane("x",Graphic3d_Vec4d(1,0,0,-0.4)); manager.AddPlane("y",Graphic3d_Vec4d(0,1,0,-0.3));
    manager.SetVisibility("x",3); manager.SetVisibility("y",3);
    const auto scalar = nodal(mesh); const auto flux = nodal(mesh,3);
    const auto before = MeshCutIntegration::HeatFlux(manager.EvaluatePhysical("x",flux),{{0,1,2}});
    const auto stress = nodal(mesh,9);
    const auto forceBefore = MeshCutIntegration::Stress(manager.EvaluatePhysical("x",stress),{{0,1,2,3,4,5,6,7,8}},Graphic3d_Vec3d(0));
    auto display = manager.EvaluatePresentation(scalar);
    for (const auto& cut : display->Branches) { for (int part = 0; part < 3; ++part) {
        if (cut.Portions[part].IsNull()) { continue; }
        const auto data = cut.Portions[part]->GetMeshData(); const auto field = cut.Fields[part];
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) {
            const auto p = data->Nodes()[n];
            near(field->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),p.x()+2*p.y()+3*p.z(),"Union original nodal interpolation");
            MeshInterpolatedNode weights; require(cut.Portions[part]->OriginNode(n,weights),"Union FE provenance");
            double sum = 0; for (const auto& w : weights.BaseNodes()) { sum += w.second; } near(sum,1,"Union FE partition");
        }
    } }
    const auto after = MeshCutIntegration::HeatFlux(manager.EvaluatePhysical("x",flux),{{0,1,2}});
    near(after.Area,before.Area,"Independent area changed"); near(after.HeatFlow,before.HeatFlow,"Independent heat flow changed");
    const auto forceAfter = MeshCutIntegration::Stress(manager.EvaluatePhysical("x",stress),{{0,1,2,3,4,5,6,7,8}},Graphic3d_Vec3d(0));
    for (int c = 0; c < 3; ++c) { near(forceAfter.Force[c],forceBefore.Force[c],"Independent force changed"); near(forceAfter.Moment[c],forceBefore.Moment[c],"Independent moment changed"); }
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
        RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto values = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); values->Init(mesh->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) { values->SetValue(RWCae::MeshPropertyEntityInfo(n),0.1*(c+1)); }
    }
    displacement->UpdateComponents(); require(manager.SetDeformation(displacement,0.2),"Deformation");
    const auto scale = manager.EffectiveDeformationScale();
    manager.SetPlane("x",Graphic3d_Vec4d(1,0,0,-0.4-0.1*scale)); manager.SetPlane("y",Graphic3d_Vec4d(0,1,0,-0.3-0.2*scale));
    auto deformed = manager.EvaluatePresentation(scalar); near(deformed->Volume,0.58,"Deformed union volume");
    for (const auto& cut : deformed->Branches) { for (int part = 0; part < 3; ++part) {
        if (cut.Portions[part].IsNull()) { continue; } const auto data = cut.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) {
            const auto p = data->Nodes()[n]; near(cut.Fields[part]->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
                p.x()+2*p.y()+3*p.z()-1.4*scale,"Deformed original FE field");
        }
    } }
    for (const auto& p : deformed->Provenance) {
        const auto& cut = deformed->Branches[p.CutId == "x" ? 0 : 1]; const auto data = cut.Portions[(int)p.Portion]->GetMeshData();
        require(p.SourceElement == 0 && p.Primitive < data->NbIndexedElems(),"Union provenance index");
        if (p.Cap) { require(p.Portion == MeshCutPortion::On,"Cap does not belong to On"); }
    }
}
static void surfacesAndQuadrics()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2}; mesh->AddElement(RWCae::MeshElementTriangle(nodes));
    MeshCutViewManager manager(mesh); manager.AddPlane("x",Graphic3d_Vec4d(1,0,0,-0.4)); manager.SetVisibility("x",3);
    manager.AddPlane("y",Graphic3d_Vec4d(0,1,0,-0.3)); manager.SetVisibility("y",3);
    const auto cut = manager.EvaluatePresentation(nodal(mesh)); near(cut->Area,0.455,"Surface union area");
    double length = 0;
    for (const auto& branch : cut->Branches) {
        if (branch.Portions[1].IsNull()) { continue; }
        const auto slice = Handle(MeshGeometrySlice)::DownCast(branch.Portions[1]); const auto data = slice->GetMeshData();
        for (const auto& line : slice->Lines()) { length += Graphic3d_Vec3d(data->Nodes()[line.Nodes[0]]-data->Nodes()[line.Nodes[1]]).Modulus(); }
    }
    near(length,0.6,"Exposed surface caps");
    manager.SetVisibility("x",7); manager.SetVisibility("y",7); auto all = manager.EvaluatePresentation(); near(all->Area,0.5,"Full surface OR");
    for (const auto& branch : all->Branches) { require(Handle(MeshGeometrySlice)::DownCast(branch.Portions[1])->Lines().empty(),"Internal surface seam"); }
    mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId tetra[] = {0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(tetra));
    MeshCutViewManager curved(mesh); curved.AddSphere("a",Graphic3d_Vec3d(0.2),0.08); curved.SetVisibility("a",1); curved.Duplicate("a","b");
    auto sphere = curved.EvaluatePresentation(nodal(mesh)); near(sphere->Volume,4*std::acos(-1.0)*std::pow(0.08,3)/3,"Sphere duplicate volume");
    near(sphere->Area,4*std::acos(-1.0)*0.08*0.08,"Sphere duplicate area");
    curved.SetSphere("b",Graphic3d_Vec3d(0.28,0.2,0.2),0.08); auto overlap = curved.EvaluatePresentation();
    const double intersection = std::acos(-1.0)*(4*0.08+0.08)*std::pow(2*0.08-0.08,2)/12;
    near(overlap->Volume,2*sphere->Volume-intersection,"Overlapping spheres OR");
    near(overlap->Area,8*std::acos(-1.0)*0.08*0.08-4*std::acos(-1.0)*0.08*0.04,"Overlapping sphere exposed area");
    curved.SetSphere("b",Graphic3d_Vec3d(0.36,0.2,0.2),0.08);
    auto touching = curved.EvaluatePresentation(); near(touching->Volume,2*sphere->Volume,"Touching sphere volume"); near(touching->Area,2*sphere->Area,"Touching sphere area");
    curved.Remove("a"); curved.Remove("b"); curved.AddCylinder("a",Graphic3d_Vec3d(0.2,0.2,0),Graphic3d_Vec3d(0,0,1),0.08);
    curved.SetVisibility("a",3); curved.Duplicate("a","b");
    const auto independent = curved.Evaluate("a"); const auto cylinder = curved.EvaluatePresentation();
    near(cylinder->Volume,independent.Volumes[0],"Cylinder duplicate volume"); near(cylinder->Area,independent.Areas[0],"Cylinder duplicate area");
}
static void warmSphereMasks()
{
    // Warm the independent source/section meshes before OR presentation, as
    // the interactive viewer does when changing an already displayed cut.
#ifdef OCCTV_USE_VTK
    const auto mesh = cube(); const double radius = 0.15, center = 0.25, sourceVolume = 1, sourceArea = 6;
#else
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(0,1,0),Graphic3d_Vec3(0,0,1)}) { mesh->AddNodePosition(p); }
    const RWCae::MeshEntityId nodes[] = {0,1,2,3}; mesh->AddElement(RWCae::MeshElementTetrahedron(nodes));
    const double radius = 0.08, center = 0.2, sourceVolume = 1.0/6, sourceArea = 1.5+std::sqrt(3.0)/2;
#endif
    MeshCutViewManager manager(mesh);
    require(manager.AddSphere("sphere",Graphic3d_Vec3d(center),radius),"Add warm sphere");
    const double sphereVolume = 4*std::acos(-1.0)*std::pow(radius,3)/3;
    const double sphereArea = 4*std::acos(-1.0)*radius*radius;
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("sphere",mask);
        manager.Evaluate("sphere");
        const auto display = manager.EvaluatePresentation();
        const bool negative = mask&1, positive = mask&4;
        near(display->Volume,negative ? positive ? sourceVolume : sphereVolume : positive ? sourceVolume-sphereVolume : 0,"Warm sphere mask volume");
        near(display->Area,negative && positive ? sourceArea : positive ? sourceArea+sphereArea : mask ? sphereArea : 0,"Warm sphere mask area");
    }
    manager.SetVisibility("sphere",2); manager.Duplicate("sphere","z_duplicate");
    const auto duplicate = manager.EvaluatePresentation(nodal(mesh));
    near(duplicate->Area,sphereArea,"Duplicate On-only sphere area");
    for (const auto& primitive : duplicate->Provenance) {
        require(primitive.CutId == "sphere" && primitive.Cap && primitive.Portion == MeshCutPortion::On,
            "On-only sphere cap ownership");
    }
    manager.SetSphere("z_duplicate",Graphic3d_Vec3d(center+radius,center,center),radius);
    near(manager.EvaluatePresentation()->Area,2*sphereArea,"Intersecting On-only sphere sheet area");
}
#ifdef OCCTV_USE_VTK
static void chartCells()
{
    vtkNew<vtkGenericCell> cell; cell->SetCellType(25); cell->GetPoints()->SetNumberOfPoints(20); cell->GetPointIds()->SetNumberOfIds(20);
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData(); RWCae::MeshData::GenericCell topology; topology.Type = 25;
    const auto parameters = cell->GetParametricCoords();
    for (int n = 0; n < 20; ++n) { topology.Nodes.push_back(mesh->AddNodePosition(Graphic3d_Vec3((float)parameters[3*n],(float)parameters[3*n+1],(float)parameters[3*n+2]))); }
    mesh->AddGenericCell(topology); auto field = nodal(mesh);
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        const auto p = mesh->Nodes()[n]; field->GetComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(n),p.x()*p.x()+2*p.y()*p.y()+3*p.z()*p.z());
    }
    field->UpdateComponents(); MeshCutViewManager manager(mesh); manager.AddPlane("x",Graphic3d_Vec4d(1,0,0,-0.4)); manager.SetVisibility("x",3);
    manager.AddPlane("y",Graphic3d_Vec4d(0,1,0,-0.3)); manager.SetVisibility("y",3);
    auto display = manager.EvaluatePresentation(field); near(display->Volume,0.58,"High-order OR volume");
    for (const auto& branch : display->Branches) { for (int part = 0; part < 3; ++part) {
        if (branch.Portions[part].IsNull()) { continue; } const auto data = branch.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) {
            const auto p = data->Nodes()[n]; near(branch.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
                p.x()*p.x()+2*p.y()*p.y()+3*p.z()*p.z(),"Original high-order FE weights");
        }
    } }
    auto iso = nodal(mesh); iso->SetName("iso_x");
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) { iso->GetComponent(0)->SetValue(RWCae::MeshPropertyEntityInfo(n),mesh->Nodes()[n].x()); }
    iso->UpdateComponents(); manager.Remove("x"); manager.AddIsosurface("x",iso,0,0.4); manager.SetVisibility("x",3);
    auto options = manager.IsoOptions(); options.MinimumDepth = 0; options.SubdivisionLimit = 2; options.RelativeTolerance = 0; options.AbsoluteTolerance = 1e-6; manager.SetIsoOptions(options);
    const auto mixed = manager.EvaluatePresentation(field); near(mixed->Volume,0.58,"High-order iso/plane OR volume"); near(mixed->Area,5.16,"High-order iso/plane exposed area");
    mesh = new RWCae::MeshData(); mesh->AddNodePosition(Graphic3d_Vec3(0,0,0)); mesh->AddNodePosition(Graphic3d_Vec3(1,0,0));
    RWCae::MeshData::GenericCell line; line.Type = 3; line.Nodes = {0,1}; mesh->AddGenericCell(line);
    MeshCutViewManager lines(mesh); lines.AddPlane("a",Graphic3d_Vec4d(1,0,0,-0.4)); lines.SetVisibility("a",3); lines.Duplicate("a","b");
    const auto segment = lines.EvaluatePresentation(nodal(mesh)); double length = 0; int points = 0;
    for (const auto& branch : segment->Branches) { for (const auto& geometry : branch.Portions) {
        if (geometry.IsNull()) { continue; } auto slice = Handle(MeshGeometrySlice)::DownCast(geometry); const auto data = slice->GetMeshData();
        points += data->NbElems(RWCae::MeshElemType::Elem0d);
        for (const auto& edge : slice->Lines()) { length += Graphic3d_Vec3d(data->Nodes()[edge.Nodes[0]]-data->Nodes()[edge.Nodes[1]]).Modulus(); }
    } }
    near(length,0.4,"Line OR length"); require(points == 1,"Line On point ownership");
    mesh = new RWCae::MeshData();
    for (float x : {0.0f,0.5f,1.0f}) { mesh->AddNodePosition(Graphic3d_Vec3(x,0,0)); }
    RWCae::MeshData::GenericCell vertices; vertices.Type = 2; vertices.Nodes = {0,1,2}; mesh->AddGenericCell(vertices);
    RWCae::MeshData::GenericCell empty; empty.Type = 0; mesh->AddGenericCell(empty);
    MeshCutViewManager pointsManager(mesh); pointsManager.AddPlane("a",Graphic3d_Vec4d(1,0,0,-0.5)); pointsManager.SetVisibility("a",7); pointsManager.Duplicate("a","b");
    const auto pointResult = pointsManager.EvaluatePresentation(nodal(mesh)); points = 0;
    for (const auto& branch : pointResult->Branches) { for (int part = 0; part < 3; ++part) {
        if (branch.Portions[part].IsNull()) { continue; } const auto data = branch.Portions[part]->GetMeshData(); points += data->NbElems(RWCae::MeshElemType::Elem0d);
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) { near(branch.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),data->Nodes()[n].x(),"Polyvertex FE source"); }
    } }
    if (points != 3) { std::cerr << "Point OR count: " << points << '\n'; }
    require(points == 3,"Point OR duplicates or empty-cell handling");
}
#endif
int main()
{
    try {
        basic(); masks(); adjacent(true); adjacent(false); fieldsAndDeformation(); surfacesAndQuadrics(); warmSphereMasks();
#ifdef OCCTV_USE_VTK
        chartCells();
#endif
        std::cout << "CUT_UNION_PASS\n"; return 0;
    }
    catch (const Standard_Failure& e) { std::cerr << "CUT_UNION_FAIL: " << e.GetMessageString() << '\n'; return 1; }
    catch (const std::exception& e) { std::cerr << "CUT_UNION_FAIL: " << e.what() << '\n'; return 1; }
}

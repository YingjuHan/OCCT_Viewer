#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshGeometryVtkCut.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWVtk/VtkNativeReader.h>
#include <vtkGenericCell.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <tuple>

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void near(double actual, double expected, const char* message, double tolerance = 1e-5)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) grid(int type, int count, bool connected = true)
{
    const int sizes = type == 25 ? 20 : type == 72 ? 27 : 8;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type == 42 ? 12 : type);
    cell->GetPoints()->SetNumberOfPoints(sizes); cell->GetPointIds()->SetNumberOfIds(sizes);
    for (int n = 0; n < sizes; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type == 72) {
        vtkNew<vtkUnstructuredGrid> source; vtkNew<vtkPoints> points; points->SetNumberOfPoints(27);
        source->SetPoints(points); std::vector<vtkIdType> nodes(27);
        for (int n = 0; n < 27; ++n) { nodes[n] = n; }
        source->InsertNextCell(72,27,nodes.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); const int order[] = {2,2,2};
        degrees->InsertNextTypedTuple(order); source->GetCellData()->SetHigherOrderDegrees(degrees); source->GetCell(0,cell);
    }
    const auto parameters = cell->GetParametricCoords();
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    std::map<std::tuple<float,float,float>,RWCae::MeshEntityId> shared;
    for (int e = 0; e < count; ++e) {
        RWCae::MeshData::GenericCell topology; topology.Type = type;
        if (type == 72) { topology.Degrees = {2,2,2}; }
        for (int n = 0; n < sizes; ++n) {
            const Graphic3d_Vec3 p((float)(parameters[3*n]+e),(float)parameters[3*n+1],(float)parameters[3*n+2]);
            const auto key = std::make_tuple(p.x(),p.y(),p.z());
            if (!connected || !shared.count(key)) { shared[key] = mesh->AddNodePosition(p); }
            topology.Nodes.push_back(shared[key]);
        }
        if (type == 42) {
            for (const auto& face : std::vector<std::vector<int>>{{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}}) {
                std::vector<RWCae::MeshEntityId> nodes;
                for (int n : face) { nodes.push_back(topology.Nodes[n]); } topology.Faces.push_back(std::move(nodes));
            }
        }
        mesh->AddGenericCell(topology);
    }
    return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, bool nodal)
{
    Handle(RWCae::MeshNVecProperty) values = new RWCae::MeshNVecProperty(nodal ? "temperature" : "pressure",1,
        nodal ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental);
    values->Init<RWCae::MeshScalarProperty>();
    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(values->GetComponent(0));
    component->Init(nodal ? mesh->NbNodes() : mesh->NbIndexedElems());
    for (RWCae::MeshEntityId n = 0; n < (nodal ? mesh->NbNodes() : mesh->NbIndexedElems()); ++n) {
        const auto p = nodal ? mesh->Nodes()[n] : Graphic3d_Vec3(0);
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),nodal ? p.x()+2*p.y()+3*p.z() : 7+2*n);
    }
    values->UpdateComponents(); return values;
}
void checkField(const MeshCutResult& cut)
{
    for (int part = 0; part < 3; ++part) {
        if (cut.Portions[part].IsNull() || cut.Fields[part].IsNull()) { continue; }
        const auto mesh = cut.Portions[part]->GetMeshData();
        if (!cut.Fields[part]->IsNodal()) { continue; }
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto p = mesh->Nodes()[n];
            near(cut.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),p.x()+2*p.y()+3*p.z(),"Cached original FE weights");
        }
    }
}
void adjacent(int type)
{
    const auto mesh = grid(type,2);
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); manager.SetVisibility("cut",7);
    auto values = field(mesh,true);
    auto cut = manager.Evaluate("cut",values); checkField(cut);
    near(cut.Volumes[0],1,"Joined negative volume"); near(cut.Volumes[2],1,"Joined positive volume");
    near(cut.Areas[0],7,"Internal negative walls must not render"); near(cut.Areas[2],7,"Internal positive walls must not render");
    near(cut.Areas[1],2,"Joined section area");
    require(manager.Statistics().FilteredFaces > 0,"Boolean history did not filter an internal wall");
    auto source = manager.SourceGeometry();
    auto uncut = dynamic_cast<MeshGeometryVtkCut*>(source->GetLastChild().get());
    require(uncut != nullptr,"General source presentation is missing"); uncut->GetMeshData();
    near(uncut->Area(),10,"Uncut source retained an internal wall"); near(uncut->Volume(),2,"Uncut volume changed during boundary filtering");
    const auto before = manager.Statistics();
    manager.AddPlane("other",Graphic3d_Vec4d(0,0,1,-0.3)); manager.Evaluate("other",values);
    manager.AddSphere("sphere",Graphic3d_Vec3d(1,0.5,0.5),0.2); manager.SetVisibility("sphere",7);
    auto sphere = manager.Evaluate("sphere",values); checkField(sphere);
    near(sphere.Volumes[0],4*std::acos(-1)*std::pow(0.2,3)/3,"Enclosed sphere across interface");
    require(manager.Statistics().PreparedCells == before.PreparedCells && manager.Statistics().SourcePreparations == 1
        && manager.Statistics().BvhBuilds == 1,"Cuts/portions rebuilt source preparation");
    manager.SetPlane("cut",Graphic3d_Vec4d(1,0,0,-1));
    const auto interface = manager.Evaluate("cut",field(mesh,false));
    near(interface.Areas[1],1,"Coincident shared-face section must be counted once");
    for (int e = 0; e < interface.Fields[1]->GetComponent(0)->NbValues(); ++e) {
        near(interface.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e)),7,"Deterministic interface CellData trace");
    }
    manager.SetPlane("cut",Graphic3d_Vec4d(0,1,0,-0.4));
    for (unsigned mask = 0; mask < 8; ++mask) {
        manager.SetVisibility("cut",mask); const auto result = manager.Evaluate("cut",values); checkField(result);
        for (unsigned part = 0; part < 3; ++part) { require(result.Portions[part].IsNull() == ((mask & (1u << part)) == 0),"Adjacency broke portion masks"); }
    }
    std::cout << "GENERAL_ADJACENCY_PASS " << type << '\n';
}
void cache()
{
    const auto mesh = grid(12,24);
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.3));
    auto values = field(mesh,true);
    const auto start = std::chrono::steady_clock::now(); manager.Evaluate("cut",values);
    const auto initial = manager.Statistics();
    require(initial.PreparedCells == 24 && initial.ShapeBuilds == 1 && initial.BvhBuilds == 1,"BVH must avoid preparing noncandidate OCCT shapes");
    for (int frame = 0; frame < 6; ++frame) {
        const double offset = 0.35+0.1*frame;
        manager.SetPlane("cut",Graphic3d_Vec4d(1,0,0,-offset)); const auto result = manager.Evaluate("cut",values); checkField(result);
        MeshCutViewManager reference(mesh); reference.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-offset));
        near(result.Areas[1],reference.Evaluate("cut",values).Areas[1],"Cached and fresh cuts differ");
    }
    const auto warm = manager.Statistics();
    require(warm.PreparedCells == initial.PreparedCells && warm.ShapeBuilds == initial.ShapeBuilds
        && warm.BvhBuilds == 1 && warm.SourcePreparations == 1,"Warm drag rebuilt source cells/shapes/BVH");
    require(warm.CandidateCells == 7,"BVH admitted nonintersecting cells");
    manager.InvalidateFields(); manager.Evaluate("cut",values);
    require(manager.Statistics().SourceEpoch == warm.SourceEpoch && manager.Statistics().ShapeBuilds == warm.ShapeBuilds,"Field edits invalidated geometric source");
    for (auto& p : mesh->ChangeNodes()) { p.z() += 1; }
    mesh->SetCachedMinMax(Bnd_Box()); manager.InvalidateGeometry();
    require(manager.Statistics().SourceEpoch > warm.SourceEpoch && manager.Statistics().PreparedCells == 0,"Geometry edit retained stale source cache");
    manager.Evaluate("cut",values);
    require(manager.Statistics().PreparedCells == 24 && manager.Statistics().BvhBuilds == 1,"Source cache did not rebuild after geometry invalidation");
    mesh->AddGenericCell(mesh->GenericCells().front()); manager.InvalidateGeometry(); manager.Evaluate("cut",values);
    require(manager.Statistics().PreparedCells == 25 && manager.Statistics().UnmergedFaceGroups > 0,"Topology edit reused stale adjacency or hid ambiguous faces");
    Handle(RWCae::MeshNVecProperty) displacement = new RWCae::MeshNVecProperty("displacement",3,
        RWCae::MeshPropertyBase::MappingType::Nodal,RWCae::MeshNVecPropertyType::Vector);
    displacement->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < 3; ++c) {
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(displacement->GetComponent(c)); component->Init(mesh->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) { component->SetValue(RWCae::MeshPropertyEntityInfo(n),c == 0 ? 1 : 0); }
    }
    displacement->UpdateComponents(); const auto epoch = manager.Statistics().SourceEpoch;
    require(manager.SetDeformation(displacement,0.25),"Generic deformation binding rejected");
    require(manager.Statistics().SourceEpoch > epoch && manager.Statistics().PreparedCells == 0,"Deformation reused old spatial cache");
    const double offset = manager.EffectiveDeformationScale()+0.85;
    manager.SetPlane("cut",Graphic3d_Vec4d(1,0,0,-offset));
    const auto deformed = manager.Evaluate("cut",values);
    require(deformed.Portions[1]->GetMeshData()->NbNodes() > 0,"Deformed cached section vanished");
    for (const auto& p : deformed.Portions[1]->GetMeshData()->Nodes()) { near(p.x(),offset,"Deformed BVH/geometry disagree"); }
    manager.Clear(); require(manager.Statistics().PreparedCells == 0,"Close retained source cache");
    const auto elapsed = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout << "GENERAL_CACHE_PASS combined_reference_test_ms " << elapsed << " (not a frame benchmark)\n";
}
void disconnected()
{
    const auto mesh = grid(12,2,false);
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); manager.SetVisibility("cut",7);
    const auto result = manager.Evaluate("cut"); near(result.Areas[0],8,"Disconnected coincident nodes were welded");
    require(manager.Statistics().FilteredFaces == 0,"Disconnected faces lost separate provenance");
}
void writeFixture()
{
    const auto mesh = grid(72,2); vtkNew<vtkUnstructuredGrid> source; vtkNew<vtkPoints> points;
    for (const auto& p : mesh->Nodes()) { points->InsertNextPoint(p.x(),p.y(),p.z()); }
    source->SetPoints(points);
    for (const auto& cell : mesh->GenericCells()) {
        std::vector<vtkIdType> ids(cell.Nodes.begin(),cell.Nodes.end()); source->InsertNextCell(cell.Type,ids.size(),ids.data());
    }
    vtkNew<vtkIntArray> degrees; degrees->SetName("HigherOrderDegrees"); degrees->SetNumberOfComponents(3);
    const int order[] = {2,2,2}; degrees->InsertNextTypedTuple(order); degrees->InsertNextTypedTuple(order);
    source->GetCellData()->SetHigherOrderDegrees(degrees);
    vtkNew<vtkDoubleArray> temperatures; temperatures->SetName("temperature");
    for (const auto& p : mesh->Nodes()) { temperatures->InsertNextValue(p.x()+2*p.y()+3*p.z()); }
    source->GetPointData()->AddArray(temperatures);
    vtkNew<vtkDoubleArray> pressures; pressures->SetName("pressure"); pressures->InsertNextValue(7); pressures->InsertNextValue(9);
    source->GetCellData()->AddArray(pressures);
    std::filesystem::create_directories(GENERAL_CACHE_FIXTURE_DIR);
    const std::string path = std::string(GENERAL_CACHE_FIXTURE_DIR)+"/two_lagrange_hex.vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(source); writer->SetDataModeToAscii();
    require(writer->Write(),"Cannot write adjacent-cell DRAW fixture");
}
}
int main()
{
    try {
        for (int type : {12,11,25,72,42}) { adjacent(type); }
        cache(); disconnected(); writeFixture(); std::cout << "GENERAL_CACHE_NATIVE_PASS\n"; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "GENERAL_CACHE_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "GENERAL_CACHE_FAIL " << error.what() << '\n'; }
    return 1;
}

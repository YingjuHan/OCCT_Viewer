#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshGeometryVtkCut.h>
#include <RWCae/MeshScalarProperty.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkGenericCell.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

// Windows headers define the historical near qualifier as a macro.
#ifdef near
#undef near
#endif

using namespace ViewCaeGeom;
namespace {
void require(bool condition, const char* message) { if (!condition) { throw std::runtime_error(message); } }
void near(double actual, double expected, double tolerance, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
template<class Callback> void rejects(Callback callback, const char* message)
{
    bool rejected = false;
    try { callback(); } catch (const std::exception&) { rejected = true; }
    catch (const Standard_Failure&) { rejected = true; }
    require(rejected,message);
}
Handle(RWCae::MeshData) warped(int count, bool translated, float scale = 1, float shift = 0, int type = 25, bool rational = false)
{
    const int size = type == 25 ? 20 : 27;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type);
    cell->GetPoints()->SetNumberOfPoints(size); cell->GetPointIds()->SetNumberOfIds(size);
    for (int n = 0; n < size; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type >= 68) {
        vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetNumberOfPoints(size); grid->SetPoints(points);
        std::vector<vtkIdType> ids(size); for (int n = 0; n < size; ++n) { ids[n] = n; }
        grid->InsertNextCell(type,size,ids.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); const int order[] = {2,2,2};
        degrees->InsertNextTypedTuple(order); grid->GetCellData()->SetHigherOrderDegrees(degrees); grid->GetCell(0,cell);
    }
    const auto q = cell->GetParametricCoords();
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    std::map<std::tuple<float,float,float>,RWCae::MeshEntityId> shared;
    for (int e = 0; e < count; ++e) {
        RWCae::MeshData::GenericCell topology; topology.Type = type;
        if (type >= 68) { topology.Degrees = {2,2,2}; }
        for (int n = 0; n < size; ++n) {
            const double u = q[3*n], v = q[3*n+1], w = q[3*n+2], h = 0.4*u*(1-u);
            const Graphic3d_Vec3 p(shift+(float)u*scale,(float)v*scale,(float)(e+w+(translated ? h : h*w))*scale);
            const auto key = std::make_tuple(p.x(),p.y(),p.z());
            if (!shared.count(key)) { shared[key] = mesh->AddNodePosition(p); }
            topology.Nodes.push_back(shared[key]);
            if (rational) { topology.RationalWeights.push_back(0.7+0.4*u); }
        }
        mesh->AddGenericCell(topology);
    }
    return mesh;
}
Handle(RWCae::MeshNVecProperty) affine(const Handle(RWCae::MeshData)& mesh)
{
    Handle(RWCae::MeshNVecProperty) field = new RWCae::MeshNVecProperty("temperature",1,RWCae::MeshPropertyBase::MappingType::Nodal);
    field->Init<RWCae::MeshScalarProperty>();
    const auto values = Handle(RWCae::MeshScalarProperty)::DownCast(field->GetComponent(0)); values->Init(mesh->NbNodes());
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        const auto p = mesh->Nodes()[n]; values->SetValue(RWCae::MeshPropertyEntityInfo(n),p.x()+2*p.y()+3*p.z());
    }
    field->UpdateComponents(); return field;
}
void values(const MeshCutResult& cut, double tolerance)
{
    for (int part = 0; part < 3; ++part) {
        if (cut.Fields[part].IsNull()) { continue; }
        const auto mesh = cut.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto p = mesh->Nodes()[n];
            near(cut.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),p.x()+2*p.y()+3*p.z(),tolerance,"Original FE interpolation");
        }
    }
}
MeshCutGeometryOptions options(double tolerance)
{
    MeshCutGeometryOptions value; value.Mode = MeshCutGeometryMode::Numerical;
    value.AbsoluteTolerance = tolerance; value.RelativeTolerance = 0; return value;
}
void convergence()
{
    const auto mesh = warped(1,false); const auto field = affine(mesh);
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5));
    double previous = 1;
    uint64_t previousCount = 0;
    for (double tolerance : {0.025,0.007,0.002}) {
        require(manager.SetGeometryOptions(options(tolerance)),"Valid geometry options rejected");
        const auto cut = manager.Evaluate("cut",field); values(cut,2e-5);
        const double error = std::abs(cut.Areas[1]-(1+0.4/6));
        const auto work = manager.Statistics();
        near(work.GeometryTolerance,tolerance,1e-12,"Absolute tolerance not used");
        require(error < previous*0.6 && work.SourcePrimitives > previousCount,"Curved section does not converge under refinement");
        require(error <= tolerance*2,"Curved section exceeds reference error bound");
        previous = error; previousCount = work.SourcePrimitives;
        std::cout << "CUT_GEOMETRY_CONVERGENCE tolerance " << tolerance << " area_error " << error
                  << " primitives " << work.SourcePrimitives << '\n';
    }
    const auto before = manager.Statistics();
    require(manager.SetGeometryOptions(manager.GeometryOptions()),"Identical options rejected");
    require(manager.Statistics().SourceEpoch == before.SourceEpoch,"Identical options rebuilt source");
    auto invalid = manager.GeometryOptions(); invalid.AbsoluteTolerance = std::numeric_limits<double>::quiet_NaN();
    require(!manager.SetGeometryOptions(invalid),"NaN tolerance accepted");
    invalid = manager.GeometryOptions(); invalid.RelativeTolerance = -1;
    require(!manager.SetGeometryOptions(invalid),"Negative relative tolerance accepted");
    invalid = manager.GeometryOptions(); invalid.FaceSubdivisionLimit = 11;
    require(!manager.SetGeometryOptions(invalid),"Unbounded face refinement accepted");
    require(manager.Statistics().SourceEpoch == before.SourceEpoch,"Rejected options invalidated source");
    auto limited = options(1e-4); limited.FaceSubdivisionLimit = 0;
    require(manager.SetGeometryOptions(limited),"Zero subdivision limit rejected");
    rejects([&] { manager.Evaluate("cut",field); },"Unsatisfied face tolerance returned partial data");
    require(manager.Statistics().SourcePreparations == 0,"Failed preparation was published");
    limited = options(0.007); limited.MaxSourcePrimitives = 1;
    require(manager.SetGeometryOptions(limited),"Valid primitive budget rejected");
    rejects([&] { manager.Evaluate("cut",field); },"Source primitive budget ignored");
    require(manager.Statistics().PreparedCells == 0,"Over-budget source was published");
    require(manager.SetGeometryOptions(options(0.007)),"Recovery options rejected");
    require(manager.Evaluate("cut",field).Areas[1] > 1,"Cannot recover after rejected tessellation");
}
void sharedCurvedFace(int type = 25, bool rational = false)
{
    const auto mesh = warped(2,true,1,0,type,rational); MeshCutViewManager manager(mesh);
    manager.SetGeometryOptions(options(0.007)); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); manager.SetVisibility("cut",7);
    const auto result = manager.Evaluate("cut",affine(mesh)); values(result,2e-5);
    const double length = 0.5*std::sqrt(1+0.4*0.4)+std::asinh(0.4)/(2*0.4);
    near(result.Volumes[0],1,2e-5,"Joined curved negative volume"); near(result.Volumes[2],1,2e-5,"Joined curved positive volume");
    near(result.Areas[1],2,2e-5,"Joined curved section area");
    const double areaTolerance = type == 79 ? 0.045 : 0.012;
    near(result.Areas[0],6+length,areaTolerance,"Curved internal negative wall remains");
    near(result.Areas[2],6+length,areaTolerance,"Curved internal positive wall remains");
    require(manager.Statistics().FilteredFaces > 0 && manager.Statistics().UnmergedFaceGroups == 0,"Conforming curved adjacency not recognized");
    const auto source = manager.SourceGeometry(); auto boundary = dynamic_cast<MeshGeometryVtkCut*>(source->GetLastChild().get());
    require(boundary != nullptr,"Missing curved source boundary"); boundary->GetMeshData();
    near(boundary->Area(),8+2*length,areaTolerance*2,"Curved source retains internal walls"); near(boundary->Volume(),2,2e-5,"Curved source volume changed");
    const auto warm = manager.Statistics(); manager.SetPlane("cut",Graphic3d_Vec4d(0,1,0,-0.4)); manager.Evaluate("cut",affine(mesh));
    require(manager.Statistics().SourcePreparations == warm.SourcePreparations
        && manager.Statistics().FaceSubdivisionDepth == warm.FaceSubdivisionDepth,"Dragging rebuilt conforming tessellation");
    std::cout << "CUT_GEOMETRY_CURVED_ADJACENCY type " << type << " rational " << rational << '\n';
}
void relativeAndPrecision()
{
    const auto mesh = warped(1,false,10); MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-5));
    auto relative = options(1e-6); relative.RelativeTolerance = 0.001; manager.SetGeometryOptions(relative); manager.Evaluate("cut");
    const auto bounds = mesh->BoundingBox();
    near(manager.Statistics().GeometryTolerance,0.001*(bounds.CornerMax().XYZ()-bounds.CornerMin().XYZ()).Modulus(),1e-10,"Relative tolerance does not scale with model");
    const auto translated = warped(1,false,1,1000000); MeshCutViewManager shifted(translated);
    shifted.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); shifted.Evaluate("cut");
    require(shifted.Statistics().StorageResolution >= 0.0625,"Float output resolution is concealed");
}
void curvedLine()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    mesh->AddNodePosition(Graphic3d_Vec3(0,0,0)); mesh->AddNodePosition(Graphic3d_Vec3(1,0,0)); mesh->AddNodePosition(Graphic3d_Vec3(0.5,0.25,0));
    RWCae::MeshData::GenericCell cell; cell.Type = 21; cell.Nodes = {0,1,2}; mesh->AddGenericCell(cell);
    MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.37));
    for (double tolerance : {0.01,0.002,0.0005}) {
        manager.SetGeometryOptions(options(tolerance)); const auto result = manager.Evaluate("cut",affine(mesh));
        const auto output = result.Portions[1]->GetMeshData(); require(output->NbNodes() > 0,"Curved line section vanished");
        for (const auto& p : output->Nodes()) {
            near(p.x(),0.37,1e-6,"Curved line is not cut by plane");
            near(p.y(),0.37*(1-0.37),tolerance,"Curved line chord does not converge");
        }
    }
    auto limited = options(1e-5); limited.LineSubdivisionLimit = 0; manager.SetGeometryOptions(limited);
    rejects([&] { manager.Evaluate("cut"); },"Unsatisfied line tolerance returned a partial section");
}
void writeFixture()
{
    const auto mesh = warped(2,true); vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetDataTypeToDouble();
    for (const auto& p : mesh->Nodes()) { points->InsertNextPoint(p.x(),p.y(),p.z()); }
    grid->SetPoints(points);
    for (const auto& cell : mesh->GenericCells()) {
        std::vector<vtkIdType> ids(cell.Nodes.begin(),cell.Nodes.end()); grid->InsertNextCell(cell.Type,ids.size(),ids.data());
    }
    vtkNew<vtkDoubleArray> field; field->SetName("temperature");
    for (const auto& p : mesh->Nodes()) { field->InsertNextValue(p.x()+2*p.y()+3*p.z()); }
    grid->GetPointData()->AddArray(field);
    vtkNew<vtkDoubleArray> cell; cell->SetName("pressure"); cell->InsertNextValue(7); cell->InsertNextValue(9); grid->GetCellData()->AddArray(cell);
    std::filesystem::create_directories(CUT_GEOMETRY_FIXTURE_DIR);
    const std::string path = std::string(CUT_GEOMETRY_FIXTURE_DIR)+"/joined_curved_hex.vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(grid); writer->SetDataModeToAscii();
    require(writer->Write(),"Cannot create curved DRAW fixture");
}
}
int main()
{
    try {
        convergence(); for (int type : {25,29,72,79}) { sharedCurvedFace(type,type == 79); }
        relativeAndPrecision(); curvedLine(); writeFixture(); std::cout << "CUT_GEOMETRY_NATIVE_PASS\n"; return 0;
    }
    catch (const Standard_Failure& error) { std::cerr << "CUT_GEOMETRY_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "CUT_GEOMETRY_FAIL " << error.what() << '\n'; }
    return 1;
}

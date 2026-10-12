#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshGeometryVtkCut.h>
#include <RWCae/MeshScalarProperty.h>
#include <vtkGenericCell.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iomanip>
#include <stdexcept>

// Windows headers define the historical near qualifier as a macro.
#ifdef near
#undef near
#endif

using namespace ViewCaeGeom;
namespace {
void require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void near(double actual, double expected, double tolerance, const char* message)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << std::setprecision(16) << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) fixture(int type, bool rational = false, int count = 0)
{
    const int size = count ? count : type == 24 ? 10 : type == 25 ? 20 : type == 33 ? 24 : type == 21 ? 3 : 27;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type);
    cell->GetPoints()->SetNumberOfPoints(size); cell->GetPointIds()->SetNumberOfIds(size);
    for (int n = 0; n < size; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type == 72 || type == 79) {
        vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetNumberOfPoints(size); grid->SetPoints(points);
        std::vector<vtkIdType> ids(size); for (int n = 0; n < size; ++n) { ids[n] = n; }
        grid->InsertNextCell(type,size,ids.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); const int order[] = {2,2,2};
        degrees->InsertNextTypedTuple(order); grid->GetCellData()->SetHigherOrderDegrees(degrees); grid->GetCell(0,cell);
    }
    const auto coordinates = cell->GetParametricCoords();
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData(); RWCae::MeshData::GenericCell topology; topology.Type = type;
    if (type == 72 || type == 79) { topology.Degrees = {2,2,2}; }
    if (type == 68) { topology.Degrees = {size-1,0,0}; }
    for (int n = 0; n < size; ++n) {
        const double u = coordinates[3*n], v = coordinates[3*n+1], w = coordinates[3*n+2];
        Graphic3d_Vec3 p((float)u,(float)v,(float)(w*(1+0.4*u*(1-u))));
        if (type == 24) { p.z() = (float)(w+0.4*u*(1-u-v-w)); }
        if (type == 21 || type == 68) { p.y() = (float)(u*(1-u)); p.z() = 0; }
        mesh->AddNodePosition(p); topology.Nodes.push_back(n);
        if (rational) { topology.RationalWeights.push_back(0.7+0.4*u); }
    }
    mesh->AddGenericCell(topology); return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, bool polynomial = false)
{
    Handle(RWCae::MeshNVecProperty) values = new RWCae::MeshNVecProperty(polynomial ? "quadratic" : "temperature",1,
        RWCae::MeshPropertyBase::MappingType::Nodal);
    values->Init<RWCae::MeshScalarProperty>();
    const auto component = Handle(RWCae::MeshScalarProperty)::DownCast(values->GetComponent(0)); component->Init(mesh->NbNodes());
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        const auto p = mesh->Nodes()[n];
        const double u = p.x(), v = p.y(), w = mesh->GenericCells()[0].Type == 24
            ? (p.z()-0.4*u*(1-u-v))/(1-0.4*u) : p.z();
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),polynomial ? u*u+2*v*v+3*w*w : u+2*v+3*p.z());
    }
    values->UpdateComponents(); return values;
}
MeshCutGeometryOptions required(double tolerance = 0.03)
{
    MeshCutGeometryOptions options; options.Mode = MeshCutGeometryMode::AnalyticRequired;
    options.AbsoluteTolerance = tolerance; options.RelativeTolerance = 0; return options;
}
void writeFixture(const Handle(RWCae::MeshData)& mesh, const std::string& name, bool polynomial = false);
void checkAffine(const MeshCutResult& result)
{
    require(!result.SourceApproximate,"Required analytic mode returned numerical geometry");
    for (int part = 0; part < 3; ++part) {
        if (result.Fields[part].IsNull()) { continue; }
        const auto mesh = result.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto p = mesh->Nodes()[n];
            near(result.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),p.x()+2*p.y()+3*p.z(),2e-5,"Analytic original-cell interpolation");
        }
    }
}
double rationalVolume()
{
    auto integrand = [](double u) {
        const double d = 0.7+0.4*u, n = 0.9*u+0.2*u*u;
        return (1+0.18*u*(1-u)/d)*((0.9+0.4*u)*d-0.4*n)/(d*d);
    };
    constexpr int count = 4096;
    double sum = integrand(0)+integrand(1);
    for (int n = 1; n < count; ++n) { sum += (n%2 ? 4 : 2)*integrand((double)n/count); }
    return sum/(3*count);
}
void box(int type)
{
    const bool rational = type == 79;
    const auto mesh = fixture(type,rational); MeshCutViewManager manager(mesh);
    require(manager.SetGeometryOptions(required()),"Analytic options rejected");
    manager.AddPlane("cut",Graphic3d_Vec4d(0,1,0,-0.5)); manager.SetVisibility("cut",7);
    const auto values = field(mesh); const auto result = manager.Evaluate("cut",values); checkAffine(result);
    const double volume = rational ? rationalVolume() : 1+0.4/6;
    near(result.Areas[1],volume,2e-7,"Exact warped-hex section area");
    near(result.Volumes[0],volume/2,2e-7,"Exact warped-hex negative volume");
    near(result.Volumes[2],volume/2,2e-7,"Exact warped-hex positive volume");
    const auto source = manager.SourceGeometry(); auto boundary = dynamic_cast<MeshGeometryVtkCut*>(source->GetLastChild().get());
    require(boundary != nullptr,"Missing analytic source boundary"); boundary->GetMeshData();
    near(boundary->Volume(),volume,2e-7,"Exact warped-hex source volume");
    const auto work = manager.Statistics();
    require(work.AnalyticCells == 1 && work.NumericalCells == 0 && work.AnalyticResidual < 1e-9,"Analytic conversion diagnostics missing");
    manager.SetPlane("cut",Graphic3d_Vec4d(0,1,0,-0.4)); checkAffine(manager.Evaluate("cut",values));
    require(manager.Statistics().SourcePreparations == work.SourcePreparations && manager.Statistics().ShapeBuilds == work.ShapeBuilds,"Moving plane rebuilt analytic source");
    manager.AddSphere("sphere",Graphic3d_Vec3d(0.4,0.5,0.5),0.08); manager.SetVisibility("sphere",7);
    const auto sphere = manager.Evaluate("sphere",values); checkAffine(sphere);
    near(sphere.Volumes[0],4*std::acos(-1)*std::pow(0.08,3)/3,2e-8,"Analytic enclosed sphere volume");
    std::cout << "CUT_ANALYTIC_BOX_PASS " << type << '\n';
}
void tetra()
{
    const auto mesh = fixture(24); MeshCutViewManager manager(mesh); manager.SetGeometryOptions(required());
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); manager.SetVisibility("cut",7);
    const auto result = manager.Evaluate("cut",field(mesh,true)); require(!result.SourceApproximate,"Quadratic tetrahedron was faceted");
    const double h = 0.4, u = 0.25;
    const double negative = (u-(2+h)*u*u/2+(1+2*h)*u*u*u/3-h*std::pow(u,4)/4)/2;
    near(result.Areas[1],(1-h*u)*std::pow(1-u,2)/2,2e-7,"Exact bowed-tetra section area");
    near(result.Volumes[0],negative,2e-7,"Exact bowed-tetra negative volume");
    near(result.Volumes[0]+result.Volumes[2],1.0/6-h/24,2e-7,"Exact bowed-tetra source volume");
    const auto section = result.Portions[1]->GetMeshData();
    for (RWCae::MeshEntityId n = 0; n < section->NbNodes(); ++n) {
        const auto p = section->Nodes()[n]; const double w = (p.z()-h*p.x()*(1-p.x()-p.y()))/(1-h*p.x());
        near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),p.x()*p.x()+2*p.y()*p.y()+3*w*w,2e-6,"Bowed-tetra quadratic FE values");
    }
    std::cout << "CUT_ANALYTIC_TETRA_PASS\n";
}
void curveAndFallback()
{
    const auto mesh = fixture(21); MeshCutViewManager manager(mesh); manager.SetGeometryOptions(required(0.1));
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.37)); const auto result = manager.Evaluate("cut",field(mesh,true));
    require(!result.SourceApproximate && result.Portions[1]->GetMeshData()->NbNodes() > 0,"Analytic curve section is missing");
    for (const auto& p : result.Portions[1]->GetMeshData()->Nodes()) { near(p.y(),0.37*(1-0.37),2e-7,"Analytic quadratic curve, not chord intersection"); }
    const auto unsupported = fixture(68,false,8); MeshCutViewManager fallback(unsupported);
    fallback.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.37));
    require(fallback.Evaluate("cut").SourceApproximate && fallback.Statistics().NumericalCells == 1,"Unsupported analytic order was not identified as numerical");
    fallback.SetGeometryOptions(required()); bool rejected = false;
    try { fallback.Evaluate("cut"); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected && fallback.Statistics().SourcePreparations == 0,"Required analytic mode published an unsupported source");
    auto options = required(); options.Mode = MeshCutGeometryMode::Numerical; fallback.SetGeometryOptions(options);
    require(fallback.Evaluate("cut").Portions[1]->GetMeshData()->NbNodes() > 0,"Numerical fallback did not recover");
    std::cout << "CUT_ANALYTIC_CURVE_FALLBACK_PASS\n";
}
void polygon()
{
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    for (const auto& p : {Graphic3d_Vec3(0,0,0),Graphic3d_Vec3(1,0,0),Graphic3d_Vec3(1,1,0),Graphic3d_Vec3(0,1,0),
        Graphic3d_Vec3(0.5f,-0.1f,0),Graphic3d_Vec3(1,0.5f,0),Graphic3d_Vec3(0.5f,1.1f,0),Graphic3d_Vec3(0,0.5f,0)}) { mesh->AddNodePosition(p); }
    RWCae::MeshData::GenericCell topology; topology.Type = 36; topology.Nodes = {0,1,2,3,4,5,6,7}; mesh->AddGenericCell(topology);
    MeshCutViewManager manager(mesh); manager.SetGeometryOptions(required());
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.37));
    const auto result = manager.Evaluate("cut",field(mesh)); checkAffine(result);
    const auto slice = Handle(MeshGeometrySlice)::DownCast(result.Portions[1]);
    require(slice->Lines().size() > 0,"Curved polygon has no section lines");
    double length = 0;
    for (const auto& line : slice->Lines()) {
        const auto& nodes = slice->GetMeshData()->Nodes(); length += (nodes[line.Nodes[0]]-nodes[line.Nodes[1]]).Modulus();
    }
    near(length,1+0.8*0.37*(1-0.37),3e-7,"Analytic curved polygon section length");
    const auto source = manager.SourceGeometry(); auto boundary = dynamic_cast<MeshGeometryVtkCut*>(source->GetLastChild().get()); boundary->GetMeshData();
    near(boundary->Area(),1+0.8/6,3e-7,"Analytic curved polygon area");
    std::cout << "CUT_ANALYTIC_POLYGON_PASS\n";
}
double simpson(const std::function<double(double)>& function, double end)
{
    constexpr int count = 4096; double sum = function(0)+function(end);
    for (int n = 1; n < count; ++n) { sum += (n%2 ? 4 : 2)*function(end*n/count); }
    return sum*end/(3*count);
}
void wedge(int type, bool rational = false)
{
    const int count = type == 26 ? 15 : type == 31 ? 12 : 18;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type);
    cell->GetPoints()->SetNumberOfPoints(count); cell->GetPointIds()->SetNumberOfIds(count);
    for (int n = 0; n < count; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type == 73 || type == 80) {
        vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetNumberOfPoints(count); grid->SetPoints(points);
        std::vector<vtkIdType> ids(count); for (int n = 0; n < count; ++n) { ids[n] = n; }
        grid->InsertNextCell(type,count,ids.data()); vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3);
        const int orders[] = {2,2,2}; degrees->InsertNextTypedTuple(orders); grid->GetCellData()->SetHigherOrderDegrees(degrees); grid->GetCell(0,cell);
    }
    auto coordinates = cell->GetParametricCoords(); Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    RWCae::MeshData::GenericCell topology; topology.Type = type;
    if (type == 73 || type == 80) { topology.Degrees = {2,2,2}; }
    for (int n = 0; n < count; ++n) {
        const double u = coordinates[3*n], v = coordinates[3*n+1], w = coordinates[3*n+2];
        topology.Nodes.push_back(mesh->AddNodePosition(Graphic3d_Vec3((float)u,(float)v,(float)(w*(1+0.4*u*(1-u-v))))));
        if (rational) { topology.RationalWeights.push_back(0.7+0.4*u); }
    }
    mesh->AddGenericCell(topology); MeshCutViewManager manager(mesh); manager.SetGeometryOptions(required());
    manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.25)); manager.SetVisibility("cut",7);
    // Independent triangular Bernstein moments: rational U=N/D and V=v*K.
    const double h = type == 80 ? 0.2 : 0.4;
    const double u = rational ? (-0.8+std::sqrt(0.64+0.14))/0.4 : 0.25;
    auto section = [&](double q) {
        const double d = 0.7+0.4*q, k = rational ? (0.7+0.2*q)/d : 1;
        return k*((1-q)+(rational ? 0.09/d : h/2)*q*std::pow(1-q,2));
    };
    auto density = [&](double q) {
        const double d = 0.7+0.4*q, numerator = 0.9*q+0.2*q*q;
        const double derivative = rational ? ((0.9+0.4*q)*d-0.4*numerator)/(d*d) : 1;
        return derivative*section(q);
    };
    const auto result = manager.Evaluate("cut",field(mesh)); checkAffine(result);
    near(result.Areas[1],section(u),3e-7,"Exact curved-wedge section reference");
    near(result.Volumes[0],simpson(density,u),3e-7,"Exact curved-wedge partial volume reference");
    near(result.Volumes[0]+result.Volumes[2],simpson(density,1),3e-7,"Exact curved-wedge volume reference");
    require(manager.Statistics().AnalyticCells == 1 && manager.Statistics().NumericalCells == 0,"Curved wedge fell back to numerical geometry");
    writeFixture(mesh,"curved_wedge_"+std::to_string(type)+(rational ? "_rational" : ""));
    std::cout << "CUT_ANALYTIC_WEDGE_PASS " << type << " rational " << rational << '\n';
}
void pyramid(int type)
{
    const int count = type == 27 ? 13 : 19;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type); cell->GetPoints()->SetNumberOfPoints(count);
    cell->GetPointIds()->SetNumberOfIds(count);
    for (int n = 0; n < count; ++n) { cell->GetPointIds()->SetId(n,n); }
    const double canonical[][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0.5,0.5,1},
        {0.5,0,0},{1,0.5,0},{0.5,1,0},{0,0.5,0},{0.25,0.25,0.5},{0.75,0.25,0.5},{0.75,0.75,0.5},{0.25,0.75,0.5}};
    auto parameters = cell->GetParametricCoords(); Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    RWCae::MeshData::GenericCell topology; topology.Type = type;
    for (int n = 0; n < count; ++n) {
        const double x = type == 27 ? canonical[n][0] : parameters[3*n];
        const double y = type == 27 ? canonical[n][1] : parameters[3*n+1];
        const double z = type == 27 ? canonical[n][2] : 2*parameters[3*n+2]-1;
        topology.Nodes.push_back(mesh->AddNodePosition(Graphic3d_Vec3((float)(x+0.4*z*(1-z)),(float)y,(float)z)));
    }
    mesh->AddGenericCell(topology); MeshCutViewManager manager(mesh); manager.SetGeometryOptions(required());
    manager.AddPlane("cut",Graphic3d_Vec4d(0,0,1,-0.5)); manager.SetVisibility("cut",7);
    // A quadratic lateral shear preserves the Jacobian; the horizontal square shifts by 0.1.
    const auto result = manager.Evaluate("cut",field(mesh)); checkAffine(result);
    near(result.Areas[1],0.25,3e-7,"Exact curved-pyramid section area");
    near(result.Volumes[0],7.0/24,3e-7,"Exact curved-pyramid negative volume");
    near(result.Volumes[2],1.0/24,3e-7,"Exact curved-pyramid positive volume");
    const auto section = result.Portions[1]->GetMeshData(); double low = INFINITY, high = -INFINITY;
    for (auto p : section->Nodes()) { low = std::min(low,(double)p.x()); high = std::max(high,(double)p.x()); }
    near(low,0.35,3e-7,"Curved pyramid left edge was replaced by a chord");
    near(high,0.85,3e-7,"Curved pyramid right edge was replaced by a chord");
    auto quadratic = field(mesh); auto component = quadratic->GetComponent(0);
    for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),std::pow(mesh->Nodes()[n].z(),2));
    }
    quadratic->UpdateComponents(); const auto mapped = manager.Evaluate("cut",quadratic);
    for (RWCae::MeshEntityId n = 0; n < section->NbNodes(); ++n) {
        near(mapped.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),0.25,3e-6,"Curved-pyramid original quadratic field");
    }
    writeFixture(mesh,"curved_pyramid_"+std::to_string(type));
    std::cout << "CUT_ANALYTIC_PYRAMID_PASS " << type << '\n';
}
void writeFixture(const Handle(RWCae::MeshData)& mesh, const std::string& name, bool polynomial)
{
    vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetDataTypeToDouble();
    for (const auto& p : mesh->Nodes()) { points->InsertNextPoint(p.x(),p.y(),p.z()); }
    grid->SetPoints(points); std::vector<vtkIdType> ids(mesh->GenericCells()[0].Nodes.begin(),mesh->GenericCells()[0].Nodes.end());
    const auto& cell = mesh->GenericCells()[0]; grid->InsertNextCell(cell.Type,ids.size(),ids.data());
    if (!cell.Degrees.empty()) {
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3); degrees->InsertNextTypedTuple(cell.Degrees.data());
        grid->GetCellData()->SetHigherOrderDegrees(degrees);
    }
    if (!cell.RationalWeights.empty()) {
        vtkNew<vtkDoubleArray> weights; weights->SetName("RationalWeights");
        for (double weight : cell.RationalWeights) { weights->InsertNextValue(weight); }
        grid->GetPointData()->SetRationalWeights(weights);
    }
    for (int component = 0; component <= (polynomial ? 1 : 0); ++component) {
        const auto values = field(mesh,component == 1); vtkNew<vtkDoubleArray> array; array->SetName(component == 1 ? "quadratic" : "temperature");
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) { array->InsertNextValue(values->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n))); }
        grid->GetPointData()->AddArray(array);
    }
    vtkNew<vtkDoubleArray> squared; squared->SetName("z_squared");
    for (const auto& p : mesh->Nodes()) { squared->InsertNextValue((double)p.z()*p.z()); }
    grid->GetPointData()->AddArray(squared);
    vtkNew<vtkDoubleArray> pressure; pressure->SetName("pressure"); pressure->InsertNextValue(7); grid->GetCellData()->AddArray(pressure);
    std::filesystem::create_directories(CUT_ANALYTIC_FIXTURE_DIR);
    const std::string path = std::string(CUT_ANALYTIC_FIXTURE_DIR)+"/"+name+".vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(grid); writer->SetDataModeToAscii();
    require(writer->Write(),"Cannot generate analytic DRAW fixture");
}
}
int main()
{
    try {
        for (int type : {25,29,33,72,79}) { box(type); }
        for (int type : {26,31,32,73,80}) { wedge(type); }
        wedge(80,true); pyramid(27); pyramid(37);
        tetra(); curveAndFallback(); polygon(); writeFixture(fixture(24),"bowed_tetra",true); std::cout << "CUT_ANALYTIC_NATIVE_PASS\n"; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "CUT_ANALYTIC_FAIL " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "CUT_ANALYTIC_FAIL " << error.what() << '\n'; }
    return 1;
}

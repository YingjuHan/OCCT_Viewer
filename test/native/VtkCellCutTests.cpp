#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutIntegration.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWVtk/VtkNativeReader.h>
#include <vtkCell.h>
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
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <limits>

using namespace ViewCaeGeom;
namespace {
void require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
void near(double actual, double expected, const char* message, double tolerance = 1e-5)
{
    if (!std::isfinite(actual) || std::abs(actual-expected) > tolerance) {
        std::cerr << message << ": " << actual << " != " << expected << '\n'; throw std::runtime_error(message);
    }
}
Handle(RWCae::MeshData) fixture(int type, int requestedCount = 0,
    const std::vector<int>& requestedDegrees = {}, bool rational = false)
{
    const std::map<int,int> sizes = {{0,0},{1,1},{2,3},{3,2},{4,3},{5,3},{6,4},{7,5},{8,4},{9,4},
        {10,4},{11,8},{12,8},{13,6},{14,5},{15,10},{16,12},{21,3},{22,6},{23,8},{24,10},{25,20},
        {26,15},{27,13},{28,9},{29,27},{30,6},{31,12},{32,18},{33,24},{34,7},{35,4},{36,8},{37,19},
        {41,5},{42,8},{68,3},{69,6},{70,9},{71,10},{72,27},{73,18},{75,3},{76,6},{77,9},{78,10},{79,27},{80,18}};
    const int count = requestedCount ? requestedCount : sizes.at(type);
    const std::vector<int> orders = requestedDegrees.empty() ? std::vector<int>{2,2,2} : requestedDegrees;
    vtkNew<vtkGenericCell> cell; cell->SetCellType(type);
    cell->GetPoints()->SetNumberOfPoints(count); cell->GetPointIds()->SetNumberOfIds(count);
    for (int n = 0; n < count; ++n) { cell->GetPointIds()->SetId(n,n); }
    if (type >= 68) {
        vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetNumberOfPoints(count);
        grid->SetPoints(points);
        std::vector<vtkIdType> ids(count); for (int n = 0; n < count; ++n) { ids[n] = n; }
        grid->InsertNextCell(type,count,ids.data());
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3);
        degrees->InsertNextTypedTuple(orders.data()); grid->GetCellData()->SetHigherOrderDegrees(degrees);
        grid->GetCell(0,cell);
    }
    Handle(RWCae::MeshData) mesh = new RWCae::MeshData();
    RWCae::MeshData::GenericCell topology; topology.Type = type;
    if (type >= 68) { topology.Degrees = orders; }
    double* parametric = type == 42 || type == 41 ? nullptr : cell->GetParametricCoords();
    for (int n = 0; n < count; ++n) {
        Graphic3d_Vec3 p;
        if (type == 2) { p = Graphic3d_Vec3((float)n/2,0,0); }
        else if (type == 4) { p = Graphic3d_Vec3((float)n/2,0,0); }
        else if (type == 6) {
            const float points[][3] = {{0,0,0},{1,0,0},{0,1,0},{1,1,0}}; p = Graphic3d_Vec3(points[n][0],points[n][1],points[n][2]);
        } else if (type == 7) { p = Graphic3d_Vec3((float)std::cos(2*n*std::acos(-1)/count),(float)std::sin(2*n*std::acos(-1)/count),0); }
        else if (type == 36) {
            const float points[][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0.5f,0,0},{1,0.5f,0},{0.5f,1,0},{0,0.5f,0}};
            p = Graphic3d_Vec3(points[n][0],points[n][1],points[n][2]);
        } else if (type == 41) {
            const float points[][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0.5f,0.5f,1}}; p = Graphic3d_Vec3(points[n][0],points[n][1],points[n][2]);
        } else if (type == 42) {
            const float points[][3] = {{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}}; p = Graphic3d_Vec3(points[n][0],points[n][1],points[n][2]);
        } else {
            require(parametric != nullptr,"Fixture lacks parametric coordinates");
            p = Graphic3d_Vec3((float)parametric[3*n],(float)parametric[3*n+1],(float)parametric[3*n+2]);
        }
        mesh->AddNodePosition(p); topology.Nodes.push_back(n);
        if (rational) { topology.RationalWeights.push_back(0.65+0.11*(n%5)); }
    }
    if (type == 42) { topology.Faces = {{0,3,2,1},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}}; }
    mesh->AddGenericCell(topology);
    return mesh;
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, bool nodal)
{
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(nodal ? "linear" : "constant",1,
        nodal ? RWCae::MeshPropertyBase::MappingType::Nodal : RWCae::MeshPropertyBase::MappingType::Elemental);
    result->Init<RWCae::MeshScalarProperty>();
    auto component = Handle(RWCae::MeshScalarProperty)::DownCast(result->GetComponent(0)); component->Init(nodal ? mesh->NbNodes() : 1);
    for (RWCae::MeshEntityId n = 0; n < (nodal ? mesh->NbNodes() : 1); ++n) {
        const auto p = nodal ? mesh->Nodes()[n] : Graphic3d_Vec3(0);
        component->SetValue(RWCae::MeshPropertyEntityInfo(n),nodal ? p.x()+2*p.y()+3*p.z() : 7);
    }
    result->UpdateComponents(); return result;
}
void writeFixture(const Handle(RWCae::MeshData)& mesh, int type, const std::string& stem = "", bool valid = true)
{
    vtkNew<vtkUnstructuredGrid> grid; vtkNew<vtkPoints> points; points->SetDataTypeToDouble();
    for (const auto& p : mesh->Nodes()) { points->InsertNextPoint(p.x(),p.y(),p.z()); }
    grid->SetPoints(points);
    const auto& topology = mesh->GenericCells()[0];
    std::vector<vtkIdType> ids(topology.Nodes.begin(),topology.Nodes.end());
    if (type == 42) {
        std::vector<vtkIdType> faces;
        for (const auto& face : topology.Faces) { faces.push_back((vtkIdType)face.size()); faces.insert(faces.end(),face.begin(),face.end()); }
        grid->InsertNextCell(type,(vtkIdType)ids.size(),ids.data(),(vtkIdType)topology.Faces.size(),faces.data());
    } else { grid->InsertNextCell(type,(vtkIdType)ids.size(),ids.data()); }
    vtkNew<vtkDoubleArray> values; values->SetName("temperature");
    for (const auto& p : mesh->Nodes()) { values->InsertNextValue(p.x()+2*p.y()+3*p.z()); }
    grid->GetPointData()->AddArray(values);
    vtkNew<vtkDoubleArray> cellValues; cellValues->SetName("pressure"); cellValues->InsertNextValue(7);
    grid->GetCellData()->AddArray(cellValues);
    if (!topology.Degrees.empty()) {
        vtkNew<vtkIntArray> degrees; degrees->SetName("HigherOrderDegrees"); degrees->SetNumberOfComponents(3);
        degrees->InsertNextTypedTuple(topology.Degrees.data()); grid->GetCellData()->SetHigherOrderDegrees(degrees);
    }
    if (!topology.RationalWeights.empty()) {
        vtkNew<vtkDoubleArray> weights; weights->SetName("RationalWeights");
        for (double value : topology.RationalWeights) { weights->InsertNextValue(value); }
        grid->GetPointData()->SetRationalWeights(weights);
    }
    std::filesystem::create_directories(VTK_CELL_FIXTURE_DIR);
    const std::string path = std::string(VTK_CELL_FIXTURE_DIR)+"/"+(stem.empty() ? "cell_"+std::to_string(type) : stem)+".vtu";
    vtkNew<vtkXMLUnstructuredGridWriter> writer; writer->SetFileName(path.c_str()); writer->SetInputData(grid);
    writer->SetDataModeToAscii(); require(writer->Write() != 0,"Cannot write VTK-cell fixture");
    RWVtk::VtkNativeReader reader;
    const bool imported = reader.Perform(path.c_str());
    if (!valid) { require(!imported && reader.GetMeshData().IsNull() && !reader.Report().errors.empty(),"Malformed native VTK metadata was accepted"); return; }
    require(imported,"Native VTK reader rejected concrete cell fixture");
    require(reader.GetMeshData()->GenericCells()[0].Type == type
        && reader.GetMeshData()->GenericCells()[0].Nodes == topology.Nodes,"VTK topology round trip changed cell type/nodes");
    require(reader.GetMeshData()->GenericCells()[0].Degrees == topology.Degrees
        && reader.GetMeshData()->GenericCells()[0].RationalWeights == topology.RationalWeights,"High-order metadata changed during VTK import");
}
void check(const MeshCutResult& result, const Handle(RWCae::MeshData)& source = Handle(RWCae::MeshData)())
{
    for (int part = 0; part < 3; ++part) {
        if (result.Portions[part].IsNull()) { continue; }
        const auto data = result.Portions[part]->GetMeshData();
        for (RWCae::MeshEntityId n = 0; n < data->NbNodes(); ++n) {
            const auto& p = data->Nodes()[n];
            MeshInterpolatedNode weights; require(result.Portions[part]->OriginNode(n,weights),"Source node mapping missing");
            Graphic3d_Vec3d interpolated(p);
            if (!source.IsNull()) {
                interpolated = Graphic3d_Vec3d(0);
                for (const auto& weight : weights.BaseNodes()) { interpolated += Graphic3d_Vec3d(source->Nodes()[weight.first])*weight.second; }
                const auto bounds = source->BoundingBox();
                const double tolerance = std::max(1e-6,(bounds.CornerMax().XYZ()-bounds.CornerMin().XYZ()).Modulus()*1e-3);
                if ((interpolated-Graphic3d_Vec3d(p)).Modulus() > 2*tolerance) {
                    std::cerr << "FE position " << p.x() << ' ' << p.y() << ' ' << p.z()
                        << " mapped " << interpolated.x() << ' ' << interpolated.y() << ' ' << interpolated.z() << '\n';
                    throw std::runtime_error("FE map exceeds faceted-geometry tolerance");
                }
            }
            const double expected = interpolated.x()+2*interpolated.y()+3*interpolated.z();
            near(result.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),expected,"Original-cell interpolation",
                std::max(1e-5,std::abs(expected)*4*std::numeric_limits<float>::epsilon()));
            double sum = 0; for (const auto& weight : weights.BaseNodes()) { sum += weight.second; }
            near(sum,1,"Original-cell partition of unity");
            if (part == 1) {
                const auto& definition = result.Definition;
                const double geometricTolerance = std::max(1e-5,Graphic3d_Vec3d(p).Modulus()*4*std::numeric_limits<float>::epsilon());
                if (definition.Shape == MeshCutShape::Plane) {
                    const auto& plane = definition.Plane;
                    near(plane.x()*p.x()+plane.y()*p.y()+plane.z()*p.z()+plane.w(),0,"Plane intersection point",geometricTolerance);
                } else {
                    auto d = Graphic3d_Vec3d(p)-definition.Center;
                    if (definition.Shape == MeshCutShape::Cylinder) { d -= definition.Axis*d.Dot(definition.Axis); }
                    near(d.Modulus(),definition.Radius,"Analytic curved intersection point",geometricTolerance);
                }
            }
        }
    }
}
void runMesh(const Handle(RWCae::MeshData)& mesh, int type)
{
    std::cout << "VTK_CELL_CHECK " << type << " nodes " << mesh->NbNodes() << '\n';
    MeshCutViewManager manager(mesh);
    Graphic3d_Vec3d center(0); for (const auto& p : mesh->Nodes()) { center += Graphic3d_Vec3d(p); }
    if (mesh->NbNodes() > 0) { center /= mesh->NbNodes(); }
    manager.AddPlane("plane",Graphic3d_Vec4d(1,0,0,-center.x()));
    manager.AddSphere("sphere",center,0.15);
    manager.AddCylinder("cylinder",center,Graphic3d_Vec3d(0,0,1),0.15);
    auto nodal = field(mesh,true), constant = field(mesh,false);
    for (const char* id : {"plane","sphere","cylinder"}) {
        manager.SetVisibility(id,7);
        if (type == 0) { manager.Evaluate(id); continue; }
        const auto cut = manager.Evaluate(id,nodal);
        if (std::string(id) == "plane") { require(cut.Portions[1]->GetMeshData()->NbNodes() > 0,"Canonical plane section is empty"); }
        check(cut);
        auto result = manager.Evaluate(id,constant);
        for (int part = 0; part < 3; ++part) {
            if (result.Fields[part].IsNull()) { continue; }
            for (int e = 0; e < result.Fields[part]->GetComponent(0)->NbValues(); ++e) {
                near(result.Fields[part]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(e)),7,"Original CellData mapping");
            }
        }
    }
}
void run(int type)
{
    const auto mesh = fixture(type); writeFixture(mesh,type); runMesh(mesh,type);
}
void extended()
{
    const auto quad = fixture(70,12,{2,3,0}); writeFixture(quad,70,"advanced_anisotropic_quad"); runMesh(quad,70);
    const auto wedge = fixture(73,30,{3,3,2}); writeFixture(wedge,73,"advanced_anisotropic_wedge"); runMesh(wedge,73);
    const auto large = fixture(72,343,{6,6,6});
    require(large->GenericCells()[0].Nodes.size() == 343,"Generic connectivity was truncated to 8-bit node count");
    writeFixture(large,72,"advanced_343_nodes"); runMesh(large,72);
    for (int type = 75; type <= 80; ++type) {
        const auto rational = fixture(type,0,{},true);
        writeFixture(rational,type,"advanced_rational_"+std::to_string(type)); runMesh(rational,type);
    }
    const auto volumeMesh = fixture(42);
    MeshCutViewManager volume(volumeMesh); volume.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.5));
    auto cut = volume.Evaluate("cut",field(volumeMesh,false));
    near(MeshCutIntegration::HeatFlux(cut,{0,0,0}).HeatFlow,7,"Generic volume heat flow");
    MeshCutViewManager surface(fixture(5));
    cut.Roots[1] = surface.SourceGeometry();
    bool rejected = false;
    try { MeshCutIntegration::HeatFlux(cut,{0,0,0}); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected,"Generic surface was integrated as a volume source");
    for (int type : {22,24,71}) {
        const auto mesh = fixture(type);
        auto polynomial = field(mesh,true);
        auto component = Handle(RWCae::MeshScalarProperty)::DownCast(polynomial->GetComponent(0));
        for (RWCae::MeshEntityId n = 0; n < mesh->NbNodes(); ++n) {
            const auto& p = mesh->Nodes()[n]; component->SetValue(RWCae::MeshPropertyEntityInfo(n),p.x()*p.x()+p.x()*p.y()+p.z()*p.z());
        }
        polynomial->UpdateComponents();
        MeshCutViewManager manager(mesh); manager.AddPlane("cut",Graphic3d_Vec4d(1,0,0,-0.3));
        const auto result = manager.Evaluate("cut",polynomial);
        const auto section = result.Portions[1]->GetMeshData();
        require(section->NbNodes() > 0,"Polynomial field section is empty");
        for (RWCae::MeshEntityId n = 0; n < section->NbNodes(); ++n) {
            const auto& p = section->Nodes()[n];
            near(result.Fields[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(n)),
                p.x()*p.x()+p.x()*p.y()+p.z()*p.z(),"Quadratic FE interpolation, not linearized vertex values");
        }
    }
    const auto original = fixture(70);
    for (bool rationalError : {false,true}) {
        Handle(RWCae::MeshData) invalid = new RWCae::MeshData();
        for (const auto& p : original->Nodes()) { invalid->AddNodePosition(p); }
        auto topology = original->GenericCells()[0];
        if (rationalError) { topology.RationalWeights.assign(topology.Nodes.size(),-1); }
        else { topology.Degrees = {3,3,0}; }
        invalid->AddGenericCell(topology);
        writeFixture(invalid,70,rationalError ? "invalid_weights" : "invalid_degrees",false);
    }
    std::cout << "VTK_EXTENDED_PASS anisotropic rational 343_nodes integration\n";
}
void external(const std::string& path)
{
    RWVtk::VtkNativeReader reader;
    if (!reader.Perform(path.c_str())) {
        throw std::runtime_error(reader.Report().errors.empty() ? "External VTK import failed" : reader.Report().errors.front());
    }
    const auto mesh = reader.GetMeshData();
    require(mesh->NbNodes() > 0,"External VTK dataset is empty");
    std::cout << "VTK_EXTERNAL_IMPORT " << path << " nodes " << mesh->NbNodes() << " cells " << mesh->NbIndexedElems() << '\n';
    MeshCutViewManager manager(mesh);
    gp_XYZ midpoint(0,0,0); Bnd_Box cellBounds;
    const auto& topology = *std::max_element(mesh->GenericCells().begin(),mesh->GenericCells().end(),[](const auto& a, const auto& b) {
        auto ca = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(a.Type));
        auto cb = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(b.Type));
        return ca->GetCellDimension() < cb->GetCellDimension();
    });
    for (auto id : topology.Nodes) {
        const auto& p = mesh->Nodes()[id]; midpoint += gp_XYZ(p.x(),p.y(),p.z()); cellBounds.Add(gp_Pnt(p.x(),p.y(),p.z()));
    }
    midpoint /= topology.Nodes.size();
    const auto& vertex = mesh->Nodes()[topology.Nodes.front()];
    midpoint = midpoint*0.2+gp_XYZ(vertex.x(),vertex.y(),vertex.z())*0.8;
    const double radius = (cellBounds.CornerMax().XYZ()-cellBounds.CornerMin().XYZ()).Modulus()*0.15;
    manager.AddPlane("plane",Graphic3d_Vec4d(1,0,0,-midpoint.X()));
    manager.AddSphere("sphere",Graphic3d_Vec3d(midpoint.X(),midpoint.Y(),midpoint.Z()),radius);
    manager.AddCylinder("cylinder",Graphic3d_Vec3d(midpoint.X(),midpoint.Y(),midpoint.Z()),Graphic3d_Vec3d(0,0,1),radius);
    auto values = field(mesh,true);
    for (const char* id : {"plane","sphere","cylinder"}) {
        manager.SetVisibility(id,2);
        const auto result = manager.Evaluate(id,values);
        require(result.Portions[1]->GetMeshData()->NbNodes() > 0,"External fixture probe has an empty section");
        check(result,mesh);
        std::cout << "VTK_EXTERNAL_CUT " << id << " nodes " << result.Portions[1]->GetMeshData()->NbNodes() << '\n';
    }
}
}
int main(int argc, char** argv)
{
    if (argc > 2 && std::string(argv[1]) == "--reject-import") {
        RWVtk::VtkNativeReader reader;
        if (!reader.Perform(argv[2]) && !reader.Report().errors.empty() && reader.GetMeshData().IsNull()) {
            std::cout << "VTK_INVALID_IMPORT_PASS " << reader.Report().errors.front() << '\n'; return 0;
        }
        std::cerr << "Invalid VTK connectivity was accepted\n"; return 1;
    }
    if (argc > 2 && std::string(argv[1]) == "--reject-cut") {
        try { external(argv[2]); }
        catch (const std::exception& error) {
            const std::string message(error.what());
            if (message.find("Polyhedron") != std::string::npos || message.find("polyhedron") != std::string::npos
                || message.find("closed") != std::string::npos) {
                std::cout << "VTK_INVALID_CUT_PASS " << message << '\n'; return 0;
            }
            std::cerr << message << '\n'; return 1;
        }
        catch (const Standard_Failure& error) { std::cerr << error.GetMessageString() << '\n'; return 1; }
        std::cerr << "Invalid non-watertight cell was cut silently\n"; return 1;
    }
    if (argc > 1 && std::string(argv[1]) == "--extended") {
        try { extended(); return 0; }
        catch (const Standard_Failure& error) { std::cerr << error.GetMessageString() << '\n'; }
        catch (const std::exception& error) { std::cerr << error.what() << '\n'; }
        return 1;
    }
    if (argc > 2 && std::string(argv[1]) == "--external") {
        try { external(argv[2]); return 0; }
        catch (const Standard_Failure& error) { std::cerr << error.GetMessageString() << '\n'; }
        catch (const std::exception& error) { std::cerr << error.what() << '\n'; }
        return 1;
    }
    int failed = 0, passed = 0;
    const int selected = argc > 1 ? std::stoi(argv[1]) : -1;
    for (int type = 0; type < 82; ++type) {
        if (selected >= 0 && selected != type) { continue; }
        auto cell = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(type));
        if (!cell) { continue; }
        try { run(type); ++passed; std::cout << "VTK_CELL_PASS " << type << " " << cell->GetClassName() << '\n'; }
        catch (const Standard_Failure& error) { ++failed; std::cerr << "VTK_CELL_FAIL " << type << " " << error.GetMessageString() << '\n'; }
        catch (const std::exception& error) { ++failed; std::cerr << "VTK_CELL_FAIL " << type << " " << error.what() << '\n'; }
    }
    if (selected < 0) {
        try { extended(); }
        catch (const Standard_Failure& error) { ++failed; std::cerr << "VTK_EXTENDED_FAIL " << error.GetMessageString() << '\n'; }
        catch (const std::exception& error) { ++failed; std::cerr << "VTK_EXTENDED_FAIL " << error.what() << '\n'; }
    }
    std::cout << "VTK_CELL_COVERAGE " << passed << "/" << passed+failed << '\n';
    return failed == 0 ? 0 : 1;
}

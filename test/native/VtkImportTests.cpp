#include <RWMeshConv/MeshLoader.h>
#ifdef OCCTV_TEST_VIEWER
#include <Viewer/InteractiveCaeMeshManager.h>
#include <ViewBase/IInteractiveObject.h>
#include <ViewCae/InteractiveCaeMesh.h>
#include <ViewCae/MeshPartGeneralCells.h>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <V3d_Viewer.hxx>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#endif
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_Printer.hxx>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#ifdef OCCTV_USE_VTK
#include <RWVtk/VtkNativeReader.h>
#endif

namespace {
void require(bool condition, const char* message)
{
    if (!condition) { throw std::runtime_error(message); }
}

class Capture : public Message_Printer {
public:
    mutable std::vector<std::string> Errors, Warnings;
protected:
    void send(const TCollection_AsciiString& text, Message_Gravity gravity) const override
    {
        if (gravity == Message_Fail) { Errors.push_back(text.ToCString()); }
        if (gravity == Message_Warning) { Warnings.push_back(text.ToCString()); }
    }
};

const std::string directory = VTK_IMPORT_FIXTURE_DIR;
std::string path(const std::string& name) { return directory + "/" + name; }
void write(const std::string& name, const std::string& contents)
{
    std::ofstream out(path(name), std::ios::binary); out << contents;
    require(out.good(), "Cannot write import fixture");
}

void fixtures()
{
    std::filesystem::create_directories(directory);
    std::ifstream tetra(std::string(VTK_FIXTURE_DIR) + "/tetra.vtu", std::ios::binary);
    require(tetra.good(), "Cannot read tetra fixture");
    write("tetra.vtu", std::string(std::istreambuf_iterator<char>(tetra), std::istreambuf_iterator<char>()));
    const std::string points = "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        "0 0 0 1 0 0 1 1 0 0 1 0</DataArray></Points>";
    const std::string topology = "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray>";
    const std::string fields = "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">0 1 2 3</DataArray></PointData>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7</DataArray></CellData>";
    write("polyline.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">" + points + "<Cells>" + topology +
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">4</DataArray></Cells>" + fields + "</Piece></UnstructuredGrid></VTKFile>");
    write("polyline.vtp", "<VTKFile type=\"PolyData\" byte_order=\"LittleEndian\"><PolyData>"
        "<Piece NumberOfPoints=\"4\" NumberOfVerts=\"0\" NumberOfLines=\"1\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">"
        + points + "<Lines>" + topology + "</Lines>" + fields + "</Piece></PolyData></VTKFile>");
    write("polyline.vtk", "# vtk DataFile Version 3.0\nPolyline\nASCII\nDATASET UNSTRUCTURED_GRID\n"
        "POINTS 4 float\n0 0 0 1 0 0 1 1 0 0 1 0\nCELLS 1 5\n4 0 1 2 3\nCELL_TYPES 1\n4\n"
        "POINT_DATA 4\nSCALARS temperature double 1\nLOOKUP_TABLE default\n0 1 2 3\n"
        "CELL_DATA 1\nSCALARS pressure double 1\nLOOKUP_TABLE default\n7\n");
    std::ifstream legacy(path("polyline.vtk"), std::ios::binary);
    write("polyline_upper.VTK", std::string(std::istreambuf_iterator<char>(legacy), std::istreambuf_iterator<char>()));
    write("invalid.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"invalid\"><UnstructuredGrid/></VTKFile>");
    write("missing.vtm", "<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet>"
        "<DataSet name=\"missing child\" file=\"absent.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    write("mapping.json", "[{\"description\":\"Mapped temperature\",\"valueType\":\"Vector\","
        "\"fieldMask\":\"temperature\",\"componentMask\":[\"missing\"],\"componentUnit\":[\"K\"]}]");
    write("tetra.vtm", "<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet>"
        "<DataSet file=\"tetra.vtu\"/></vtkMultiBlockDataSet></VTKFile>");
    write("tetra.vtm.series", "{\"files\":[{\"name\":\"tetra.vtm\",\"time\":0.25}]}");
    write("unsupported_section.vtk", "# vtk DataFile Version 3.0\nUnsupported section\nASCII\n"
        "DATASET UNSTRUCTURED_GRID\nPOINTS 4 float\n0 0 0 1 0 0 0 1 0 0 0 1\n"
        "CELLS 1 5\n4 0 1 2 3\nCELL_TYPES 1\n10\nUNSUPPORTED_SECTION 1\n");
    write("mixed.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"4\" NumberOfCells=\"3\">" + points + "<Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 0 1 2 3 0 1 2 3</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">2 6 10</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">2 4 9</DataArray></Cells>"
        "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">0 1 3 2</DataArray></PointData>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">5 7 11</DataArray></CellData>"
        "</Piece></UnstructuredGrid></VTKFile>");
    write("curved_line.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"3\" NumberOfCells=\"1\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        "0 0 0 1 0 0 0.5 0.3 0</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">3</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">21</DataArray></Cells>"
        "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">0 1 0.25</DataArray></PointData>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7</DataArray></CellData>"
        "</Piece></UnstructuredGrid></VTKFile>");
    write("polyvertex.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">" + points + "<Cells>" + topology +
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">2</DataArray></Cells>" + fields + "</Piece></UnstructuredGrid></VTKFile>");
    write("voxel.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"8\" NumberOfCells=\"1\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        "0 0 0 1 0 0 0 1 0 1 1 0 0 0 1 1 0 1 0 1 1 1 1 1</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3 4 5 6 7</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">8</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">11</DataArray></Cells>"
        "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">0 1 2 3 3 4 5 6</DataArray></PointData>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7</DataArray></CellData>"
        "</Piece></UnstructuredGrid></VTKFile>");
}

Handle(RWCae::MeshData) load(RWMeshConv::MeshLoader& loader, const std::string& name, const std::string& mapping = "")
{
    return loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(), path(name).c_str(), mapping.c_str());
}

void outcomes(const Handle(Capture)& capture)
{
    RWMeshConv::MeshLoader loader;
    auto mesh = load(loader, "tetra.vtu");
    auto report = loader.VtkReport();
    require(!mesh.IsNull() && report.succeeded && report.IsComplete(), "Portable import outcome");
    require(report.backend == "portable" && report.attempts.size() == 1, "Portable attempt missing");
    require(report.points == 4 && report.cells == 1 && report.fields == 2, "Published counts differ");
    require(report.source == path("tetra.vtu") && report.format == "VTU", "Source/format missing");
    for (const auto& name : {"polyline.vtu", "polyline.vtp", "polyline.vtk", "polyline_upper.VTK"}) {
        capture->Errors.clear(); mesh = load(loader, name); report = loader.VtkReport();
#ifdef OCCTV_USE_VTK
        require(!mesh.IsNull() && report.succeeded && report.IsComplete(), "Fallback failed");
        require(capture->Errors.empty(), "Successful fallback printed an overall failure");
        require(report.backend == "native-vtk" && report.attempts.size() == 2, "Fallback not identified");
        require(!report.attempts[0].succeeded && !report.attempts[0].errors.empty()
            && report.attempts[1].succeeded, "Rejected attempt diagnostics lost");
        require(report.points == 4 && report.cells == 1 && report.fields == 2, "Fallback fields lost");
        require(mesh->GenericCells().size() == 1 && mesh->GenericCells()[0].Type == 4, "Polyline topology lost");
        require(mesh->Properties()[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 7,
            "CellData no longer bound to original polyline");
#else
        require(mesh.IsNull() && !report.succeeded && !report.errors.empty(), "OFF silently accepted unsupported cells");
        require(report.attempts.size() == 1 && !capture->Errors.empty(), "OFF diagnostics missing");
#endif
    }
    capture->Errors.clear();
    require(load(loader, "invalid.vtu").IsNull(), "Invalid file accepted");
    report = loader.VtkReport();
    require(!report.succeeded && !report.errors.empty() && report.points == 0 && !capture->Errors.empty(), "Final errors missing");
#ifdef OCCTV_USE_VTK
    require(report.attempts.size() == 1, "Native fallback bypassed XML syntax rejection");
#endif
    require(load(loader, "polyline.vtu", path("mapping.json")).IsNull(), "Fallback silently discarded mapping");
    require(loader.VtkReport().attempts.size() == 1, "Mapping fallback policy changed");
    capture->Errors.clear(); capture->Warnings.clear();
    for (const auto& name : {"tetra.vtu", "tetra.vtm", "tetra.vtm.series"}) {
        mesh = load(loader, name, path("mapping.json")); report = loader.VtkReport();
        require(!mesh.IsNull() && report.succeeded && report.errors.empty() && !report.warnings.empty(), "Mapping warning lost");
        require(!report.IsComplete() && !capture->Warnings.empty(), "Warning completeness/printing lost");
    }
    require(capture->Errors.empty(), "Warning-only import printed failure");
    capture->Errors.clear(); capture->Warnings.clear(); loader.SetToReportMessages(false);
    require(load(loader, "missing.vtm").IsNull() && !loader.VtkReport().errors.empty(), "Missing child report lost");
    require(capture->Errors.empty(), "Quiet composite emitted failure");
    require(!load(loader, "tetra.vtm.series", path("mapping.json")).IsNull(), "Quiet warning import failed");
    require(capture->Warnings.empty() && !loader.VtkReport().warnings.empty(), "Quiet warning report lost");
    require(!load(loader, "tetra.vtu").IsNull() && loader.VtkReport().IsComplete(), "Old diagnostics leaked into next import");
}

#ifdef OCCTV_TEST_VIEWER
void viewer()
{
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(new Aspect_DisplayConnection(), false);
    Handle(V3d_Viewer) v = new V3d_Viewer(driver);
    Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(v);
    RWVtk::VtkImportReport report;
    require(Viewer::InteractiveCaeMeshManager::OpenMesh(context, "live", path("tetra.vtu"), "", "", 0, &report), "Viewer open failed");
    require(report.succeeded && report.fields == 2, "Viewer report missing");
    const auto old = ViewBase::IInteractiveObject::FindAisObjectById(*context, "live");
    require(!old.IsNull(), "Viewer object not registered");
    const auto oldMesh = Handle(ViewCae::InteractiveCaeMesh)::DownCast(old)->GetMeshData();
    context->Display(old, false);
    AIS_ListOfInteractive originalObjects; context->ObjectsInside(originalObjects);
    for (const auto& name : {"absent.vtu", "missing.vtm", "invalid.vtu"}) {
        require(!Viewer::InteractiveCaeMeshManager::OpenMesh(context, "live", path(name), "", "", 0, &report), "Viewer accepted bad import");
        require(!report.succeeded && !report.errors.empty(), "Viewer final diagnostics missing");
        const auto preserved = ViewBase::IInteractiveObject::FindAisObjectById(*context, "live");
        require(preserved == old && context->IsDisplayed(old), "Failed Viewer import replaced/erased old object");
        require(Handle(ViewCae::InteractiveCaeMesh)::DownCast(preserved)->GetMeshData() == oldMesh, "Failed Viewer import mutated old mesh");
    }
    require(Viewer::InteractiveCaeMeshManager::OpenMesh(context, "live", path("tetra.vtu"), "", "", 0, &report), "Viewer replacement failed");
    require(ViewBase::IInteractiveObject::FindAisObjectById(*context, "live") != old, "Successful replacement kept old object");
    AIS_ListOfInteractive objects; context->ObjectsInside(objects);
    for (const auto& object : objects) { require(object != old, "Replacement left old parent registered"); }
    require(objects.Size() <= originalObjects.Size(), "Replacement grew registered object count");
}

#ifdef OCCTV_USE_VTK
void ordinaryPrimitives()
{
    RWMeshConv::MeshLoader loader;
    RWVtk::VtkNativeReader native;
    require(native.Perform(path("tetra.vtu").c_str()), "Ordinary general volume import failed");
    Handle(ViewCaeGeom::MeshGeometry) volume = new ViewCaeGeom::MeshGeometryBase(new ViewCaeGeom::MeshBvhData(), native.GetMeshData());
    const auto boundary = ViewCae::MeshPartGeneralCells::BuildPrimitives(volume);
    require(boundary.size() == 4, "Ordinary Viewer general volume boundary missing");
    for (const auto& face : boundary) { require(face.Dimension == 3 && face.Size == 3, "General volume primitive type"); }
    for (const auto& name : {"polyline.vtk", "polyline.vtu", "polyline.vtp", "mixed.vtu", "curved_line.vtu", "polyvertex.vtu", "voxel.vtu"}) {
        const auto mesh = load(loader, name);
        require(!mesh.IsNull(), "Ordinary primitive fixture import failed");
        Handle(ViewCaeGeom::MeshGeometry) geometry = new ViewCaeGeom::MeshGeometryBase(new ViewCaeGeom::MeshBvhData(), mesh);
        const auto primitives = ViewCae::MeshPartGeneralCells::BuildPrimitives(geometry);
        require(!primitives.empty(), "Ordinary Viewer generic primitives absent");
        int counts[4] = {};
        for (const auto& primitive : primitives) {
            ++counts[primitive.Dimension];
            for (int n = 0; n < primitive.Size; ++n) {
                double sum = 0;
                Graphic3d_Vec3d position(0);
                for (const auto& weight : primitive.Vertices[n].Weights.BaseNodes()) {
                    sum += weight.second;
                    position += Graphic3d_Vec3d(mesh->Nodes()[weight.first]) * weight.second;
                }
                require(std::abs(sum - 1) < 1e-9 && (position - primitive.Vertices[n].Position).Modulus() < 1e-7,
                    "Ordinary Viewer original FE mapping invalid");
                const auto field = RWCae::MeshPropertyComponent(mesh->Properties()[1], 0);
                const double expected = std::string(name) == "mixed.vtu" ? (primitive.Element == 0 ? 5 : primitive.Element == 1 ? 7 : 11) : 7;
                require(ViewCae::MeshPartGeneralCells::Value(primitive, n, field) == expected,
                    "Ordinary Viewer original CellData mapping changed");
                if (std::string(name) == "mixed.vtu") {
                    const auto& p = primitive.Vertices[n].Position;
                    require(std::abs(ViewCae::MeshPartGeneralCells::Value(primitive, n,
                        RWCae::MeshPropertyComponent(mesh->Properties()[0], 0)) - p.x() - 2*p.y()) < 1e-7,
                        "Ordinary Viewer affine nodal field changed");
                }
                if (std::string(name) == "curved_line.vtu") {
                    const auto& p = primitive.Vertices[n].Position;
                    require(std::abs(ViewCae::MeshPartGeneralCells::Value(primitive, n,
                        RWCae::MeshPropertyComponent(mesh->Properties()[0], 0)) - p.x()*p.x()) < 1e-7,
                        "Ordinary Viewer quadratic original FE field changed");
                }
            }
        }
        require(std::string(name) == "voxel.vtu" ? counts[3] == 12 && counts[1] == 0
            : std::string(name) == "polyvertex.vtu" ? counts[0] == 4 && counts[1] == 0
            : std::string(name) == "curved_line.vtu" ? counts[1] > 3 : counts[1] == 3,
            "Ordinary Viewer curve/polyline segment count");
        if (std::string(name) == "mixed.vtu") { require(counts[0] == 2 && counts[2] == 2, "Ordinary Viewer point/surface count"); }
    }
}
#endif
#endif
}

int main()
{
    // Viewer links TKDraw, whose global interpreter finalizes Tcl on process exit.
#ifdef OCCTV_TEST_VIEWER
    Draw::GetInterpretor().Init();
#endif
    Handle(Capture) capture = new Capture(); Message::DefaultMessenger()->AddPrinter(capture);
    try {
        fixtures(); outcomes(capture);
#ifdef OCCTV_TEST_VIEWER
#ifdef OCCTV_USE_VTK
        ordinaryPrimitives();
#endif
#ifdef _WIN32
        viewer();
#else
        if (std::getenv("DISPLAY") != nullptr) { viewer(); }
        else { std::cout << "Viewer lifecycle skipped: no X display\n"; }
#endif
#else
        std::cout << "Viewer lifecycle skipped: Viewer target disabled\n";
#endif
        Message::DefaultMessenger()->RemovePrinter(capture);
        std::cout << "VTK_IMPORT_PASS" << std::endl; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "VTK_IMPORT_FAIL: " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "VTK_IMPORT_FAIL: " << error.what() << '\n'; }
    Message::DefaultMessenger()->RemovePrinter(capture); return 1;
}

#include <RWMeshConv/MeshLoader.h>
#include <RWVtk/VtkReferenceStager.h>
#include <RWVtk/VtkMultiBlockDataReader.h>
#ifdef OCCTV_TEST_VIEWER
#include <Viewer/InteractiveCaeMeshManager.h>
#include <ViewBase/IInteractiveObject.h>
#include <ViewCae/InteractiveCaeMesh.h>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <V3d_Viewer.hxx>
#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;
const fs::path root = fs::u8path(VTK_REFERENCE_FIXTURE_DIR);
void require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
std::string path(const std::string& name) { return (root / fs::u8path(name)).generic_u8string(); }
void write(const std::string& name, const std::string& text)
{
    const auto file = root / fs::u8path(name);
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary); out << text;
    require(out.good(), "Cannot write reference fixture");
}
bool contains(const std::vector<std::string>& messages, const std::string& needle)
{
    for (const auto& text : messages) { if (text.find(needle) != std::string::npos) { return true; } }
    return false;
}
std::string composite(const std::string& body)
{
    return "<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet>"
        + body + "</vtkMultiBlockDataSet></VTKFile>";
}
std::string ref(const std::string& name, const std::string& file)
{
    return "<DataSet name=\"" + name + "\" file=\"" + file + "\"/>";
}
std::string dataset(int shift, const std::string& numeric = "Float64", bool fields = true, bool remesh = false)
{
    const std::string cells = remesh ? "2" : "1";
    return "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid><Piece NumberOfPoints=\""
        + std::string(remesh ? "5" : "4") + "\" NumberOfCells=\"" + cells
        + "\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        + std::to_string(shift) + " 0 0 " + std::to_string(shift+1) + " 0 0 " + std::to_string(shift)
        + " 1 0 " + std::to_string(shift) + " 0 1 " + (remesh ? std::to_string(shift+1)+" 1 0" : "")
        + "</DataArray></Points><Cells><DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">"
        + (remesh ? "0 1 2 1 4 2" : "0 1 2 3")
        + "</DataArray><DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" + (remesh ? "3 6" : "4")
        + "</DataArray><DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" + (remesh ? "5 5" : "10")
        + "</DataArray></Cells>" + (fields ? "<PointData><DataArray type=\"" + numeric
            + "\" Name=\"temperature\" format=\"ascii\">0 1 2 3 " + (remesh ? "4" : "")
            + "</DataArray></PointData><CellData><DataArray type=\"" + numeric
            + "\" Name=\"pressure\" format=\"ascii\">1.123456789 " + (remesh ? "2.5" : "")
            + "</DataArray></CellData>" : "") + "</Piece></UnstructuredGrid></VTKFile>";
}
Handle(RWCae::MeshData) load(RWMeshConv::MeshLoader& loader, const std::string& name)
{
    return loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(), path(name).c_str());
}
Handle(RWCae::MeshNVecProperty) field(const Handle(RWCae::MeshData)& mesh, const char* name)
{
    for (const auto& p : mesh->Properties()) { if (p->Name() == name) { return p; } }
    throw std::runtime_error(std::string("Missing field: ") + name);
}
void fixtures()
{
    write("src/A/mesh.vtu", dataset(0, "Float32"));
    write("src/B/mesh.vtu", dataset(3));
    write("src/B/nested.vtm", composite(ref("right", "mesh.vtu")));
    write("src/root.vtm", composite(ref("left", "A/mesh.vtu")
        + "<Block name=\"nested\">" + ref("inside", "B/nested.vtm") + "</Block>"));
    write("src/sparse.vtm", composite(ref("left", "A/mesh.vtu") + ref("unmapped", "bare.vtu")));
    write("src/bare.vtu", dataset(3, "Float64", false));
    write("src/missing.vtm", composite(ref("lost", "B/absent.vtu")));
    write("src/invalid.vtu", "<VTKFile>");
    write("src/invalid.vtm", composite(ref("bad", "invalid.vtu")));
    write("src/duplicate.vtm", composite(ref("same", "A/mesh.vtu") + ref("same", "B/mesh.vtu")));
    write("src/cycle.vtm", composite(ref("again", "cycle.vtm")));
    write("src/C/remesh.vtu", dataset(10, "Float64", true, true));
    write("src/C/remesh.vtm", composite(ref("left", "remesh.vtu")));
    write("src/first.vtm", composite(ref("left", "A/mesh.vtu")));
    write("src/remesh.vtm.series", "{\"files\":[{\"name\":\"first.vtm\",\"time\":0.25},{\"name\":\"C/remesh.vtm\",\"time\":1.75}]}");
    write("src/line.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid><Piece NumberOfPoints=\"2\" NumberOfCells=\"1\">"
        "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">0 0 0 1 1 0</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">2</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">3</DataArray></Cells>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7</DataArray></CellData></Piece></UnstructuredGrid></VTKFile>");
    write("src/line.vtk", "# vtk DataFile Version 3.0\nline\nASCII\nDATASET UNSTRUCTURED_GRID\nPOINTS 2 float\n2 0 0 3 1 0\n"
        "CELLS 1 3\n2 0 1\nCELL_TYPES 1\n3\nCELL_DATA 1\nSCALARS pressure double\nLOOKUP_TABLE default\n11\n");
    write("src/empty.vtu", "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid><Piece NumberOfPoints=\"0\" NumberOfCells=\"1\">"
        "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\"></DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\"></DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">0</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">0</DataArray></Cells>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">19</DataArray></CellData></Piece></UnstructuredGrid></VTKFile>");
    write("src/general.vtm", composite(ref("xml", "line.vtu") + ref("legacy", "line.vtk") + ref("empty", "empty.vtu")));
    write("stage/sentinel.txt", "keep user data");
}
void tests()
{
    RWMeshConv::MeshLoader loader; loader.SetToReportMessages(false);
    auto original = load(loader, "src/root.vtm");
    require(!original.IsNull() && original->NbNodes() == 8 && original->NbIndexedElems() == 2, "Nested references failed");
    require(original->Properties().Size() == 2, "Mixed numeric fields split");
    const auto pressure = field(original, "pressure")->GetComponent(0);
    require(std::abs(pressure->Value(RWCae::MeshPropertyEntityInfo(1)) - 1.123456789) < 1e-12, "Float64 composite precision lost");
    auto sparse = load(loader, "src/sparse.vtm");
    require(!sparse.IsNull() && contains(loader.VtkReport().warnings, "missing values remain NaN")
        && std::isnan(field(sparse, "pressure")->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1))), "Missing composite field silently filled");

    loader.SetVtkStagingDirectory(path("stage").c_str());
    auto staged = load(loader, "src/root.vtm");
    auto report = loader.VtkReport();
    require(!staged.IsNull() && report.source == path("src/root.vtm") && !report.stagedSource.empty(), "Staging report/source lost");
    const fs::path stagedRoot = fs::u8path(report.stagedSource);
    const fs::path attempt = stagedRoot.parent_path().parent_path();
    size_t meshes = 0;
    for (const auto& file : fs::recursive_directory_iterator(attempt)) { if (file.path().filename() == "mesh.vtu") { ++meshes; } }
    require(meshes == 2, "Duplicate basenames overwrite each other");
    // Move the source tree away, then read only the rewritten references.
    const fs::path source = root / "src", hidden = root / "hidden";
    fs::rename(source, hidden);
    RWMeshConv::MeshLoader isolated; isolated.SetToReportMessages(false);
    auto frozen = isolated.OpenMesh(OSD_FileSystem::DefaultFileSystem(), report.stagedSource.c_str());
    fs::rename(hidden, source);
    require(!frozen.IsNull() && frozen->NbNodes() == 8 && frozen->Properties().Size() == 2, "Staged references still depend on source tree");
    require(std::abs(field(frozen, "pressure")->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1)) - 1.123456789) < 1e-12, "Staging changed field values");

    auto series = load(loader, "src/remesh.vtm.series");
    require(!series.IsNull() && series->SeriesSteps().size() == 2, "Staged series failed");
    const auto& step = series->SeriesSteps()[1];
    require(step.Mesh->NbNodes() == 5 && step.Mesh->NbIndexedElems() == 2 && step.Mesh->Nodes()[0].x() == 10
        && step.Mesh->Properties().Size() == 2 && field(step.Mesh, "pressure")->NbValues() == 2
        && step.Time == 1.75 && series->NbNodes() == 4 && series->NbIndexedElems() == 1, "Remeshed complete snapshot lost");

    const auto entries = [&]() { return std::distance(fs::directory_iterator(root / "stage"), fs::directory_iterator()); };
    const auto before = entries();
    require(load(loader, "src/missing.vtm").IsNull(), "Missing reference accepted");
    require(contains(loader.VtkReport().errors, "B/absent.vtu") && contains(loader.VtkReport().errors, "resolved")
        && entries() == before && fs::exists(root / "stage/sentinel.txt"), "Failed staging lost context or touched existing files");
    require(load(loader, "src/cycle.vtm").IsNull() && contains(loader.VtkReport().errors, "Cyclic"), "Staging cycle accepted");
    require(load(loader, "src/invalid.vtm").IsNull() && contains(loader.VtkReport().errors, "bad")
        && !loader.VtkReport().stagedSource.empty(), "Malformed staged leaf lost block context");
    loader.SetVtkStagingDirectory("");
    for (const auto& bad : {"src/missing.vtm", "src/duplicate.vtm", "src/cycle.vtm", "src/invalid.vtm"}) {
        require(load(loader, bad).IsNull() && !loader.VtkReport().errors.empty(), "Invalid composite accepted");
    }
    require(contains(loader.VtkReport().errors, "bad") && contains(loader.VtkReport().errors, "resolved"), "Reader child context missing");
    RWVtk::VtkResourceLimits limits; limits.MaxDepth = 1;
    loader.SetVtkResourceLimits(limits);
    require(load(loader, "src/root.vtm").IsNull() && contains(loader.VtkReport().errors, "depth"), "Reader depth limit ignored");
    loader.SetVtkStagingDirectory(path("stage").c_str());
    require(load(loader, "src/root.vtm").IsNull() && contains(loader.VtkReport().errors, "depth"), "Staging depth limit ignored");
    limits = RWVtk::VtkResourceLimits(); limits.MaxFiles = 2; loader.SetVtkResourceLimits(limits);
    require(load(loader, "src/root.vtm").IsNull() && contains(loader.VtkReport().errors, "files"), "Staging file limit ignored");
    loader.SetVtkStagingDirectory(path("stage/sentinel.txt").c_str()); loader.SetVtkResourceLimits(RWVtk::VtkResourceLimits());
    require(load(loader, "src/root.vtm").IsNull() && contains(loader.VtkReport().errors, "staging"), "Invalid staging destination accepted");
    loader.SetVtkStagingDirectory("");
    const auto general = load(loader, "src/general.vtm");
#ifdef OCCTV_USE_VTK
    require(!general.IsNull() && general->NbNodes() == 4 && general->NbIndexedElems() == 3
        && general->GenericCells().size() == 3 && general->GenericCells()[1].Nodes == std::vector<RWCae::MeshEntityId>({2,3})
        && general->GenericCells()[2].Type == 0, "Original generic composite topology lost");
    require(field(general, "pressure")->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 7
        && field(general, "pressure")->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1)) == 11
        && field(general, "pressure")->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(2)) == 19, "Generic original CellData offsets lost");
#else
    require(general.IsNull() && contains(loader.VtkReport().errors, "Unsupported VTK cell type"), "OFF generic composite silently accepted");
#endif
    for (const auto* numeric : {"Int64", "UInt64"}) {
        std::string xml = dataset(0, numeric);
        const auto pos = xml.find("1.123456789"); xml.replace(pos, 11, "9007199254740993");
        write("src/inexact.vtu", xml);
        require(load(loader, "src/inexact.vtu").IsNull() && contains(loader.VtkReport().errors, "exact double range"), "64-bit field silently lost precision");
    }
    for (const auto* component : {"x", "y"}) {
        std::string xml = dataset(0);
        const auto pos = xml.find("Name=\"pressure\"");
        xml.insert(pos, std::string("ComponentName0=\"") + component + "\" ");
        write(std::string("src/") + component + ".vtu", xml);
    }
    write("src/schema.vtm", composite(ref("x", "x.vtu") + ref("y", "y.vtu")));
    require(load(loader, "src/schema.vtm").IsNull() && contains(loader.VtkReport().errors, "component name mismatch"), "Composite component schema mismatch accepted");
    RWVtk::VtkMultiBlockDataReader reader; reader.SetToReportMessages(false);
    require(reader.Perform(path("src/root.vtm").c_str()), "Tree fixture read failed");
    const auto tree = reader.GetMeshDataTree();
    require(tree->SubNodes[0]->Name == "left" && tree->SubNodes[1]->Name == "nested", "XML block order changed");
    require(!reader.Perform(path("src/invalid.vtu").c_str()) && reader.GetMergedMeshData().IsNull()
        && reader.GetMeshDataTree().IsNull(), "Failed parse exposed previous reader mesh");
    reader.SetMeshDataTree(tree);
    require(!reader.Perform(path("src/invalid.vtm").c_str()) && tree->SubNodes.size() == 2
        && tree->SubNodes[0]->NodeMeshData->NbNodes() == 4
        && contains(reader.Report().errors, "Loaded-tree mutation"), "Failed reader mutated supplied tree");
}
#ifdef OCCTV_TEST_VIEWER
void viewer()
{
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(new Aspect_DisplayConnection(), false);
    Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(new V3d_Viewer(driver));
    RWVtk::VtkImportReport report;
    require(Viewer::InteractiveCaeMeshManager::OpenMesh(context, "reference-live", path("src/root.vtm"), "", "", 0,
        &report, path("stage")), "Ordinary Viewer staged import failed");
    require(report.succeeded && !report.stagedSource.empty() && report.source == path("src/root.vtm"), "Ordinary Viewer staging report missing");
    const auto old = ViewBase::IInteractiveObject::FindAisObjectById(*context, "reference-live");
    const auto mesh = Handle(ViewCae::InteractiveCaeMesh)::DownCast(old)->GetMeshData();
    context->Display(old, false);
    for (const auto& bad : {"src/missing.vtm", "src/cycle.vtm", "src/invalid.vtm"}) {
        require(!Viewer::InteractiveCaeMeshManager::OpenMesh(context, "reference-live", path(bad), "", "", 0,
            &report, path("stage")) && !report.errors.empty(), "Ordinary Viewer staging failure accepted");
        require(ViewBase::IInteractiveObject::FindAisObjectById(*context, "reference-live") == old && context->IsDisplayed(old)
            && Handle(ViewCae::InteractiveCaeMesh)::DownCast(old)->GetMeshData() == mesh, "Staging failure changed live Viewer model");
    }
}
#endif
}
int main()
{
#ifdef OCCTV_TEST_VIEWER
    Draw::GetInterpretor().Init();
#endif
    try {
        fixtures(); tests();
#ifdef OCCTV_TEST_VIEWER
#ifdef _WIN32
        viewer();
#else
        if (std::getenv("DISPLAY")) { viewer(); }
#endif
#endif
        std::cout << "VTK_REFERENCE_PASS" << std::endl; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "VTK_REFERENCE_FAIL: " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "VTK_REFERENCE_FAIL: " << error.what() << '\n'; }
    return 1;
}

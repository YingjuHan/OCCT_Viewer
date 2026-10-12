#include <RWMeshConv/MeshLoader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace {
void require(bool ok, const char* message) { if (!ok) { throw std::runtime_error(message); } }
const std::string root = VTK_FORMAT_FIXTURE_DIR;
const std::string geometry = "# vtk DataFile Version 3.0\nformats\nASCII\nDATASET UNSTRUCTURED_GRID\n"
    "POINTS 4 float\n0 0 0 1 0 0 0 1 0 0 0 1\nCELLS 1 5\n4 0 1 2 3\nCELL_TYPES 1\n10\n";
void write(const std::string& name, const std::string& contents)
{
    std::ofstream file(root + "/" + name, std::ios::binary); file << contents;
    require(file.good(), "Cannot write format fixture");
}
void legacy()
{
    RWVtk::VtkLegacyReader reader; reader.SetToReportMessages(false);
    const std::string data = geometry + "POINT_DATA 4\nTEXTURE_COORDINATES uv 2 float\n0 0 1 0 0 1 1 1\n"
        "COLOR_SCALARS rgba 4\n1 0 0 1 0 1 0 1 0 0 1 1 1 1 1 1\n"
        "NORMALS normal float\n0 0 1 0 0 1 0 0 1 0 0 1\n"
        "SCALARS density float\nLOOKUP_TABLE custom\n0 1 2 3\n"
        "LOOKUP_TABLE custom 2\n1 0 0 1 0 1 0 1\n"
        "CELL_DATA 1\nFIELD fields 2\npressure 1 1 double\n7\nmetadata 1 2 int\n12 13\n";
    std::istringstream stream(data);
    require(reader.PerformStream(stream, "supported.vtk"), "Remaining supported Legacy sections failed");
    const auto mesh = reader.GetMeshData();
    require(mesh->Properties().Size() == 5, "Legacy arrays lost");
    require(mesh->Properties()[0]->NbRawComponents() == 2 && mesh->Properties()[1]->NbRawComponents() == 4,
        "Texture/color component cardinality lost");
    require(mesh->Properties()[0]->GetComponent(1)->Value(RWCae::MeshPropertyEntityInfo(2)) == 1,
        "Texture coordinate values lost");
    require(mesh->Properties()[4]->IsElemental()
        && mesh->Properties()[4]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 7,
        "Original Legacy CellData mapping lost");
    require(reader.Report().warnings.size() == 2, "Lookup table/metadata diagnostic missing");
    for (const auto& suffix : {"POINT_DATA 4\nTEXTURE_COORDINATES uv 5 float\n",
        "POINT_DATA 4\nCOLOR_SCALARS bad 1\n2 0 0 0\n", "POINT_DATA 4\nSCALARS x mystery\nLOOKUP_TABLE default\n0 1 2 3\n",
        "UNSUPPORTED_SECTION 1\n"}) {
        std::istringstream invalid(geometry + suffix);
        require(!reader.PerformStream(invalid, "invalid.vtk") && !reader.Report().errors.empty(), "Unsupported/invalid Legacy accepted silently");
    }
    write("unsupported.vtk", geometry + "UNSUPPORTED_SECTION 1\n");
    write("structured.vtk", "# vtk DataFile Version 3.0\nunsupported\nASCII\nDATASET STRUCTURED_POINTS\nDIMENSIONS 2 2 2\nORIGIN 0 0 0\nSPACING 1 1 1\n");
    RWMeshConv::MeshLoader loader; loader.SetToReportMessages(false);
    for (const auto& name : {"unsupported.vtk", "structured.vtk"}) {
        require(loader.OpenMesh(OSD_FileSystem::DefaultFileSystem(), (root + "/" + name).c_str()).IsNull(), "Fallback bypassed unsupported format contract");
        require(loader.VtkReport().attempts.size() == 1, "Unsupported section used native fallback");
    }
    std::string binary = "# vtk DataFile Version 3.0\nbinary attributes\nBINARY\nDATASET UNSTRUCTURED_GRID\nPOINTS 4 float\n";
    auto word = [&](uint32_t value) { for (int n = 3; n >= 0; --n) { binary += (char)(value >> (n*8)); } };
    for (float value : {0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f}) {
        uint32_t bits; std::memcpy(&bits, &value, 4); word(bits);
    }
    binary += "\nCELLS 1 5\n"; for (uint32_t value : {4,0,1,2,3}) { word(value); }
    binary += "\nCELL_TYPES 1\n"; word(10);
    binary += "\nPOINT_DATA 4\nCOLOR_SCALARS rgba 4\n";
    for (unsigned char value : {255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}) { binary += (char)value; }
    binary += "\nTEXTURE_COORDINATES uv 2 float\n";
    for (float value : {0.f,0.f,1.f,0.f,0.f,1.f,1.f,1.f}) { uint32_t bits; std::memcpy(&bits, &value, 4); word(bits); }
    binary += '\n'; std::istringstream bytes(binary);
    require(reader.PerformStream(bytes, "binary.vtk"), "Binary Legacy color/texture attributes failed");
    require(reader.GetMeshData()->Properties().Size() == 2
        && reader.GetMeshData()->Properties()[0]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 255,
        "Binary Legacy raw color byte values changed");
}
void xmlScope()
{
    const std::string prefix = "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
        "<Piece NumberOfPoints=\"4\" NumberOfCells=\"1\"><Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        "0 0 0 1 0 0 0 1 0 0 0 1</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">10</DataArray></Cells>";
    const std::string field = "<DataArray type=\"Float64\" Name=\"same\" format=\"ascii\">0 1 2 3</DataArray>";
    RWVtk::VtkUnstructuredGridReader reader; reader.SetToReportMessages(false);
    for (const auto& body : std::vector<std::string>{"<UnsupportedSection/>", "<PointData>" + field + field + "</PointData>"}) {
        std::istringstream stream(prefix + body + "</Piece></UnstructuredGrid></VTKFile>");
        require(!reader.PerformStream(stream, "invalid.vtu") && !reader.Report().errors.empty(),
            "Unsupported XML section/duplicate field was silently ignored");
    }
}
}
int main()
{
    try {
        std::filesystem::create_directories(root); legacy(); xmlScope();
        std::cout << "VTK_FORMAT_PASS\n"; return 0;
    } catch (const Standard_Failure& error) { std::cerr << "VTK_FORMAT_FAIL: " << error.GetMessageString() << '\n'; }
    catch (const std::exception& error) { std::cerr << "VTK_FORMAT_FAIL: " << error.what() << '\n'; }
    return 1;
}

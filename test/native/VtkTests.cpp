#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkPolyDataReader.h>
#include <RWVtk/VtkSeriesReader.h>
#include <RWCae/MeshAdaptor.h>
#include <RWCae/MeshNVecProperty.h>
#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

static std::string encode64(const std::string& bytes)
{
    const char* digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    for (size_t i = 0; i < bytes.size(); i += 3) {
        unsigned value = static_cast<unsigned char>(bytes[i]) << 16;
        if (i + 1 < bytes.size()) { value |= static_cast<unsigned char>(bytes[i+1]) << 8; }
        if (i + 2 < bytes.size()) { value |= static_cast<unsigned char>(bytes[i+2]); }
        result += digits[(value >> 18) & 63];
        result += digits[(value >> 12) & 63];
        result += i + 1 < bytes.size() ? digits[(value >> 6) & 63] : '=';
        result += i + 2 < bytes.size() ? digits[value & 63] : '=';
    }
    return result;
}

template<typename T>
static void appendNumber(std::string& data, T value, bool bigEndian = false)
{
    char bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
    if (bigEndian) { std::reverse(bytes, bytes + sizeof(T)); }
    data.append(bytes, sizeof(T));
}

static std::string xmlMesh(const std::string& points, const std::string& extra = "",
                           const std::string& root = "", const std::string& cellValue = "7", int cellType = 10)
{
    return "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\" " + root + ">"
           "<UnstructuredGrid><Piece NumberOfPoints=\"4\" NumberOfCells=\"1\"><Points>" + points +
           "</Points><Cells><DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>"
           "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray>"
           "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" + std::to_string(cellType) + "</DataArray></Cells>"
           "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">0 1 2 3</DataArray></PointData>"
           "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">" + cellValue +
           "</DataArray></CellData></Piece></UnstructuredGrid>" + extra + "</VTKFile>";
}

static void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void basicXml()
{
    std::istringstream input(
        "<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\">"
        "<UnstructuredGrid><Piece NumberOfPoints=\"4\" NumberOfCells=\"1\">"
        "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
        "0 0 0 1 0 0 0 1 0 0 0 1</DataArray></Points><Cells>"
        "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>"
        "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray>"
        "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">10</DataArray></Cells>"
        "<PointData><DataArray type=\"Float64\" Name=\"temperature\" format=\"ascii\">"
        "0 1 2 3</DataArray></PointData>"
        "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7</DataArray></CellData>"
        "</Piece></UnstructuredGrid></VTKFile>");
    RWVtk::VtkUnstructuredGridReader reader;
    require(reader.PerformStream(input, "test.vtu"), "VTU input failed");
    require(reader.GetMeshData()->NbNodes() == 4, "VTU points lost");
    require(reader.GetMeshData()->NbIndexedElems() == 1, "VTU cell lost");
    require(reader.GetMeshData()->Properties().Size() == 2, "VTU fields lost");
}

static void binaryXml()
{
    for (bool compressed : {false, true}) {
        for (int encoding : {0, 1, 2}) {
            std::string values;
            for (double value : {0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.}) { appendNumber(values, value); }
            std::string header, body;
            if (compressed) {
                uLongf length = compressBound(values.size());
                body.resize(length);
                require(compress2(reinterpret_cast<Bytef*>(&body[0]), &length,
                                  reinterpret_cast<const Bytef*>(values.data()), values.size(), Z_DEFAULT_COMPRESSION) == Z_OK,
                        "Test compression failed");
                body.resize(length);
                for (uint32_t value : {1u,static_cast<uint32_t>(values.size()),static_cast<uint32_t>(values.size()),static_cast<uint32_t>(length)}) {
                    appendNumber(header, value);
                }
            } else { appendNumber(header, static_cast<uint32_t>(values.size())); body = values; }
            const std::string data = encoding == 2 ? header + body : encode64(header) + encode64(body);
            const std::string points = "<DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\""
                                      + std::string(encoding == 0 ? "binary" : "appended") + "\" offset=\"0\">"
                                      + (encoding == 0 ? data : "") + "</DataArray>";
            const std::string appended = encoding ? "<AppendedData encoding=\"" + std::string(encoding == 2 ? "raw" : "base64")
                                                   + "\">_" + data + "</AppendedData>" : "";
            std::istringstream input(xmlMesh(points, appended, compressed ? "compressor=\"vtkZLibDataCompressor\"" : ""));
            RWVtk::VtkUnstructuredGridReader reader;
            require(reader.PerformStream(input, "binary.vtu"), "VTK binary/compressed/raw input failed");
            require(reader.GetMeshData()->Nodes()[1].x() == 1.0f, "VTK binary coordinates corrupted");
            require(reader.Report().points == 4 && reader.Report().fields == 2, "VTK import counts missing");
        }
    }
    std::string values, header;
    for (double value : {0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.}) { appendNumber(values, value, true); }
    appendNumber(header, static_cast<uint64_t>(values.size()), true);
    std::string xml = xmlMesh("<DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"binary\">"
                              + encode64(header + values) + "</DataArray>", "", "header_type=\"UInt64\"");
    xml.replace(xml.find("LittleEndian"), 12, "BigEndian");
    std::istringstream input(xml);
    RWVtk::VtkUnstructuredGridReader reader;
    require(reader.PerformStream(input, "endian.vtu"), "BigEndian input failed");
    require(reader.GetMeshData()->Nodes()[1].x() == 1.0f, "BigEndian values corrupted");
}

static void malformedXml()
{
    const std::string points = "<DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
                               "0 0 0 1 0 0 0 1 0 0 0 1</DataArray>";
    for (const std::string& xml : {xmlMesh(points, "", "", "7 8"), xmlMesh(points, "", "", "7", 99),
                                  xmlMesh(points, "", "", "7", 42)}) {
        std::istringstream input(xml);
        RWVtk::VtkUnstructuredGridReader reader;
        require(!reader.PerformStream(input, "invalid.vtu"), "Invalid VTK accepted");
        require(!reader.Report().errors.empty(), "Invalid VTK lacks diagnostics");
    }
    RWVtk::VtkUnstructuredGridReader repeated;
    for (int i = 0; i < 2; ++i) {
        std::istringstream input(xmlMesh(points));
        require(repeated.PerformStream(input, "repeat.vtu"), "Repeated VTU read failed");
        require(repeated.GetMeshData()->Properties()[1]->GetComponent(0)->NbValues() == 1, "Repeated VTU cell counts accumulate");
    }
}

static void legacyVtk()
{
    for (bool binary : {false, true}) {
        std::ostringstream data;
        data << "# vtk DataFile Version 3.0\nTetra fixture\n" << (binary ? "BINARY" : "ASCII")
             << "\nDATASET UNSTRUCTURED_GRID\nPOINTS 4 double\n";
        if (binary) {
            std::string bytes;
            for (double value : {0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.,1.}) { appendNumber(bytes, value, true); }
            data << bytes;
        } else { data << "0 0 0 1 0 0 0 1 0 0 0 1"; }
        data << "\nCELLS 1 5\n";
        if (binary) {
            std::string bytes;
            for (int32_t value : {4,0,1,2,3}) { appendNumber(bytes, value, true); }
            data << bytes;
        } else { data << "4 0 1 2 3"; }
        data << "\nCELL_TYPES 1\n";
        if (binary) { std::string bytes; appendNumber(bytes, int32_t(10), true); data << bytes; }
        else { data << "10"; }
        data << "\nCELL_DATA 1\nSCALARS pressure double\nLOOKUP_TABLE default\n";
        if (binary) { std::string bytes; appendNumber(bytes, 7.0, true); data << bytes; }
        else { data << "7"; }
        data << '\n';
        data << "POINT_DATA 4\nVECTORS displacement double\n";
        if (binary) {
            std::string bytes;
            for (double value : {1.,0.,0.,1.,0.,0.,1.,0.,0.,1.,0.,0.}) { appendNumber(bytes, value, true); }
            data << bytes;
        } else { data << "1 0 0 1 0 0 1 0 0 1 0 0"; }
        data << '\n';
        std::istringstream input(data.str());
        RWVtk::VtkLegacyReader reader;
        require(reader.PerformStream(input, "tetra.vtk"), "Legacy ASCII/binary input failed");
        require(reader.GetMeshData()->NbIndexedElems() == 1 && reader.GetMeshData()->Properties().Size() == 2,
                "Legacy cells/fields lost");
        require(reader.GetMeshData()->Properties()[0]->IsVector3(), "Legacy vector metadata lost");
    }
}

static std::string polyXml(const std::string& connectivity = "0 1 2", const std::string& offset = "3")
{
    return "<VTKFile type=\"PolyData\" byte_order=\"LittleEndian\"><PolyData>"
           "<Piece NumberOfPoints=\"4\" NumberOfVerts=\"0\" NumberOfLines=\"0\" NumberOfPolys=\"1\" NumberOfStrips=\"1\">"
           "<Points><DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">"
           "0 0 0 1 0 0 0 1 0 1 1 0</DataArray></Points>"
           "<Strips><DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">0 1 2 3</DataArray>"
           "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">4</DataArray></Strips>"
           "<Polys><DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">" + connectivity + "</DataArray>"
           "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" + offset + "</DataArray></Polys>"
           "<CellData><DataArray type=\"Float64\" Name=\"pressure\" format=\"ascii\">7 11</DataArray></CellData>"
           "</Piece></PolyData></VTKFile>";
}

static void polyData()
{
    RWVtk::VtkPolyDataReader reader;
    std::istringstream input(polyXml());
    require(reader.PerformStream(input, "surface.vtp"), "PolyData import failed");
    auto mesh = reader.GetMeshData();
    require(mesh->NbIndexedElems() == 3, "PolyData strip triangulation lost");
    auto prop = mesh->Properties()[0]->GetComponent(0);
    require(prop->Value(RWCae::MeshPropertyEntityInfo(0)) == 7
         && prop->Value(RWCae::MeshPropertyEntityInfo(1)) == 11
         && prop->Value(RWCae::MeshPropertyEntityInfo(2)) == 11, "PolyData CellData order corrupted");
    RWCae::MeshAdaptor<RWCae::MeshElementTriangle> adaptor(*mesh);
    for (int i = 1; i < 3; ++i) {
        const auto& elem = adaptor.GetElement(i);
        const auto a = mesh->Nodes()[elem[1]] - mesh->Nodes()[elem[0]];
        const auto b = mesh->Nodes()[elem[2]] - mesh->Nodes()[elem[0]];
        require(Graphic3d_Vec3::Cross(a, b).z() > 0, "Triangle strip winding alternates");
    }
    for (const auto& xml : {polyXml("0 1 99"), polyXml("0 1 4294967298"), polyXml("0 1 2", "2"),
                            polyXml("0 1 2", "-1"), polyXml("0 1 garbage")}) {
        std::istringstream invalid(xml);
        require(!reader.PerformStream(invalid, "invalid.vtp") && !reader.Report().errors.empty(), "Malformed PolyData accepted");
    }
    std::istringstream again(polyXml());
    require(reader.PerformStream(again, "surface.vtp") && reader.GetMeshData()->NbNodes() == 4, "Reader state not reset");
}

static void compositeSeries()
{
    RWVtk::VtkSeriesReader series;
    require(series.Perform(VTK_FIXTURE_DIR "tetra.vtm.series"), "Composite series failed");
    auto mesh = series.GetMergedMeshData();
    require(mesh->NbNodes() == 4 && mesh->NbIndexedElems() == 1 && mesh->Properties().Size() == 4, "Series topology/fields lost");
    for (const auto& property : mesh->Properties()) {
        require(property->TimeStep() == 0 || property->TimeStep() == 1, "Series step index corrupted");
        require(property->Time() == (property->TimeStep() == 0 ? 0.25 : 1.75), "Series manifest time ignored");
        if (property->IsElemental()) {
            require(property->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 7, "Time-series CellData lost");
        }
    }
    require(series.Report().fields == 4, "Series report missing");
    require(series.Perform(VTK_FIXTURE_DIR "tetra.vtm.series") && series.GetMergedMeshData()->Properties().Size() == 4,
            "Repeated series read accumulates state");
    std::istringstream broken("<VTKFile type=\"vtkMultiBlockDataSet\" byte_order=\"LittleEndian\"><vtkMultiBlockDataSet>"
        "<DataSet index=\"0\" file=\"tetra.vtu\"/><DataSet index=\"1\" file=\"missing.vtu\"/>"
        "</vtkMultiBlockDataSet></VTKFile>");
    RWVtk::VtkMultiBlockDataReader composite;
    require(!composite.PerformStream(broken, VTK_FIXTURE_DIR "broken.vtm") && !composite.Report().IsComplete(),
            "Missing composite child silently ignored");
    std::istringstream invalidSeries("{\"files\":{}}");
    require(!series.PerformStream(invalidSeries, "invalid.vtm.series"), "Invalid series files accepted");
}

static void topologyAndMetadata()
{
    RWVtk::VtkUnstructuredGridReader reader;
    require(reader.Perform(VTK_FIXTURE_DIR "multiple_piece.vtu"), "Multiple Piece import failed");
    auto mesh = reader.GetMeshData();
    require(mesh->NbNodes() == 8 && mesh->NbIndexedElems() == 2 && mesh->Properties().Size() == 2, "Multiple Piece topology incorrect");
    require(mesh->Properties()[0]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(4)) == 4
         && mesh->Properties()[1]->GetComponent(0)->Value(RWCae::MeshPropertyEntityInfo(1)) == 11, "Piece field offsets incorrect");
    RWCae::MeshAdaptor<RWCae::MeshElementTetrahedron> tetra(*mesh);
    require(tetra.GetElement(1)[0] == 4, "Piece connectivity offset missing");
    require(reader.Perform(VTK_FIXTURE_DIR "polyhedron.vtu"), "Polyhedron face stream failed");
    RWCae::MeshAdaptor<RWCae::MeshElementPolyhedron> poly(*reader.GetMeshData());
    std::vector<RWCae::MeshEntityId> nodes;
    RWCae::MeshElemType type;
    poly.FaceNodes(poly.GetElement(0), 0, nodes, type);
    require(nodes == std::vector<RWCae::MeshEntityId>({1,2,3}), "Polyhedron face rotation incorrect");
    require(reader.Perform(VTK_FIXTURE_DIR "deformed_tetra.vtu"), "Vector metadata import failed");
    auto displacement = reader.GetMeshData()->Properties()[1];
    require(displacement->IsVector3() && displacement->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0)) == 1,
            "Active VTK vector role not preserved");
}

int main()
{
    try {
        basicXml();
        binaryXml();
        malformedXml();
        legacyVtk();
        polyData();
        compositeSeries();
        topologyAndMetadata();
        std::cout << "VTK reader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

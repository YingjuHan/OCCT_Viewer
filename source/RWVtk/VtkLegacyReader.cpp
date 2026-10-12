#include <RWVtk/VtkLegacyReader.h>
#include <RWVtk/VtkUnstructuredGridReader.h>
#include <RWVtk/VtkPolyDataReader.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
struct Array {
    std::string name;
    std::string role;
    int components = 1;
    std::vector<double> values;
    explicit Array(const std::string& theName = "") : name(theName) {}
};

std::string escapeXml(const std::string& value)
{
    std::string result;
    for (char ch : value) {
        if (ch == '&') { result += "&amp;"; }
        else if (ch == '<') { result += "&lt;"; }
        else if (ch == '"') { result += "&quot;"; }
        else { result += ch; }
    }
    return result;
}

void emitArray(std::ostream& out, const Array& array, const char* type = "Float64")
{
    out << "<DataArray type=\"" << type << "\" Name=\"" << escapeXml(array.name)
        << "\" NumberOfComponents=\"" << array.components << "\" format=\"ascii\">";
    for (double value : array.values) { out << std::setprecision(17) << value << ' '; }
    out << "</DataArray>";
}

void emitFields(std::ostream& out, const char* tag, const std::vector<Array>& fields)
{
    out << '<' << tag;
    for (const char* role : {"Vectors", "Normals", "Tensors"}) {
        for (const auto& field : fields) {
            if (field.role == role) { out << ' ' << role << "=\"" << escapeXml(field.name) << '"'; break; }
        }
    }
    out << '>';
    for (const auto& field : fields) { emitArray(out, field); }
    out << "</" << tag << '>';
}

size_t count(std::istream& input)
{
    long long value = -1;
    if (!(input >> value) || value < 0 || value > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Invalid legacy VTK count");
    }
    return static_cast<size_t>(value);
}

template<typename T>
double readBigEndian(std::istream& input)
{
    unsigned char bytes[sizeof(T)];
    if (!input.read(reinterpret_cast<char*>(bytes), sizeof(T))) { throw std::runtime_error("Truncated legacy binary data"); }
    const uint16_t endianProbe = 1;
    if (*reinterpret_cast<const unsigned char*>(&endianProbe) == 1) { std::reverse(bytes, bytes + sizeof(T)); }
    T value;
    std::memcpy(&value, bytes, sizeof(T));
    return static_cast<double>(value);
}

Array readArray(std::istream& input, const std::string& type, size_t tuples, int components, bool binary)
{
    const std::vector<std::string> supported = {"float", "double", "int", "vtkTypeInt32", "unsigned_int",
        "short", "unsigned_short", "char", "signed_char", "unsigned_char", "long_long", "vtkTypeInt64", "unsigned_long_long"};
    if (std::find(supported.begin(), supported.end(), type) == supported.end()) {
        throw std::runtime_error("Unsupported legacy numeric type: " + type);
    }
    if (components <= 0 || tuples > std::numeric_limits<size_t>::max() / static_cast<size_t>(components)) {
        throw std::runtime_error("Invalid legacy VTK array shape");
    }
    Array result;
    result.components = components;
    result.values.reserve(tuples * components);
    if (binary) {
        std::string line;
        std::getline(input, line);
    }
    for (size_t i = 0; i < tuples * components; ++i) {
        double value;
        if (!binary) {
            std::string token;
            if (!(input >> token)) { throw std::runtime_error("Truncated legacy ASCII data"); }
            char* end = nullptr;
            value = std::strtod(token.c_str(), &end);
            if (end != token.c_str() + token.size()) { throw std::runtime_error("Invalid legacy numeric value"); }
        } else if (type == "float") { value = readBigEndian<float>(input); }
        else if (type == "double") { value = readBigEndian<double>(input); }
        else if (type == "int" || type == "vtkTypeInt32") { value = readBigEndian<int32_t>(input); }
        else if (type == "unsigned_int") { value = readBigEndian<uint32_t>(input); }
        else if (type == "short") { value = readBigEndian<int16_t>(input); }
        else if (type == "unsigned_short") { value = readBigEndian<uint16_t>(input); }
        else if (type == "char" || type == "signed_char") { value = readBigEndian<int8_t>(input); }
        else if (type == "unsigned_char") { value = readBigEndian<uint8_t>(input); }
        else if (type == "long_long" || type == "vtkTypeInt64") { value = readBigEndian<int64_t>(input); }
        else if (type == "unsigned_long_long") { value = readBigEndian<uint64_t>(input); }
        else { throw std::runtime_error("Unsupported legacy binary numeric type: " + type); }
        static const std::map<std::string, std::pair<double, double>> ranges = {
            {"char", {-128, 127}}, {"signed_char", {-128, 127}}, {"unsigned_char", {0, 255}},
            {"short", {-32768, 32767}}, {"unsigned_short", {0, 65535}},
            {"int", {-2147483648.0, 2147483647.0}}, {"vtkTypeInt32", {-2147483648.0, 2147483647.0}},
            {"unsigned_int", {0, 4294967295.0}}, {"long_long", {-9007199254740991.0, 9007199254740991.0}},
            {"vtkTypeInt64", {-9007199254740991.0, 9007199254740991.0}}, {"unsigned_long_long", {0, 9007199254740991.0}}};
        const auto range = ranges.find(type);
        if (range != ranges.end() && (!std::isfinite(value) || std::trunc(value) != value
            || value < range->second.first || value > range->second.second)) {
            throw std::runtime_error("Legacy value exceeds declared numeric type or exact double integer range");
        }
        if (type == "float" && std::isfinite(value) && std::abs(value) > std::numeric_limits<float>::max()) {
            throw std::runtime_error("Legacy float value out of range");
        }
        result.values.push_back(value);
    }
    return result;
}

struct Cells {
    Array connectivity{"connectivity"};
    Array offsets{"offsets"};
    size_t size = 0;
};

Cells readCells(std::istream& input, bool binary)
{
    Cells result;
    const size_t cells = count(input);
    const size_t entries = count(input);
    if (!binary) { input >> std::ws; }
    else {
        std::string line;
        std::getline(input, line);
    }
    if (input.peek() == 'O') {
        std::string keyword, type;
        input >> keyword >> type;
        if (keyword != "OFFSETS") { throw std::runtime_error("Expected OFFSETS"); }
        Array offsets = readArray(input, type, cells, 1, binary);
        input >> keyword >> type;
        if (keyword != "CONNECTIVITY" || cells == 0 || offsets.values.front() != 0) {
            throw std::runtime_error("Invalid legacy offsets/connectivity");
        }
        result.connectivity = readArray(input, type, entries, 1, binary);
        result.connectivity.name = "connectivity";
        result.offsets.values.assign(offsets.values.begin() + 1, offsets.values.end());
        result.size = cells - 1;
        return result;
    }
    // The old cell list contains each cell's node count followed by its indices.
    Array packed;
    if (binary) {
        packed.values.reserve(entries);
        for (size_t i = 0; i < entries; ++i) { packed.values.push_back(readBigEndian<int32_t>(input)); }
    } else { packed = readArray(input, "int", entries, 1, false); }
    size_t offset = 0;
    for (size_t cell = 0; cell < cells; ++cell) {
        if (offset >= packed.values.size()) { throw std::runtime_error("Truncated legacy cell list"); }
        const double length = packed.values[offset++];
        if (length < 1 || length != std::floor(length) || length > packed.values.size() - offset) {
            throw std::runtime_error("Invalid legacy cell size");
        }
        result.connectivity.values.insert(result.connectivity.values.end(), packed.values.begin() + offset,
                                          packed.values.begin() + offset + static_cast<size_t>(length));
        offset += static_cast<size_t>(length);
        result.offsets.values.push_back(static_cast<double>(result.connectivity.values.size()));
    }
    if (offset != entries) { throw std::runtime_error("Legacy cell list size mismatch"); }
    result.size = cells;
    return result;
}
}

namespace RWVtk {
VtkLegacyReader::VtkLegacyReader() = default;
VtkLegacyReader::~VtkLegacyReader() = default;

bool VtkLegacyReader::PerformStream(std::istream& input, const TCollection_AsciiString& fileName)
{
    if (!VtkBaseReader::PerformStream(input, fileName)) { return false; }
    myMeshData.Nullify();
    try {
        std::string line, keyword, dataset;
        std::getline(input, line);
        if (line.find("# vtk DataFile Version") != 0) { return fail("Invalid legacy VTK header"); }
        std::getline(input, line);
        std::getline(input, line);
        const bool binary = line.find("BINARY") == 0;
        if (!binary && line.find("ASCII") != 0) { return fail("Invalid legacy VTK encoding"); }
        input >> keyword >> dataset;
        if (keyword != "DATASET" || (dataset != "UNSTRUCTURED_GRID" && dataset != "POLYDATA")) {
            return fail("Unsupported legacy VTK dataset: " + dataset);
        }
        Array points, types{"types"};
        Cells cells;
        std::vector<std::pair<std::string, Cells>> polyCells;
        std::vector<Array> pointFields, cellFields, metadata;
        size_t tupleCount = 0;
        bool nodal = true;
        while (input >> keyword) {
            if (keyword == "POINTS") {
                const size_t n = count(input);
                std::string type;
                input >> type;
                points = readArray(input, type, n, 3, binary);
            } else if (keyword == "CELLS") { cells = readCells(input, binary); }
            else if (keyword == "CELL_TYPES") {
                types = readArray(input, "int", count(input), 1, binary);
                types.name = "types";
            } else if (keyword == "VERTICES" || keyword == "POLYGONS" || keyword == "TRIANGLE_STRIPS" || keyword == "LINES") {
                const std::string tag = keyword == "VERTICES" ? "Verts" : keyword == "POLYGONS" ? "Polys"
                                      : keyword == "TRIANGLE_STRIPS" ? "Strips" : "Lines";
                polyCells.emplace_back(tag, readCells(input, binary));
            } else if (keyword == "POINT_DATA" || keyword == "CELL_DATA") {
                nodal = keyword == "POINT_DATA";
                tupleCount = count(input);
            } else if (keyword == "SCALARS" || keyword == "VECTORS" || keyword == "TENSORS" || keyword == "NORMALS") {
                const std::string kind = keyword;
                std::string name, type;
                input >> name >> type;
                int components = kind == "TENSORS" ? 9 : kind == "SCALARS" ? 1 : 3;
                if (kind == "SCALARS") {
                    std::getline(input, line);
                    std::istringstream options(line);
                    options >> components;
                    input >> keyword >> line;
                    if (keyword != "LOOKUP_TABLE") { return fail("Missing legacy LOOKUP_TABLE"); }
                }
                Array field = readArray(input, type, tupleCount, components, binary);
                field.name = name;
                field.role = kind == "VECTORS" ? "Vectors" : kind == "NORMALS" ? "Normals" : kind == "TENSORS" ? "Tensors" : "";
                (nodal ? pointFields : cellFields).push_back(std::move(field));
            } else if (keyword == "FIELD") {
                std::string name, type;
                input >> name;
                const size_t arrays = count(input);
                for (size_t i = 0; i < arrays; ++i) {
                    input >> name;
                    const int components = static_cast<int>(count(input));
                    const size_t n = count(input);
                    input >> type;
                    Array field = readArray(input, type, n, components, binary);
                    field.name = name;
                    if (tupleCount && n == tupleCount) { (nodal ? pointFields : cellFields).push_back(std::move(field)); }
                    else { metadata.push_back(std::move(field)); }
                }
            } else if (keyword == "TEXTURE_COORDINATES") {
                std::string name, type;
                input >> name;
                const int components = static_cast<int>(count(input));
                input >> type;
                if (components < 1 || components > 3 || tupleCount == 0) {
                    return fail("Invalid legacy TEXTURE_COORDINATES shape/association");
                }
                Array field = readArray(input, type, tupleCount, components, binary);
                field.name = name;
                (nodal ? pointFields : cellFields).push_back(std::move(field));
            } else if (keyword == "COLOR_SCALARS") {
                std::string name;
                input >> name;
                const int components = static_cast<int>(count(input));
                if (components < 1 || components > 4 || tupleCount == 0) {
                    return fail("Invalid legacy COLOR_SCALARS shape/association");
                }
                Array field = readArray(input, binary ? "unsigned_char" : "float", tupleCount, components, binary);
                field.name = name;
                // VTK stores binary colors as bytes and ASCII colors as normalized floats.
                for (double& value : field.values) {
                    if (!std::isfinite(value) || value < 0 || value > (binary ? 255 : 1)) {
                        return fail("Legacy COLOR_SCALARS value out of range");
                    }
                }
                (nodal ? pointFields : cellFields).push_back(std::move(field));
            } else if (keyword == "LOOKUP_TABLE") {
                std::string name;
                input >> name;
                const size_t entries = count(input);
                const Array table = readArray(input, binary ? "unsigned_char" : "float", entries, 4, binary);
                for (double value : table.values) {
                    if (!std::isfinite(value) || value < 0 || value > (binary ? 255 : 1)) {
                        return fail("Legacy LOOKUP_TABLE value out of range");
                    }
                }
                warn("Legacy LOOKUP_TABLE '" + name + "' is validated but not applied; numeric scalar values are retained");
            } else { return fail("Unsupported legacy VTK section: " + keyword); }
        }
        if (points.values.empty()) { return fail("Legacy VTK has no points"); }
        std::ostringstream xml;
        const bool volume = dataset == "UNSTRUCTURED_GRID";
        const char* tag = volume ? "UnstructuredGrid" : "PolyData";
        xml << "<VTKFile type=\"" << tag << "\" byte_order=\"LittleEndian\"><" << tag << ">";
        if (!metadata.empty()) {
            xml << "<FieldData>";
            for (const Array& field : metadata) { emitArray(xml, field); }
            xml << "</FieldData>";
        }
        xml << "<Piece NumberOfPoints=\"" << points.values.size() / 3 << "\"";
        if (volume) { xml << " NumberOfCells=\"" << cells.size << "\""; }
        else { for (const auto& part : polyCells) { xml << " NumberOf" << part.first << "=\"" << part.second.size << "\""; } }
        xml << "><Points>";
        emitArray(xml, points);
        xml << "</Points>";
        if (volume) {
            if (types.values.size() != cells.size) { return fail("Legacy CELL_TYPES count mismatch"); }
            xml << "<Cells>";
            emitArray(xml, cells.connectivity, "Int32");
            emitArray(xml, cells.offsets, "Int32");
            emitArray(xml, types, "UInt8");
            xml << "</Cells>";
        } else {
            for (const auto& part : polyCells) {
                xml << '<' << part.first << '>';
                emitArray(xml, part.second.connectivity, "Int32");
                emitArray(xml, part.second.offsets, "Int32");
                xml << "</" << part.first << '>';
            }
        }
        emitFields(xml, "PointData", pointFields);
        emitFields(xml, "CellData", cellFields);
        xml << "</Piece></" << tag << "></VTKFile>";
        std::istringstream normalized(xml.str());
        // Reuse the XML reader's cell ordering and field association validation.
        if (volume) {
            VtkUnstructuredGridReader reader;
            reader.SetToReportMessages(myToReportMessages);
            reader.SetMappingInfo(myMappingInfo);
            const bool ok = reader.PerformStream(normalized, fileName);
            const auto warnings = myReport.warnings;
            myReport = reader.Report();
            myReport.warnings.insert(myReport.warnings.begin(), warnings.begin(), warnings.end());
            if (ok) { myMeshData = reader.GetMeshData(); }
            return ok;
        }
        VtkPolyDataReader reader;
        reader.SetToReportMessages(myToReportMessages);
        reader.SetMappingInfo(myMappingInfo);
        const bool ok = reader.PerformStream(normalized, fileName);
        const auto warnings = myReport.warnings;
        myReport = reader.Report();
        myReport.warnings.insert(myReport.warnings.begin(), warnings.begin(), warnings.end());
        if (ok) { myMeshData = reader.GetMeshData(); }
        return ok;
    } catch (const std::exception& error) { return fail(error.what()); }
}
}

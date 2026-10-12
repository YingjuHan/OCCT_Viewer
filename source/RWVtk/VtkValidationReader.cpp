#include <RWVtk/VtkValidationReader.h>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace RWVtk {
VtkValidationReader::VtkValidationReader(bool polyData) : myPolyData(polyData) {}
VtkValidationReader::~VtkValidationReader() {}
bool VtkValidationReader::perform()
{
    auto count = [&](const LDOM_Element& e, const char* name, bool optional = false) {
        const TCollection_AsciiString text = e.getAttribute(name);
        if (optional && text.IsEmpty()) { return 0; }
        size_t used = 0; const auto value = std::stoll(text.ToCString(), &used);
        if (used != (size_t)text.Length() || value < 0 || value > std::numeric_limits<int>::max()) {
            throw std::runtime_error(std::string("Invalid VTK count: ") + name);
        }
        return (int)value;
    };
    using Arrays = std::map<std::string, DataArray>;
    auto arrays = [&](const LDOM_Element& e) {
        Arrays result;
        for (const auto& child : getChildrenElementsByTagName(e, "DataArray")) {
            const auto a = readDataArray(child);
            if (a.Data.IsNull()) { throw std::runtime_error("Invalid VTK array"); }
            if (!result.emplace(a.Name.ToCString(), a).second) { throw std::runtime_error("Duplicate VTK array name"); }
        }
        return result;
    };
    auto required = [](const Arrays& list, const char* name) -> const DataArray& {
        const auto found = list.find(name);
        if (found == list.end() || found->second.Components.size() != 1
            || (found->second.Type != "Int32" && found->second.Type != "Int64" && found->second.Type != "UInt8")) {
            throw std::runtime_error(std::string("Missing or invalid integer topology array: ") + name);
        }
        return found->second;
    };
    auto offsets = [&](const DataArray& a, int entries, int minSize, int limit, bool empty = false) {
        if (a.Data->NbElements() != entries) { throw std::runtime_error("Topology offset count mismatch"); }
        int begin = 0;
        for (int i = 0; i < entries; ++i) {
            const int end = a.Data->GetInt(i, 0);
            if (empty && end == -1) { continue; }
            if (end < begin || end - begin < minSize || end > limit) { throw std::runtime_error("Invalid topology offset"); }
            begin = end;
        }
        if (begin != limit) { throw std::runtime_error("Unused topology entries"); }
    };
    const auto pieces = getChildrenElementsByTagName(myDataElement, "Piece");
    if (pieces.empty()) { return fail("VTK dataset has no Piece"); }
    for (const auto& piece : pieces) {
        const int nodes = count(piece, "NumberOfPoints");
        int cells = myPolyData ? 0 : count(piece, "NumberOfCells");
        if (myPolyData) {
            uint64_t total = 0;
            for (const auto* name : {"NumberOfVerts", "NumberOfLines", "NumberOfPolys", "NumberOfStrips"}) { total += count(piece, name, true); }
            if (total > std::numeric_limits<int>::max()) { return fail("PolyData cell count overflow"); }
            cells = (int)total;
        }
        for (LDOM_Node node = piece.getFirstChild(); !node.isNull(); node = node.getNextSibling()) {
            if (node.getNodeType() != LDOM_Node::ELEMENT_NODE) { continue; }
            const std::string tag = TCollection_AsciiString(node.getNodeName()).ToCString();
            const std::set<std::string> allowed = {"Points", "Cells", "PointData", "CellData", "Verts", "Lines", "Polys", "Strips"};
            if (!allowed.count(tag)) { return fail("Unsupported VTK Piece section: " + tag); }
        }
        const auto points = arrays(getFirstElement(piece, "Points"));
        if (points.size() != 1 || points.begin()->second.Components.size() != 3
            || points.begin()->second.Data->NbElements() != nodes) { return fail("Invalid VTK point array shape"); }
        for (int i = 0; i < nodes; ++i) { for (int c = 0; c < 3; ++c) {
            const double value = points.begin()->second.Data->GetDouble(i, c);
            if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max()) { return fail("Invalid VTK coordinate"); }
        } }
        for (const char* association : {"PointData", "CellData"}) {
            const auto element = getFirstElement(piece, association);
            const auto fields = arrays(element);
            for (const auto& field : fields) {
                if (field.first.empty() || field.second.Data->NbElements() != (std::string(association) == "PointData" ? nodes : cells)) { return fail("Invalid VTK field name/tuple count"); }
            }
            for (const char* role : {"Vectors", "Normals", "Tensors"}) {
                const TCollection_AsciiString name = element.isNull() ? TCollection_AsciiString() : TCollection_AsciiString(element.getAttribute(role));
                if (!name.IsEmpty()) {
                    auto found = fields.find(name.ToCString());
                    if (found == fields.end() || found->second.Components.size() != (std::string(role) == "Tensors" ? 9u : 3u)) { return fail("Invalid VTK field role"); }
                }
            }
        }
        const std::vector<std::string> sections = myPolyData ? std::vector<std::string>{"Verts", "Lines", "Polys", "Strips"} : std::vector<std::string>{"Cells"};
        for (const auto& section : sections) {
            const int size = myPolyData ? count(piece, ("NumberOf" + section).c_str(), true) : cells;
            if (myPolyData && size == 0) { continue; }
            const auto list = arrays(getFirstElement(piece, section.c_str()));
            const auto& conn = required(list, "connectivity"); const auto& off = required(list, "offsets");
            offsets(off, size, myPolyData ? section == "Verts" ? 1 : section == "Lines" ? 2 : 3 : 0, conn.Data->NbElements());
            for (int i = 0; i < conn.Data->NbElements(); ++i) {
                const int id = conn.Data->GetInt(i, 0); if (id < 0 || id >= nodes) { return fail("Connectivity point index out of range"); }
            }
            if (myPolyData) { continue; }
            const auto& types = required(list, "types");
            if (types.Data->NbElements() != cells) { return fail("Cell type count mismatch"); }
            if (types.Type != "UInt8") { return fail("VTK types must be UInt8"); }
            int cellStart = 0, faceStart = 0;
            for (int e = 0; e < cells; ++e) {
                const int end = off.Data->GetInt(e,0), type = types.Data->GetInt(e,0);
                if ((type == 0 && end != cellStart) || (type != 0 && end == cellStart)) { return fail("Invalid empty/nonempty cell connectivity"); }
                if (type == 42 && list.count("faces")) {
                    const auto& f = required(list,"faces"); const auto& o = required(list,"faceoffsets");
                    if (o.Data->NbElements() != cells) { return fail("Polyhedron face offset count mismatch"); }
                    const int last = o.Data->GetInt(e,0);
                    if (last <= faceStart || last > f.Data->NbElements()) { return fail("Invalid polyhedron face offset"); }
                    std::set<int> members;
                    for (int i = cellStart; i < end; ++i) { members.insert(conn.Data->GetInt(i,0)); }
                    int at = faceStart; const int faces = f.Data->GetInt(at++,0);
                    if (faces < 4 || faces > last-at) { return fail("Invalid polyhedron face count"); }
                    for (int i = 0; i < faces; ++i) {
                        if (at >= last) { return fail("Truncated polyhedron faces"); }
                        const int size = f.Data->GetInt(at++,0);
                        if (size < 3 || size > last-at) { return fail("Invalid polyhedron face length"); }
                        for (int j = 0; j < size; ++j) { if (!members.count(f.Data->GetInt(at++,0))) { return fail("Invalid polyhedron face point"); } }
                    }
                    if (at != last) { return fail("Extra polyhedron face entries"); } faceStart=last;
                } else if (type == 42 && !list.count("face_connectivity")) { return fail("Missing polyhedron face arrays"); }
                cellStart = end;
            }
            if (list.count("face_connectivity") || list.count("face_offsets") || list.count("polyhedron_to_faces") || list.count("polyhedron_offsets")) {
                const auto& fc = required(list,"face_connectivity"); const auto& fo = required(list,"face_offsets");
                const auto& pf = required(list,"polyhedron_to_faces"); const auto& po = required(list,"polyhedron_offsets");
                offsets(fo, fo.Data->NbElements(), 3, fc.Data->NbElements());
                offsets(po, cells, 0, pf.Data->NbElements(), true);
                int begin = 0, cellBegin = 0;
                for (int e = 0; e < cells; ++e) {
                    const int end = po.Data->GetInt(e,0), cellEnd = off.Data->GetInt(e,0);
                    std::set<int> members;
                    for (int i = cellBegin; i < cellEnd; ++i) { members.insert(conn.Data->GetInt(i,0)); }
                    if (types.Data->GetInt(e,0) == 42) {
                        if (end-begin < 4) { return fail("Polyhedron requires at least four faces"); }
                        for (int i = begin; i < end; ++i) {
                            const int face = pf.Data->GetInt(i,0);
                            if (face < 0 || face >= fo.Data->NbElements()) { return fail("Polyhedron face index out of range"); }
                            const int first = face == 0 ? 0 : fo.Data->GetInt(face-1,0);
                            for (int j = first; j < fo.Data->GetInt(face,0); ++j) {
                                if (!members.count(fc.Data->GetInt(j,0))) { return fail("Polyhedron face point is not a cell member"); }
                            }
                        }
                    } else if (end != -1 && end != begin) { return fail("Non-polyhedron contains face references"); }
                    if (end != -1) { begin=end; } cellBegin=cellEnd;
                }
            }
        }
        myReport.points += nodes; myReport.cells += cells;
    }
    const auto metadata = arrays(getFirstElement(myDataElement,"FieldData"));
    auto time = metadata.find("TimeValue");
    if (time != metadata.end() && (time->second.Components.size()!=1 || time->second.Data->NbElements()!=1
        || !std::isfinite(time->second.Data->GetDouble(0,0)))) { return fail("Invalid TimeValue"); }
    return true;
}
}

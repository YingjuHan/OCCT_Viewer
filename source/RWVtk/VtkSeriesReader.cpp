#include <RWVtk/VtkSeriesReader.h>
#include <RWCae/MeshTools.h>
#include <OSD_Path.hxx>
#include <RWGltf_GltfJsonParser.hxx>
#include <cmath>
#include <map>
#include <limits>
#include <sstream>

namespace {
using Mesh = Handle(RWCae::MeshData);
using Property = Handle(RWCae::MeshNVecProperty);
std::string key(const Property& p)
{
    return std::string(p->Name().ToCString()) + ":" + p->Description().ToCString() + ":" + std::to_string((int)p->MapType());
}
template<class T> bool equal(const T& a, const T& b)
{
    if (a.Size() != b.Size()) { return false; }
    for (int i = 0; i < a.Size(); ++i) { if (a[i] != b[i]) { return false; } }
    return true;
}
bool sameGeometry(const Mesh& a, const Mesh& b)
{
    if (a->NbNodes() != b->NbNodes() || a->NbIndexedElems() != b->NbIndexedElems()) { return false; }
    for (int n = 0; n < a->NbNodes(); ++n) { if (a->Nodes()[n] != b->Nodes()[n]) { return false; } }
    for (int e = 0; e < a->NbIndexedElems(); ++e) {
        const auto& x = a->IndexedElements()[e]; const auto& y = b->IndexedElements()[e];
        if (x.Type != y.Type || x.Index != y.Index) { return false; }
    }
    for (int t = 0; t < RWCae::MeshElemNbTypes; ++t) {
        const auto& x = a->Elements((RWCae::MeshElemType)t); const auto& y = b->Elements((RWCae::MeshElemType)t);
        if (!equal(x.Nodes(), y.Nodes()) || !equal(x.NodesOffsets(), y.NodesOffsets())
            || !equal(x.Faces(), y.Faces()) || !equal(x.FacesOffsets(), y.FacesOffsets())) { return false; }
    }
    if (a->GenericCells().size() != b->GenericCells().size()) { return false; }
    for (size_t i = 0; i < a->GenericCells().size(); ++i) {
        const auto& x = a->GenericCells()[i]; const auto& y = b->GenericCells()[i];
        if (x.Type != y.Type || x.Nodes != y.Nodes || x.Faces != y.Faces
            || x.Degrees != y.Degrees || x.RationalWeights != y.RationalWeights) { return false; }
    }
    return true;
}
Mesh clone(const Mesh& mesh) { return RWCae::MeshTools::MergeMeshData({{mesh, {}}}); }
}

namespace RWVtk {
VtkSeriesReader::VtkSeriesReader() : VtkBaseReader() {}
VtkSeriesReader::~VtkSeriesReader() {}
bool VtkSeriesReader::PerformStream(std::istream& stream, const TCollection_AsciiString& filename)
{
    myAccumMeshDataTreeRoot.Nullify(); myMergedMeshData.Nullify();
    if (!VtkBaseReader::PerformStream(stream, filename)) { return false; }
    TCollection_AsciiString leaf;
    OSD_Path::FolderAndFileFromPath(myFileName, myPath, leaf);
    myReader.reset(new rapidjson::Document());
    std::string document;
    try { document=myBudget->Read(stream); } catch (const std::exception& error) { return fail(error.what()); }
    std::istringstream bounded(document);
    rapidjson::IStreamWrapper wrapper(bounded);
    rapidjson::ParseResult result = myReader->ParseStream<rapidjson::kParseFullPrecisionFlag>(wrapper);
    if (result.IsError()) {
        return fail(std::string("Invalid VTK series JSON: ") + RWGltf_GltfJsonParser::FormatParseError(result.Code())
            + " at offset " + std::to_string(result.Offset()));
    }
    if (!myReader->IsObject() || !myReader->HasMember("files")) { return fail("VTK series has no files array"); }
    try { return readFiles(&(*myReader)["files"]); } catch (const std::exception& error) { return fail(error.what()); }
}
bool VtkSeriesReader::readFiles(const JsonValue* files)
{
    if (!files || !files->IsArray() || files->Empty()) { return fail("VTK series files must be a nonempty array"); }
    std::vector<RWCae::MeshData::SeriesStep> staged;
    std::map<std::string, Property> schema;
    double previous = -std::numeric_limits<double>::infinity();
    bool stable = true;
    for (const auto& entry : files->GetArray()) {
        const int step = (int)staged.size();
        if (!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString()
            || entry["name"].GetStringLength() == 0 || !entry.HasMember("time") || !entry["time"].IsNumber()
            || !std::isfinite(entry["time"].GetDouble()) || entry["time"].GetDouble() <= previous) {
            return fail("VTK series requires filenames and finite strictly increasing times at step " + std::to_string(step));
        }
        const double time = entry["time"].GetDouble(); previous = time;
        const TCollection_AsciiString source = myPath + entry["name"].GetString();
        VtkMultiBlockDataReader reader;
        reader.SetResourceBudget(myBudget);
        reader.SetToReportMessages(false); reader.SetMappingInfo(myMappingInfo);
        reader.SetTimeStep(step); reader.SetTimeStamp(time);
        if (!reader.Perform(source)) {
            for (const auto& error : reader.Report().errors) {
                myReport.errors.push_back("Series step " + std::to_string(step) + " '" + source.ToCString() + "': " + error);
            }
            return fail("Cannot stage VTK series step " + std::to_string(step));
        }
        for (const auto& warning : reader.Report().warnings) {
            warn("Series step " + std::to_string(step) + " '" + source.ToCString() + "': " + warning);
        }
        const Mesh mesh = reader.GetMergedMeshData();
        if (mesh.IsNull() || (mesh->NbNodes() == 0 && mesh->NbIndexedElems() == 0)) { return fail("VTK series contains no mesh"); }
        std::map<std::string, Property> fields;
        for (const Property& p : mesh->Properties()) {
            if (!fields.emplace(key(p), p).second) { return fail("Ambiguous series field '" + key(p) + "'"); }
            if (p->NbValues() != (p->IsNodal() ? mesh->NbNodes() : mesh->NbIndexedElems())) {
                return fail("Series field tuple count mismatch: " + key(p));
            }
            p->SetTime(time, step);
            if (step == 0) { schema.emplace(key(p), p); }
            else {
                auto found = schema.find(key(p));
                if (found == schema.end() || found->second->Type() != p->Type()
                    || found->second->NbComponents() != p->NbComponents()) {
                    return fail("Series field schema mismatch at step " + std::to_string(step) + ": " + key(p));
                }
                for (int c = 0; c < p->NbComponents(); ++c) {
                    if (found->second->GetComponent(c)->Name() != p->GetComponent(c)->Name()) {
                        return fail("Series field component name mismatch: " + key(p));
                    }
                }
                p->SetId(found->second->Id());
            }
        }
        if (fields.size() != schema.size()) { return fail("Series fields missing at step " + std::to_string(step)); }
        if (step > 0) { stable = stable && sameGeometry(staged.front().Mesh, mesh); }
        staged.push_back({time, source, mesh});
    }
    Mesh published = clone(staged.front().Mesh);
    if (stable) {
        for (size_t step = 1; step < staged.size(); ++step) {
            Mesh copy = clone(staged[step].Mesh);
            for (const auto& field : copy->Properties()) { published->ChangeProperties().Append(field); }
        }
        RWCae::PropertyRanges ranges;
        RWCae::MeshTools::fillPropertiesRange(published->ChangeProperties(), ranges);
        RWCae::MeshTools::restorePropertiesRange(published->Properties(), ranges);
    } else {
        warn("Series geometry/topology changes: the initial Viewer mesh exposes only first-step fields; use SeriesSteps for later geometry");
    }
    published->ChangeSeriesSteps() = std::move(staged);
    myReport.points = published->NbNodes(); myReport.cells = published->NbIndexedElems();
    myReport.fields = published->Properties().Size(); myMergedMeshData = published;
    return true;
}
void VtkSeriesReader::mergeMeshData() {}
}

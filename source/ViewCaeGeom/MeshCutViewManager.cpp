#include <ViewCaeGeom/MeshCutViewManager.h>
#include <ViewCaeGeom/MeshCutUnion.h>
#include <ViewCaeGeom/MeshCutTrace.h>
#include "MeshCutFieldTools.h"

#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshTools.h>
#include <ViewCaeGeom/MeshGeometryDeformed.h>
#include <ViewCaeGeom/MeshGeometryTools.h>
#include <ViewCaeGeom/MeshGeometryQuadric.h>
#include <ViewCaeGeom/MeshGeometryVtkCut.h>
#include <ViewCaeGeom/MeshGeometryVtkIso.h>
#include <ViewCaeGeom/MeshVtkCutSource.h>
#include <ViewCaeGeom/MeshVtkCell.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <ViewCaeGeom/MeshCellKernel.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <set>
#include <type_traits>
#include <chrono>
#include <OSD_ThreadPool.hxx>
#include <iterator>

namespace {
bool normalizePlane(const Graphic3d_Vec4d& thePlane, Graphic3d_Vec4d& theResult)
{
    for (int i = 0; i < 4; ++i) {
        if (!std::isfinite(thePlane[i])) { return false; }
    }
    const double aLength = std::hypot(std::hypot(thePlane.x(), thePlane.y()), thePlane.z());
    if (!std::isfinite(aLength) || aLength <= std::numeric_limits<double>::min()) { return false; }
    if (std::abs(aLength - 1.0) <= 8 * std::numeric_limits<double>::epsilon()) {
        theResult = thePlane;
        return true;
    }
    theResult.SetValues(thePlane.x() / aLength, thePlane.y() / aLength,
                        thePlane.z() / aLength, thePlane.w() / aLength);
    return std::isfinite(theResult.w());
}

// Managed scalar geometry shares the existing cutter, provenance and section-line API.
// Its source chain contains only the base mesh and optional deformation.
class ScalarCutGeometry : public ViewCaeGeom::MeshGeometrySlice {
public:
    ScalarCutGeometry(const Handle(ViewCaeGeom::MeshBvhData)& bvh,
        const Handle(RWCae::MeshScalarPropertyBase)& values, double threshold, int direction)
        : MeshGeometrySlice(bvh), myValues(values), myThreshold(threshold), myDirection(direction) {}

    bool IsModifier() const override { return myDirection != 0; }
    ViewCaeGeom::MeshGeometryType Type() const override
    {
        return IsModifier() ? ViewCaeGeom::MeshGeometryType::Clip : ViewCaeGeom::MeshGeometryType::Iso;
    }
    void ToJson(RWBase::JsonHelper::Writer&) override
    {
        throw std::logic_error("Scalar cut geometry must be saved through MeshCutViewManager");
    }
    bool FromJson(const RWBase::JsonHelper::Value&, const Handle(RWCae::MeshData)&, int, bool&) override
    {
        return false;
    }

protected:
    double value(const ViewCaeGeom::MeshGeometry* geometry, RWCae::MeshEntityId node) const
    {
        ViewCaeGeom::MeshInterpolatedNode origin;
        if (!geometry->OriginNode(node, origin)) { return std::numeric_limits<double>::quiet_NaN(); }
        double result = 0.0;
        for (const auto& base : origin.BaseNodes()) {
            result += base.second * myValues->Value(RWCae::MeshPropertyEntityInfo(base.first));
        }
        return (result - myThreshold) * (myDirection == 0 ? 1 : myDirection);
    }
    bool acceptElement(const Handle(RWCae::MeshData)& mesh, RWCae::MeshEntityId index,
        const ViewCaeGeom::MeshGeometry* geometry) const override
    {
        if (myGenerating || !IsModifier()) { return true; }
        const auto replaced = myReplaced.find(geometry);
        if (replaced != myReplaced.end() && replaced->second.count(index)) { return false; }
        const auto& element = mesh->IndexedElements()[index];
        const auto& elements = mesh->Elements(element.Type);
        RWCae::MeshElemEntityOffset begin, end;
        elements.GetElemNodesRange(element.Index, &begin, &end);
        for (auto i = begin; i < end; ++i) {
            const double v = value(geometry, elements.Nodes()[(int)i]);
            if (!std::isfinite(v) || v < -Tolerance) { return false; }
        }
        return true;
    }
    bool acceptNode(RWCae::MeshEntityId node, const ViewCaeGeom::MeshGeometry* geometry,
        const Graphic3d_Vec3&) const override
    {
        if (myGenerating || !IsModifier()) { return true; }
        const double v = value(geometry, node);
        return std::isfinite(v) && v >= -Tolerance;
    }
    void generateMeshData() override
    {
        myMeshData = new RWCae::MeshData();
        myNodesMap.Clear(); myElementsMap.Clear(); myLines.clear(); myReplaced.clear();
        myGenerating = true;
        try {
            for (ViewCaeGeom::MeshGeometry* parent : ViewCaeGeom::MeshGeomParents(myParentGeometry.get())) {
                const auto& mesh = parent->GetMeshData();
                if (!mesh.IsNull()) {
                    ViewCaeGeom::MeshGeometryTools::TypedMapOfContours contours;
                    ViewCaeGeom::MeshGeometryTools::AddedNodeMap added;
                    auto process = [&](const auto& adaptor) {
                        using Element = typename std::decay<decltype(adaptor.GetElement(0))>::type;
                        for (RWCae::MeshEntityId i = 0; i < adaptor.NbElements(); ++i) {
                            const auto& element = adaptor.GetElement(i);
                            if (!parent->AcceptElement(element.GlobalIndex())) { continue; }
                            bool positive = false, negative = false, zero = false, invalid = false;
                            auto nodeValue = [&](RWCae::MeshElemEntityId n) { return value(parent, element[n]); };
                            for (RWCae::MeshElemEntityId n = 0; n < element.NbNodes(); ++n) {
                                const double v = nodeValue(n);
                                positive |= v > Tolerance; negative |= v < -Tolerance;
                                zero |= std::abs(v) <= Tolerance; invalid |= !std::isfinite(v);
                            }
                            if (invalid || (IsModifier() && (!positive || (!negative && !zero)))) { continue; }
                            ViewCaeGeom::MeshGeometryTools::MeshElementCutter<Element> cutter(
                                adaptor, element, nodeValue, &contours, 0.0, Tolerance);
                            if (!cutter.Perform()) { continue; }
                            if (IsModifier()) {
                                ViewCaeGeom::MeshGeometryTools::NodesAdjElems adjacency;
                                std::map<RWCae::MeshEntityId, int> neighbours;
                                if (adaptor.NbFaces(element) != 1) {
                                    for (RWCae::MeshElemEntityId n = 0; n < element.NbNodes(); ++n) {
                                        RWCae::MeshElemEntityId count = 0;
                                        const auto ids = mesh->NodeAdjElems(element[n], count);
                                        for (RWCae::MeshElemEntityId a = 0; a < count; ++a) {
                                            if (ids[a] != (RWCae::MeshEntityId)element.GlobalIndex() && !neighbours.count(ids[a])) {
                                                neighbours.emplace(ids[a], (int)neighbours.size());
                                            }
                                        }
                                    }
                                    adjacency.second.resize(neighbours.size());
                                    for (const auto& neighbour : neighbours) { adjacency.second[neighbour.second] = neighbour.first; }
                                    adjacency.first.resize(element.NbNodes(), ViewCaeGeom::BitMaskSet<1>((int)neighbours.size()));
                                    for (RWCae::MeshElemEntityId n = 0; n < element.NbNodes(); ++n) {
                                        RWCae::MeshElemEntityId count = 0;
                                        const auto ids = mesh->NodeAdjElems(element[n], count);
                                        for (RWCae::MeshElemEntityId a = 0; a < count; ++a) {
                                            if (ids[a] != (RWCae::MeshEntityId)element.GlobalIndex()) { adjacency.first[n].Set(neighbours.at(ids[a]), 0); }
                                        }
                                    }
                                }
                                if (cutter.AddCutElem(parent, myMeshData, adjacency, &myNodesMap, &myElementsMap, &added, i)) {
                                    myReplaced[parent].insert(element.GlobalIndex());
                                }
                            } else {
                                cutter.AddCutSurf(parent, myMeshData, &myNodesMap, &myElementsMap, &added, i);
                                if ((int)Element::Type >= RWCae::MeshElem3dLowerType) { continue; }
                                RWCae::MeshEntityId source = 0;
                                if (!parent->OriginElement(ViewCaeGeom::MeshGeomElem(i, Element::Type, false), source)) { continue; }
                                for (const auto& contour : cutter.CutContourNodes()) {
                                    if (contour.size() != 2) { continue; }
                                    std::array<RWCae::MeshEntityId, 2> nodes;
                                    for (size_t n = 0; n < nodes.size(); ++n) {
                                        if (!added.Find(contour[n], nodes[n])) {
                                            Graphic3d_Vec3 position(0.0f);
                                            for (const auto& base : contour[n].BaseNodes()) {
                                                auto point = mesh->Nodes()[base.first];
                                                parent->ModifyNode(base.first, point);
                                                position += point * (float)base.second;
                                            }
                                            nodes[n] = myMeshData->AddNodePosition(position);
                                            added.Bind(contour[n], nodes[n]);
                                            myNodesMap.Bind(nodes[n], std::make_pair(parent, contour[n]));
                                        }
                                    }
                                    if (nodes[0] != nodes[1]) { myLines.push_back({nodes, source}); }
                                }
                            }
                        }
                    };
                    RWCae::MeshAdaptorsIterator2dElems::For(process, mesh);
                    RWCae::MeshAdaptorsIterator3dElems::For(process, mesh);
                }
                if (!parent->IsModifier()) { break; }
            }
        } catch (...) { myGenerating = false; throw; }
        myGenerating = false;
    }

private:
    static constexpr double Tolerance = 1e-12;
    Handle(RWCae::MeshScalarPropertyBase) myValues;
    double myThreshold;
    int myDirection;
    bool myGenerating = false;
    std::map<const ViewCaeGeom::MeshGeometry*, std::set<RWCae::MeshEntityId>> myReplaced;
};
}

namespace ViewCaeGeom {
struct MeshCutViewManager::Entry {
    MeshCutDefinition Definition;
    uint64_t Revision = 1;
    std::array<Handle(MeshGeometry), 3> Roots;
    std::array<Handle(MeshGeometry), 3> Portions;
    std::vector<double> NodeProjections;
    Handle(RWCae::MeshScalarPropertyBase) IsoValues;
    unsigned CachedMask = 0;
    Handle(MeshGeometry) TraceRoot;
    Handle(MeshGeometryTraceSection) TraceSection;
    uint64_t TraceRevision = 0;
    std::shared_ptr<void> Lease;
    uint64_t Access=0;
    uint64_t Reserved=0;
};

MeshCutViewManager::MeshCutViewManager(const Handle(RWCae::MeshData)& theMesh)
    : myMesh(theMesh), myBvh(new MeshBvhData())
{
    if (myMesh.IsNull()) { throw std::invalid_argument("Cut manager requires a mesh"); }
}

MeshCutViewManager::~MeshCutViewManager() { Clear(); }

MeshCutViewManager::Entry* MeshCutViewManager::find(const std::string& theId)
{
    const auto anEntry = myEntries.find(theId);
    return anEntry == myEntries.end() ? nullptr : anEntry->second.get();
}

bool MeshCutViewManager::AddPlane(const std::string& theId, const Graphic3d_Vec4d& thePlane)
{
    Graphic3d_Vec4d aPlane;
    if (theId.empty() || theId.find('\0') != std::string::npos || find(theId) != nullptr
        || !normalizePlane(thePlane, aPlane)) { return false; }
    auto anEntry = std::make_unique<Entry>();
    anEntry->Definition.Id = theId;
    anEntry->Definition.Name = theId;
    anEntry->Definition.Plane = aPlane;
    if (!anEntry->Definition.Placement.FromEquation(aPlane)) { return false; }
    myEntries.emplace(theId, std::move(anEntry));
    return true;
}

bool MeshCutViewManager::SetPlane(const std::string& theId, const Graphic3d_Vec4d& thePlane)
{
    Entry* anEntry = find(theId);
    Graphic3d_Vec4d aPlane;
    if (anEntry == nullptr || anEntry->Definition.Shape != MeshCutShape::Plane
        || !normalizePlane(thePlane, aPlane)) { return false; }
    if (anEntry->Definition.Plane == aPlane) { return true; }
    auto placement = anEntry->Definition.Placement;
    const Graphic3d_Vec3d normal(aPlane.x(),aPlane.y(),aPlane.z());
    if ((normal-placement.CurrentNormal()).Modulus() < 1e-12) {
        placement.Offset = -aPlane.w()-normal.Dot(placement.Origin);
    } else {
        // A legacy world-equation edit explicitly redefines orientation. Preserve
        // the previous pivot's tangential position and transport its in-plane axis.
        const auto origin = placement.CurrentOrigin();
        auto tangent = placement.CurrentAxis2(); tangent -= normal*normal.Dot(tangent);
        if (tangent.Modulus() < 1e-10) {
            if (!placement.FromEquation(aPlane)) { return false; }
            tangent = placement.Axis2;
        }
        if (!placement.Define(origin-normal*(normal.Dot(origin)+aPlane.w()),normal,tangent.Normalized())) { return false; }
    }
    return SetPlanePlacement(theId,placement);
}

bool MeshCutViewManager::SetPlanePlacement(const std::string& id, const MeshCutPlanePlacement& placement)
{
    auto entry = find(id); Graphic3d_Vec4d plane;
    if (!entry || entry->Definition.Shape != MeshCutShape::Plane || !placement.IsValid()
        || !normalizePlane(placement.Equation(),plane)) { return false; }
    if (!setPlaneEquation(*entry,plane)) { return false; }
    entry->Definition.Placement = placement;
    return true;
}
bool MeshCutViewManager::TranslatePlane(const std::string& id, double offset)
{
    auto entry = find(id); if (!entry) { return false; }
    auto p = entry->Definition.Placement; p.Offset = offset; return SetPlanePlacement(id,p);
}
bool MeshCutViewManager::RotatePlane(const std::string& id, int axis, double degrees)
{
    auto entry = find(id); if (!entry) { return false; }
    auto p = entry->Definition.Placement; p.RotationAxis = axis; p.Angle = degrees; p.Offset = 0;
    return SetPlanePlacement(id,p);
}
bool MeshCutViewManager::setPlaneEquation(Entry& entry, const Graphic3d_Vec4d& aPlane)
{
    auto anEntry = &entry;
    if (anEntry->Definition.Plane == aPlane) { return true; }
    if (needsGeneralPlane()) {
        release(*anEntry); anEntry->Definition.Plane = aPlane; ++anEntry->Revision;
        return true;
    }
    const bool reused = moveCachedPlane(*anEntry, aPlane);
    if (anEntry->Definition.Plane.x() != aPlane.x() || anEntry->Definition.Plane.y() != aPlane.y()
        || anEntry->Definition.Plane.z() != aPlane.z()) { anEntry->NodeProjections.clear(); }
    anEntry->Definition.Plane = aPlane;
    ++anEntry->Revision;
    if (!reused && !anEntry->Portions[0].IsNull()) {
        anEntry->CachedMask = 0;
        Handle(MeshGeometryClip)::DownCast(anEntry->Portions[0])->SetClipPlane(new Graphic3d_ClipPlane(-aPlane));
        Handle(MeshGeometryClip)::DownCast(anEntry->Portions[2])->SetClipPlane(new Graphic3d_ClipPlane(aPlane));
        Graphic3d_SequenceOfHClipPlane aPlanes;
        aPlanes.Append(new Graphic3d_ClipPlane(aPlane));
        Handle(MeshGeometrySlice)::DownCast(anEntry->Portions[1])->SetSlicePlanes(aPlanes);
    }
    return true;
}

MeshCutTraceCacheStatistics MeshCutViewManager::TraceCacheStatistics() const
{
    return myTraceSource?myTraceSource->CacheStatistics():MeshCutTraceCacheStatistics();
}

bool MeshCutViewManager::EnsureDefaultPlanes()
{
    if (!myEntries.empty()) { return true; }
    if (myMesh->NbNodes() == 0) { return false; }
    std::array<double, 3> minimum = {INFINITY, INFINITY, INFINITY};
    std::array<double, 3> maximum = {-INFINITY, -INFINITY, -INFINITY};
    for (const auto& node : myMesh->Nodes()) {
        for (int axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(node[axis])) { return false; }
            minimum[axis] = std::min(minimum[axis], (double)node[axis]);
            maximum[axis] = std::max(maximum[axis], (double)node[axis]);
        }
    }
    const char* ids[] = {"x", "y", "z"};
    const char* names[] = {"X-plane", "Y-plane", "Z-plane"};
    for (int axis = 0; axis < 3; ++axis) {
        Graphic3d_Vec4d plane(0,0,0,-(minimum[axis]+maximum[axis])*0.5);
        plane[axis] = 1;
        AddPlane(ids[axis], plane);
        Rename(ids[axis], names[axis]);
        SetEnabled(ids[axis], false);
    }
    return true;
}

bool MeshCutViewManager::validIsoField(const Handle(RWCae::MeshNVecProperty)& field, int component) const
{
    // These cells have world-coordinate interpolation or disconnected strips,
    // rather than the single FE parameter chart used by this numerical backend.
    for (const auto& cell : myMesh->GenericCells()) {
        if (cell.Type < 3 || cell.Type == 4 || cell.Type == 6 || cell.Type == 7
            || cell.Type == 36 || cell.Type == 41 || cell.Type == 42
            || !MeshCellKernel::InstantiateCell(cell.Type)) { return false; }
    }
    if (needsGeneralIso() && (myMesh->NbElems(RWCae::MeshElemType::Polyhedron)
        || myMesh->NbElems(RWCae::MeshElemType::Polygon) || myMesh->NbElems(RWCae::MeshElemType::Elem0d))) { return false; }
    if (field.IsNull() || !field->IsInitialized() || !field->HasScalarComponents()
        || (!field->IsNodal() && !field->IsElemental()) || field->Name().IsEmpty()
        || component < 0 || component >= field->NbRawComponents()) { return false; }
    const auto values = field->GetRawComponent(component);
    const size_t count = field->IsNodal() ? myMesh->NbNodes() : myMesh->NbIndexedElems();
    if (count == 0 || count > (size_t)std::numeric_limits<int>::max()
        || values.IsNull() || (size_t)values->NbValues() != count) { return false; }
    for (size_t n = 0; n < count; ++n) {
        if (!std::isfinite(values->Value(RWCae::MeshPropertyEntityInfo((int)n)))) { return false; }
    }
    return true;
}

Handle(RWCae::MeshNVecProperty) MeshCutViewManager::resolveIsoField(const MeshCutDefinition& definition) const
{
    Handle(RWCae::MeshNVecProperty) result;
    for (const auto& field : myMesh->Properties()) {
        if (field->Name() == definition.IsoFieldName.c_str() && field->IsNodal() == definition.IsoFieldNodal
            && field->TimeStep() == definition.IsoTimeStep) {
            if (!result.IsNull() && result != field) { return nullptr; }
            result = field;
        }
    }
    return result;
}

Handle(RWCae::MeshScalarPropertyBase) MeshCutViewManager::isoValues(Entry& entry)
{
    if (!entry.IsoValues.IsNull()) { return entry.IsoValues; }
    const auto field = resolveIsoField(entry.Definition);
    if (!validIsoField(field, entry.Definition.IsoComponent)) {
        throw std::invalid_argument("Isosurface requires a unique finite scalar on a supported mesh (high-order charts require VTK)");
    }
    entry.IsoValues = field->GetRawComponent(entry.Definition.IsoComponent);
    if (field->IsElemental()) {
        // The cut binding uses a fixed equal-weight projection of the selected
        // scalar. Display recovery policies must not change the cutting tool.
        std::vector<double> means(myMesh->NbNodes(),0); std::vector<unsigned> counts(myMesh->NbNodes(),0);
        for (RWCae::MeshEntityId e = 0; e < myMesh->NbIndexedElems(); ++e) {
            const double value = entry.IsoValues->Value(RWCae::MeshPropertyEntityInfo(e));
            std::vector<RWCae::MeshEntityId> nodes; myMesh->ElementNodes(e,nodes);
            std::sort(nodes.begin(),nodes.end()); nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());
            for (auto n : nodes) { means[n] = CutFieldTools::Mean(means[n],value,++counts[n]); }
        }
        Handle(RWCae::MeshScalarProperty) projected = new RWCae::MeshScalarProperty(field->Name(),RWCae::MeshPropertyBase::MappingType::Nodal);
        projected->Init(myMesh->NbNodes());
        for (RWCae::MeshEntityId n = 0; n < myMesh->NbNodes(); ++n) {
            projected->SetValue(RWCae::MeshPropertyEntityInfo(n),counts[n] ? means[n] : 0);
        }
        entry.IsoValues = projected;
    }
    return entry.IsoValues;
}

bool MeshCutViewManager::bindIsoField(Entry& entry, const Handle(RWCae::MeshNVecProperty)& field,
    int component, double threshold)
{
    if (!std::isfinite(threshold) || !validIsoField(field, component)) { return false; }
    bool registered = false;
    for (const auto& candidate : myMesh->Properties()) {
        registered |= candidate == field;
        if (candidate != field && candidate->Name() == field->Name() && candidate->IsNodal() == field->IsNodal()
            && candidate->TimeStep() == field->TimeStep()) { return false; }
    }
    if (!registered) { myMesh->ChangeProperties().Append(field); }
    release(entry);
    entry.IsoValues.Nullify();
    entry.Definition.Shape = MeshCutShape::Isosurface;
    entry.Definition.IsoFieldName = field->Name().ToCString();
    entry.Definition.IsoFieldNodal = field->IsNodal();
    entry.Definition.IsoComponent = component;
    entry.Definition.IsoTimeStep = field->TimeStep();
    entry.Definition.IsoValue = threshold;
    ++entry.Revision;
    return true;
}

bool MeshCutViewManager::AddIsosurface(const std::string& id,
    const Handle(RWCae::MeshNVecProperty)& field, int component, double threshold)
{
    if (!validIsoField(field, component) || !std::isfinite(threshold)
        || !AddPlane(id, Graphic3d_Vec4d(1,0,0,0))) { return false; }
    if (bindIsoField(*find(id), field, component, threshold)) { return true; }
    Remove(id);
    return false;
}

bool MeshCutViewManager::SetIsosurface(const std::string& id,
    const Handle(RWCae::MeshNVecProperty)& field, int component, double threshold)
{
    auto entry = find(id);
    return entry != nullptr && entry->Definition.Shape == MeshCutShape::Isosurface
        && bindIsoField(*entry, field, component, threshold);
}

bool MeshCutViewManager::SetIsoValue(const std::string& id, double threshold)
{
    auto entry = find(id);
    if (entry == nullptr || entry->Definition.Shape != MeshCutShape::Isosurface
        || !std::isfinite(threshold)) { return false; }
    if (entry->Definition.IsoValue == threshold) { return true; }
    release(*entry);
    entry->Definition.IsoValue = threshold;
    ++entry->Revision;
    return true;
}

bool MeshCutViewManager::IsoRange(const std::string& id, double& minimum, double& maximum)
{
    auto entry = find(id);
    if (entry == nullptr || entry->Definition.Shape != MeshCutShape::Isosurface) { return false; }
    if (!validIsoField(resolveIsoField(entry->Definition), entry->Definition.IsoComponent)) { return false; }
    return isoValues(*entry)->ValueRange().GetBounds(minimum, maximum)
        && std::isfinite(minimum) && std::isfinite(maximum);
}

bool MeshCutViewManager::validQuadricMesh() const
{
    if (myMesh->NbIndexedElems() == 0) { return false; }
    for (const auto& cell : myMesh->GenericCells()) {
        auto concrete = MeshCellKernel::InstantiateCell(cell.Type);
        if (!concrete) { return false; }
    }
    return true;
}

bool MeshCutViewManager::needsGeneralPlane() const
{
    if (myMesh->NbElems(RWCae::MeshElemType::Elem0d) > 0) { return true; }
    if (myGeometryOptions.Mode == MeshCutGeometryMode::AnalyticRequired) {
        for (auto type : {RWCae::MeshElemType::Quad,RWCae::MeshElemType::Hexahedron,RWCae::MeshElemType::Prism,
            RWCae::MeshElemType::Pyramid,RWCae::MeshElemType::Polygon,RWCae::MeshElemType::Polyhedron}) {
            if (myMesh->NbElems(type) > 0) { return true; }
        }
    }
    for (auto type : {RWCae::MeshElemType::Generic,RWCae::MeshElemType::Triangle2nd,RWCae::MeshElemType::Quad2nd,
        RWCae::MeshElemType::Tetrahedron2nd,RWCae::MeshElemType::Hexahedron2nd,RWCae::MeshElemType::Prism2nd,
        RWCae::MeshElemType::Pyramid2nd}) {
        if (myMesh->NbElems(type) > 0) { return true; }
    }
    return false;
}

bool MeshCutViewManager::needsGeneralIso() const
{
    for (auto type : {RWCae::MeshElemType::Generic,RWCae::MeshElemType::Triangle2nd,RWCae::MeshElemType::Quad2nd,
        RWCae::MeshElemType::Tetrahedron2nd,RWCae::MeshElemType::Hexahedron2nd,RWCae::MeshElemType::Prism2nd,
        RWCae::MeshElemType::Pyramid2nd}) { if (myMesh->NbElems(type) > 0) { return true; } }
    return false;
}

bool MeshCutViewManager::setQuadric(Entry& entry, MeshCutShape shape,
    const Graphic3d_Vec3d& center, const Graphic3d_Vec3d& axis, double radius)
{
    if (!validQuadricMesh() || !std::isfinite(radius) || radius <= 1e-7) { return false; }
    for (int a = 0; a < 3; ++a) {
        if (!std::isfinite(center[a]) || !std::isfinite(axis[a])) { return false; }
    }
    const double length = std::hypot(std::hypot(axis.x(),axis.y()),axis.z());
    if (!std::isfinite(length) || length <= 1e-12) { return false; }
    const auto unit = std::abs(length-1) <= 8*std::numeric_limits<double>::epsilon() ? axis : axis/length;
    if (entry.Definition.Shape == shape && entry.Definition.Center == center
        && entry.Definition.Axis == unit && entry.Definition.Radius == radius) { return true; }
    release(entry);
    entry.Definition.Shape = shape;
    entry.Definition.Center = center; entry.Definition.Axis = unit; entry.Definition.Radius = radius;
    ++entry.Revision;
    return true;
}

bool MeshCutViewManager::AddSphere(const std::string& id, const Graphic3d_Vec3d& center, double radius)
{
    if (!AddPlane(id,Graphic3d_Vec4d(1,0,0,0))) { return false; }
    if (setQuadric(*find(id),MeshCutShape::Sphere,center,Graphic3d_Vec3d(0,0,1),radius)) { return true; }
    Remove(id);
    return false;
}

bool MeshCutViewManager::SetSphere(const std::string& id, const Graphic3d_Vec3d& center, double radius)
{
    auto entry = find(id);
    return entry != nullptr && entry->Definition.Shape == MeshCutShape::Sphere
        && setQuadric(*entry,MeshCutShape::Sphere,center,entry->Definition.Axis,radius);
}

bool MeshCutViewManager::AddCylinder(const std::string& id, const Graphic3d_Vec3d& center,
    const Graphic3d_Vec3d& axis, double radius)
{
    if (!AddPlane(id,Graphic3d_Vec4d(1,0,0,0))) { return false; }
    if (setQuadric(*find(id),MeshCutShape::Cylinder,center,axis,radius)) { return true; }
    Remove(id);
    return false;
}

bool MeshCutViewManager::SetCylinder(const std::string& id, const Graphic3d_Vec3d& center,
    const Graphic3d_Vec3d& axis, double radius)
{
    auto entry = find(id);
    return entry != nullptr && entry->Definition.Shape == MeshCutShape::Cylinder
        && setQuadric(*entry,MeshCutShape::Cylinder,center,axis,radius);
}

bool MeshCutViewManager::moveCachedPlane(Entry& theEntry, const Graphic3d_Vec4d& thePlane)
{
    const auto& oldPlane = theEntry.Definition.Plane;
    if ((!myDeformationField.IsNull() && theEntry.CachedMask!=2) || theEntry.CachedMask == 0 || oldPlane.x() != thePlane.x() || oldPlane.y() != thePlane.y()
        || oldPlane.z() != thePlane.z()) { return false; }
    // Only convex first-order cells have a plane topology determined by vertex signs alone.
    for (auto type : {RWCae::MeshElemType::Triangle2nd, RWCae::MeshElemType::Quad2nd,
                     RWCae::MeshElemType::Tetrahedron2nd, RWCae::MeshElemType::Pyramid2nd,
                     RWCae::MeshElemType::Prism2nd, RWCae::MeshElemType::Hexahedron2nd,
                     RWCae::MeshElemType::Polygon, RWCae::MeshElemType::Polyhedron}) {
        if (myMesh->NbElems(type) > 0) { return false; }
    }
    auto projection = [&thePlane](const Graphic3d_Vec3& p) {
        return thePlane.x()*p.x() + thePlane.y()*p.y() + thePlane.z()*p.z();
    };
    auto position=[&](RWCae::MeshEntityId n) { auto p=myMesh->Nodes()[n]; if(!myDeformationField.IsNull()) { theEntry.Roots[1]->ModifyNode(n,p); } return p; };
    if (theEntry.NodeProjections.empty()) {
        if(myMesh->NbNodes()>=65536) {
            // Sorted unique projections define exactly the same topology
            // intervals. Sort and merge bounded runs in parallel instead of
            // sorting millions of repeated layer coordinates on the UI thread.
            if(!myDeformationField.IsNull()) { theEntry.Roots[1]->GetMeshData(); }
            const int blockSize=8192,blocks=(int)(((size_t)myMesh->NbNodes()+blockSize-1)/blockSize);
            std::vector<std::vector<double>> runs(blocks);
            OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4);
            launcher.Perform(0,blocks,[&](int,int block) {
                auto& run=runs[block]; const size_t begin=(size_t)block*blockSize,end=std::min((size_t)myMesh->NbNodes(),begin+blockSize);
                run.reserve(end-begin); for(size_t n=begin;n<end;++n) { run.push_back(projection(position((RWCae::MeshEntityId)n))); }
                std::sort(run.begin(),run.end()); run.erase(std::unique(run.begin(),run.end()),run.end());
            });
            while(runs.size()>1) {
                std::vector<std::vector<double>> next((runs.size()+1)/2);
                launcher.Perform(0,(int)next.size(),[&](int,int i) {
                    const auto& a=runs[2*i]; auto& result=next[i];
                    if((size_t)(2*i+1)>=runs.size()) { result=a; return; }
                    const auto& b=runs[2*i+1]; result.reserve(a.size()+b.size());
                    std::set_union(a.begin(),a.end(),b.begin(),b.end(),std::back_inserter(result));
                });
                runs.swap(next);
            }
            theEntry.NodeProjections=std::move(runs.front());
        } else {
            theEntry.NodeProjections.reserve(myMesh->NbNodes());
            for(RWCae::MeshEntityId n=0;n<myMesh->NbNodes();++n) { theEntry.NodeProjections.push_back(projection(position(n))); }
            std::sort(theEntry.NodeProjections.begin(),theEntry.NodeProjections.end());
        }
    }
    const auto upper = std::upper_bound(theEntry.NodeProjections.begin(), theEntry.NodeProjections.end(), -oldPlane.w());
    const double low = upper == theEntry.NodeProjections.begin() ? -std::numeric_limits<double>::infinity() : *(upper-1);
    const double high = upper == theEntry.NodeProjections.end() ? std::numeric_limits<double>::infinity() : *upper;
    constexpr double confusion = 1e-5;
    if (-oldPlane.w() <= low + confusion || -oldPlane.w() >= high - confusion
        || -thePlane.w() <= low + confusion || -thePlane.w() >= high - confusion) { return false; }

    // Check every weight before mutation: near-node merging must not change topology.
    for (size_t i = 0; i < theEntry.Portions.size(); ++i) {
        if ((theEntry.CachedMask & (1u << i)) == 0) { continue; }
        for (MeshGeometry::NodesMap::Iterator it(theEntry.Portions[i]->GetNodesMap()); it.More(); it.Next()) {
            const auto& bases = it.Value().second.BaseNodes();
            if (bases.size() == 1) { continue; }
            if (bases.size() != 2 || bases[0].first == bases[1].first) { return false; }
            const double a = projection(position(bases[0].first));
            const double b = projection(position(bases[1].first));
            if (std::abs(b-a) <= confusion) { return false; }
            const double weight = (-thePlane.w()-a)/(b-a);
            if (weight <= 1e-3 || weight >= 1.0-1e-3 || bases[1].second <= 1e-3
                || bases[1].second >= 1.0-1e-3) { return false; }
        }
    }
    for (size_t i = 0; i < theEntry.Portions.size(); ++i) {
        if (i == 1) {
            auto slice = Handle(MeshGeometrySlice)::DownCast(theEntry.Portions[i]);
            slice->SlicePlanes().First()->SetEquation(thePlane);
        } else {
            auto clip = Handle(MeshGeometryClip)::DownCast(theEntry.Portions[i]);
            clip->ClipPlane()->SetEquation(i == 0 ? -thePlane : thePlane);
        }
        if ((theEntry.CachedMask & (1u << i)) == 0) { continue; }
        auto mesh = theEntry.Portions[i]->GetMeshData();
        for (MeshGeometry::NodesMap::Iterator it(theEntry.Portions[i]->ChangeNodesMap()); it.More(); it.Next()) {
            auto& bases = it.ChangeValue().second.BaseNodes();
            if (bases.size() == 1) { continue; }
            const auto a = position(bases[0].first);
            const auto b = position(bases[1].first);
            const double weight = (-thePlane.w()-projection(a))/(projection(b)-projection(a));
            bases[0].second = 1.0-weight;
            bases[1].second = weight;
            mesh->SetNodePosition(it.Key(), a*(float)(1.0-weight) + b*(float)weight);
        }
        myBvh->RemoveBvhBoxSet(theEntry.Portions[i]);
    }
    return true;
}

bool MeshCutViewManager::Rename(const std::string& theId, const std::string& theName)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr || theName.empty() || theName.find('\0') != std::string::npos) { return false; }
    anEntry->Definition.Name = theName;
    ++anEntry->Revision;
    return true;
}

bool MeshCutViewManager::Duplicate(const std::string& theId, const std::string& theNewId)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr || !AddPlane(theNewId, anEntry->Definition.Shape == MeshCutShape::Plane
        ? anEntry->Definition.Plane : Graphic3d_Vec4d(1,0,0,0))) { return false; }
    Entry* aCopy = find(theNewId);
    aCopy->Definition = anEntry->Definition;
    aCopy->Definition.Id = theNewId;
    aCopy->Definition.Name = theNewId;
    return true;
}

void MeshCutViewManager::release(Entry& theEntry)
{
    if (!theEntry.TraceRoot.IsNull()) { theEntry.TraceRoot->RemoveChildren(); }
    theEntry.TraceRoot.Nullify(); theEntry.TraceSection.Nullify(); theEntry.TraceRevision = 0;
    theEntry.CachedMask = 0;
    theEntry.NodeProjections.clear();
    for (size_t i = 0; i < theEntry.Roots.size(); ++i) {
        if (!theEntry.Roots[i].IsNull()) {
            myBvh->RemoveBvhBoxSet(theEntry.Roots[i]);
            if (!theEntry.Portions[i].IsNull()) { myBvh->RemoveBvhBoxSet(theEntry.Portions[i]); }
            theEntry.Roots[i]->RemoveChildren();
        }
        theEntry.Portions[i].Nullify();
        theEntry.Roots[i].Nullify();
    }
    theEntry.Lease.reset();
    theEntry.Reserved=0;
}

bool MeshCutViewManager::Remove(const std::string& theId)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr) { return false; }
    release(*anEntry);
    myEntries.erase(theId);
    return true;
}

bool MeshCutViewManager::SetVisibility(const std::string& theId, unsigned theMask)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr || theMask > 7) { return false; }
    anEntry->Definition.Visibility = theMask;
    ++anEntry->Revision;
    return true;
}

bool MeshCutViewManager::SetEnabled(const std::string& theId, bool theEnabled)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr) { return false; }
    anEntry->Definition.Enabled = theEnabled;
    ++anEntry->Revision;
    return true;
}

bool MeshCutViewManager::SetCellMode(const std::string& theId, MeshCutCellMode theMode)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr || (theMode != MeshCutCellMode::Constant && theMode != MeshCutCellMode::Projected)) {
        return false;
    }
    anEntry->Definition.CellMode = theMode;
    ++anEntry->Revision;
    return true;
}

bool MeshCutViewManager::SetTraceOptions(const std::string& id, bool physical, const MeshCutTraceOptions& options)
{
    auto entry = find(id);
    if (!entry || !options.IsValid(myMesh->NbIndexedElems())) { return false; }
    // Preflight only the proposed policy, including hidden sections, without
    // changing the geometry's retained display selection.
    const auto previous = entry->Definition;
    entry->Definition.PhysicalTrace = options;
    entry->Definition.Enabled = true; entry->Definition.Visibility |= 2;
    try { evaluate(id,nullptr,false); }
    catch (...) { entry->Definition = previous; return false; }
    entry->Definition = previous;
    (physical ? entry->Definition.PhysicalTrace : entry->Definition.DisplayTrace) = options;
    ++entry->Revision; return true;
}

std::vector<MeshCutDefinition> MeshCutViewManager::Definitions() const
{
    std::vector<MeshCutDefinition> aResult;
    for (const auto& anEntry : myEntries) { aResult.push_back(anEntry.second->Definition); }
    return aResult;
}

std::string MeshCutViewManager::DumpDefinitionsJson() const
{
    std::ostringstream stream;
    rapidjson::OStreamWrapper wrapper(stream);
    RWBase::JsonHelper::Writer writer(wrapper);
    writer.StartObject();
    writer.Key("version"); writer.Int(8);
    writer.Key("isoOptions"); writer.StartObject();
    writer.Key("absolute"); writer.Double(myIsoOptions.AbsoluteTolerance);
    writer.Key("relative"); writer.Double(myIsoOptions.RelativeTolerance);
    writer.Key("minimumDepth"); writer.Int(myIsoOptions.MinimumDepth);
    writer.Key("limit"); writer.Int(myIsoOptions.SubdivisionLimit);
    writer.Key("primitives"); writer.Uint64(myIsoOptions.MaxPrimitives);
    writer.EndObject();
    writer.Key("recovery"); writer.StartObject();
    writer.Key("threshold"); writer.Double(myRecoveryOptions.ThresholdPercent);
    writer.Key("before"); writer.Bool(myRecoveryOptions.InvariantsBeforeAveraging);
    writer.Key("tensor"); writer.StartObject();
    writer.Key("measure"); writer.Int((int)myRecoveryOptions.Tensor.Measure);
    writer.Key("layout"); writer.Int((int)myRecoveryOptions.Tensor.Layout);
    writer.Key("engineeringShear"); writer.Bool(myRecoveryOptions.Tensor.EngineeringShear);
    writer.EndObject();
    writer.Key("regions"); writer.StartArray();
    for (auto region : myRecoveryOptions.Regions) { writer.Int64(region); }
    writer.EndArray(); writer.EndObject();
    writer.Key("cuts"); writer.StartArray();
    for (const auto& entry : myEntries) {
        const auto& definition = entry.second->Definition;
        writer.StartObject();
        writer.Key("id"); writer.String(definition.Id.c_str());
        writer.Key("name"); writer.String(definition.Name.c_str());
        writer.Key("shape"); writer.String(definition.Shape == MeshCutShape::Plane ? "plane"
            : definition.Shape == MeshCutShape::Sphere ? "sphere"
            : definition.Shape == MeshCutShape::Cylinder ? "cylinder" : "isosurface");
        if (definition.Shape == MeshCutShape::Plane) {
            writer.Key("plane"); writer.StartArray();
            for (int i = 0; i < 4; ++i) { writer.Double(definition.Plane[i]); }
            writer.EndArray();
            const auto& p = definition.Placement;
            writer.Key("placement"); writer.StartObject();
            auto vector = [&](const char* key, const Graphic3d_Vec3d& v) {
                writer.Key(key); writer.StartArray(); for (int i = 0; i < 3; ++i) { writer.Double(v[i]); } writer.EndArray();
            };
            vector("origin",p.Origin); vector("normal",p.Normal); vector("axis2",p.Axis2);
            writer.Key("offset"); writer.Double(p.Offset);
            writer.Key("angle"); writer.Double(p.Angle);
            writer.Key("rotationAxis"); writer.Int(p.RotationAxis); writer.EndObject();
        } else if (definition.Shape == MeshCutShape::Cylinder || definition.Shape == MeshCutShape::Sphere) {
            writer.Key("center"); writer.StartArray();
            for (int a = 0; a < 3; ++a) { writer.Double(definition.Center[a]); }
            writer.EndArray();
            writer.Key("axis"); writer.StartArray();
            for (int a = 0; a < 3; ++a) { writer.Double(definition.Axis[a]); }
            writer.EndArray();
            writer.Key("radius"); writer.Double(definition.Radius);
        } else {
            writer.Key("field"); writer.String(definition.IsoFieldName.c_str());
            writer.Key("mapping"); writer.String(definition.IsoFieldNodal ? "point" : "cell");
            writer.Key("component"); writer.Int(definition.IsoComponent);
            writer.Key("step"); writer.Int(definition.IsoTimeStep);
            writer.Key("value"); writer.Double(definition.IsoValue);
        }
        writer.Key("visibility"); writer.Uint(definition.Visibility);
        writer.Key("enabled"); writer.Bool(definition.Enabled);
        writer.Key("cellMode"); writer.String(definition.CellMode == MeshCutCellMode::Constant ? "constant" : "projected");
        auto writeTrace = [&](const char* name, const MeshCutTraceOptions& options) {
            writer.Key(name); writer.StartObject();
            writer.Key("policy"); writer.Int((int)options.Policy);
            writer.Key("region"); writer.Int64(options.Region);
            writer.Key("regions"); writer.StartArray();
            for (auto region : options.Regions) { writer.Int64(region); }
            writer.EndArray(); writer.EndObject();
        };
        writeTrace("displayTrace",definition.DisplayTrace); writeTrace("physicalTrace",definition.PhysicalTrace);
        writer.EndObject();
    }
    writer.EndArray(); writer.EndObject();
    return stream.str();
}

bool MeshCutViewManager::RestoreDefinitionsJson(const std::string& theJson)
{
    rapidjson::Document document;
    document.Parse<rapidjson::kParseFullPrecisionFlag>(theJson.data(), theJson.size());
    if (document.HasParseError() || !document.IsObject() || !document.HasMember("version")
        || !document["version"].IsInt() || document["version"].GetInt() < 1 || document["version"].GetInt() > 8
        || !document.HasMember("cuts") || !document["cuts"].IsArray()) { return false; }
    std::map<std::string, std::unique_ptr<Entry>> entries;
    MeshCutIsoOptions isoOptions;
    MeshCutRecoveryOptions recoveryOptions;
    if (document["version"].GetInt() >= 5) {
        if (!document.HasMember("recovery") || !document["recovery"].IsObject()) { return false; }
        const auto& recovery = document["recovery"];
        if (!recovery.HasMember("threshold") || !recovery["threshold"].IsNumber()
            || !recovery.HasMember("before") || !recovery["before"].IsBool()
            || !recovery.HasMember("regions") || !recovery["regions"].IsArray()) { return false; }
        recoveryOptions.ThresholdPercent = recovery["threshold"].GetDouble();
        recoveryOptions.InvariantsBeforeAveraging = recovery["before"].GetBool();
        if (document["version"].GetInt() >= 6) {
            if (!recovery.HasMember("tensor") || !recovery["tensor"].IsObject()) { return false; }
            const auto& tensor = recovery["tensor"];
            if (!tensor.HasMember("measure") || !tensor["measure"].IsInt()
                || !tensor.HasMember("layout") || !tensor["layout"].IsInt()
                || !tensor.HasMember("engineeringShear") || !tensor["engineeringShear"].IsBool()) { return false; }
            recoveryOptions.Tensor.Measure = (MeshCutTensorMeasure)tensor["measure"].GetInt();
            recoveryOptions.Tensor.Layout = (MeshCutTensorLayout)tensor["layout"].GetInt();
            recoveryOptions.Tensor.EngineeringShear = tensor["engineeringShear"].GetBool();
        }
        for (const auto& region : recovery["regions"].GetArray()) {
            if (!region.IsInt64()) { return false; } recoveryOptions.Regions.push_back(region.GetInt64());
        }
        if (!recoveryOptions.IsValid(myMesh->NbIndexedElems())) { return false; }
    }
    if (document["version"].GetInt() >= 4) {
        if (!document.HasMember("isoOptions") || !document["isoOptions"].IsObject()) { return false; }
        const auto& options = document["isoOptions"];
        if (!options.HasMember("absolute") || !options["absolute"].IsNumber()
            || !options.HasMember("relative") || !options["relative"].IsNumber()
            || !options.HasMember("minimumDepth") || !options["minimumDepth"].IsInt()
            || !options.HasMember("limit") || !options["limit"].IsInt()
            || !options.HasMember("primitives") || !options["primitives"].IsUint64()) { return false; }
        isoOptions.AbsoluteTolerance = options["absolute"].GetDouble(); isoOptions.RelativeTolerance = options["relative"].GetDouble();
        isoOptions.MinimumDepth = options["minimumDepth"].GetInt(); isoOptions.SubdivisionLimit = options["limit"].GetInt();
        isoOptions.MaxPrimitives = options["primitives"].GetUint64(); if (!isoOptions.IsValid()) { return false; }
    }
    for (const auto& cut : document["cuts"].GetArray()) {
        if (!cut.IsObject() || !cut.HasMember("id") || !cut["id"].IsString() || !cut.HasMember("name")
            || !cut["name"].IsString() || !cut.HasMember("visibility") || !cut["visibility"].IsUint()
            || cut["visibility"].GetUint() > 7 || !cut.HasMember("enabled") || !cut["enabled"].IsBool()
            || !cut.HasMember("cellMode") || !cut["cellMode"].IsString()) { return false; }
        const std::string id(cut["id"].GetString(), cut["id"].GetStringLength());
        const std::string name(cut["name"].GetString(), cut["name"].GetStringLength());
        const std::string mode(cut["cellMode"].GetString(), cut["cellMode"].GetStringLength());
        if (id.empty() || name.empty() || id.find('\0') != std::string::npos || name.find('\0') != std::string::npos
            || entries.count(id) || (mode != "constant" && mode != "projected")) { return false; }
        auto entry = std::make_unique<Entry>();
        auto& definition = entry->Definition;
        definition.Id = id; definition.Name = name;
        definition.Visibility = cut["visibility"].GetUint(); definition.Enabled = cut["enabled"].GetBool();
        definition.CellMode = mode == "constant" ? MeshCutCellMode::Constant : MeshCutCellMode::Projected;
        if (document["version"].GetInt() >= 7) {
            auto readTrace = [&](const char* name, MeshCutTraceOptions& options) {
                if (!cut.HasMember(name) || !cut[name].IsObject()) { return false; }
                const auto& trace = cut[name];
                if (!trace.HasMember("policy") || !trace["policy"].IsInt()
                    || !trace.HasMember("region") || !trace["region"].IsInt64()
                    || !trace.HasMember("regions") || !trace["regions"].IsArray()) { return false; }
                options.Policy = (MeshCutTracePolicy)trace["policy"].GetInt(); options.Region = trace["region"].GetInt64();
                for (const auto& region : trace["regions"].GetArray()) {
                    if (!region.IsInt64()) { return false; } options.Regions.push_back(region.GetInt64());
                }
                return options.IsValid(myMesh->NbIndexedElems());
            };
            if (!readTrace("displayTrace",definition.DisplayTrace) || !readTrace("physicalTrace",definition.PhysicalTrace)) { return false; }
        }
        std::string shape = "plane";
        if (document["version"].GetInt() >= 2) {
            if (!cut.HasMember("shape") || !cut["shape"].IsString()) { return false; }
            shape.assign(cut["shape"].GetString(), cut["shape"].GetStringLength());
        }
        if (shape == "plane") {
            if (!cut.HasMember("plane") || !cut["plane"].IsArray() || cut["plane"].Size() != 4) { return false; }
            for (int i = 0; i < 4; ++i) {
                if (!cut["plane"][i].IsNumber()) { return false; }
                definition.Plane[i] = cut["plane"][i].GetDouble();
            }
            if (!normalizePlane(definition.Plane, definition.Plane)) { return false; }
            auto& p = definition.Placement;
            if (document["version"].GetInt() < 8) {
                if (!p.FromEquation(definition.Plane)) { return false; }
            } else {
                if (!cut.HasMember("placement") || !cut["placement"].IsObject()) { return false; }
                const auto& value = cut["placement"];
                auto vector = [&](const char* key, Graphic3d_Vec3d& v) {
                    if (!value.HasMember(key) || !value[key].IsArray() || value[key].Size() != 3) { return false; }
                    for (int i = 0; i < 3; ++i) {
                        if (!value[key][i].IsNumber()) { return false; } v[i] = value[key][i].GetDouble();
                    } return true;
                };
                if (!vector("origin",p.Origin) || !vector("normal",p.Normal) || !vector("axis2",p.Axis2)
                    || !value.HasMember("offset") || !value["offset"].IsNumber()
                    || !value.HasMember("angle") || !value["angle"].IsNumber()
                    || !value.HasMember("rotationAxis") || !value["rotationAxis"].IsInt()) { return false; }
                p.Offset = value["offset"].GetDouble(); p.Angle = value["angle"].GetDouble(); p.RotationAxis = value["rotationAxis"].GetInt();
                Graphic3d_Vec4d derived;
                if (!p.IsValid() || !normalizePlane(p.Equation(),derived)) { return false; }
                for (int i = 0; i < 4; ++i) {
                    if (std::abs(derived[i]-definition.Plane[i]) > 1e-10*std::max(1.0,std::abs(derived[i]))) { return false; }
                }
            }
        } else if (shape == "sphere" || shape == "cylinder") {
            if (document["version"].GetInt() < 3 || !cut.HasMember("center") || !cut["center"].IsArray()
                || cut["center"].Size() != 3 || !cut.HasMember("axis") || !cut["axis"].IsArray()
                || cut["axis"].Size() != 3 || !cut.HasMember("radius") || !cut["radius"].IsNumber()) { return false; }
            Graphic3d_Vec3d center, axis;
            for (int a = 0; a < 3; ++a) {
                if (!cut["center"][a].IsNumber() || !cut["axis"][a].IsNumber()) { return false; }
                center[a] = cut["center"][a].GetDouble(); axis[a] = cut["axis"][a].GetDouble();
            }
            if (!setQuadric(*entry,shape == "sphere" ? MeshCutShape::Sphere : MeshCutShape::Cylinder,
                center,axis,cut["radius"].GetDouble())) { return false; }
        } else if (shape == "isosurface") {
            if (!cut.HasMember("field") || !cut["field"].IsString() || !cut.HasMember("mapping")
                || !cut["mapping"].IsString() || !cut.HasMember("component") || !cut["component"].IsInt()
                || !cut.HasMember("step") || !cut["step"].IsInt() || !cut.HasMember("value")
                || !cut["value"].IsNumber() || !std::isfinite(cut["value"].GetDouble())) { return false; }
            const std::string mapping(cut["mapping"].GetString(), cut["mapping"].GetStringLength());
            if (mapping != "point" && mapping != "cell") { return false; }
            definition.Shape = MeshCutShape::Isosurface;
            definition.IsoFieldName.assign(cut["field"].GetString(), cut["field"].GetStringLength());
            definition.IsoFieldNodal = mapping == "point";
            definition.IsoComponent = cut["component"].GetInt(); definition.IsoTimeStep = cut["step"].GetInt();
            definition.IsoValue = cut["value"].GetDouble();
            if (definition.IsoFieldName.empty() || definition.IsoFieldName.find('\0') != std::string::npos
                || !validIsoField(resolveIsoField(definition), definition.IsoComponent)) { return false; }
        } else { return false; }
        entries.emplace(id, std::move(entry));
    }
    // Validate source resolution on isolated caches before replacing live cuts.
    // Older documents migrate to lowest and need no extra geometry preparation.
    try {
        MeshCutViewManager staged(myMesh);
        if(!staged.ShareCacheFrom(*this)||!staged.SetCachePolicy(CachePolicy())) { return false; }
        staged.myGeometryOptions = myGeometryOptions; staged.myIsoOptions = isoOptions;
        staged.myDeformationField = myDeformationField; staged.myDeformationScale = myDeformationScale;
        for (const auto& item : entries) {
            auto copy = std::make_unique<Entry>(); copy->Definition = item.second->Definition;
            staged.myEntries.emplace(item.first,std::move(copy));
        }
        for (const auto& item : entries) {
            const auto& definition = item.second->Definition;
            if (definition.DisplayTrace.Policy != MeshCutTracePolicy::Lowest
                && !staged.SetTraceOptions(item.first,false,definition.DisplayTrace)) { return false; }
            if (definition.PhysicalTrace.Policy != MeshCutTracePolicy::Lowest
                && !staged.SetTraceOptions(item.first,true,definition.PhysicalTrace)) { return false; }
        }
    } catch (...) { return false; }
    Clear();
    myIsoOptions = isoOptions;
    myRecoveryOptions = recoveryOptions;
    myEntries = std::move(entries);
    return true;
}

bool MeshCutViewManager::SetDeformation(const Handle(RWCae::MeshNVecProperty)& theField, double theScale)
{
    if (!std::isfinite(theScale) || (!theField.IsNull() && (!theField->IsInitialized() || !theField->IsNodal()
        || !theField->IsVector3() || !theField->HasScalarComponents()
        || theField->GetComponent(0)->NbValues() != (int)myMesh->NbNodes()))) { return false; }
    if (!theField.IsNull()) {
        double min, max;
        if (!theField->GetRawComponent(0)->ValueRange().GetBounds(min, max) || !std::isfinite(min)
            || !std::isfinite(max)) { return false; }
        for (int c = 0; c < 3; ++c) {
            auto component = theField->GetComponent(c);
            if (component->NbValues() != (int)myMesh->NbNodes()) { return false; }
            for (int n = 0; n < component->NbValues(); ++n) {
                if (std::isinf(component->Value(RWCae::MeshPropertyEntityInfo(n)))) { return false; }
            }
        }
    }
    myDeformationField = theField;
    if (!myDeformationField.IsNull()) { myMesh->BoundingBox(); }
    myDeformationScale = theScale;
    InvalidateGeometry();
    return true;
}

double MeshCutViewManager::EffectiveDeformationScale() const
{
    return myDeformationField.IsNull() ? 0.0
        : myDeformationScale * RWCae::MeshTools::ComputeVectorTrsfRatio(myMesh, myDeformationField);
}

Handle(MeshGeometry) MeshCutViewManager::makeSourceGeometry(bool boundary)
{
    reserveSource();
    Handle(MeshGeometry) root = new MeshGeometryBase(myBvh, myMesh);
    root->RetainCacheLease(mySourceLease);
    if (!myDeformationField.IsNull()) {
        Handle(MeshGeometryDeformed) deformed = new MeshGeometryDeformed(myBvh);
        root->Append(deformed);
        deformed->SetDeformedProperty(myDeformationField);
        deformed->SetDeformedScaleCoeff(myDeformationScale);
    }
    if (boundary && needsGeneralPlane()) {
        root->Append(new MeshGeometryVtkCut(myBvh,MeshCutDefinition(),MeshCutPortion::Negative,generalSource(),true));
    }
    return root;
}

Handle(MeshGeometry) MeshCutViewManager::SourceGeometry()
{
    if (mySourceGeometry.IsNull()) { mySourceGeometry = makeSourceGeometry(); }
    return mySourceGeometry;
}

Bnd_Box MeshCutViewManager::SourceBounds()
{
    Bnd_Box bounds;
    const auto geometry = SourceGeometry();
    for (RWCae::MeshEntityId n = 0; n < myMesh->NbNodes(); ++n) {
        auto point = myMesh->Nodes()[n];
        geometry->ModifyNode(n, point);
        bounds.Add(gp_Pnt(point.x(), point.y(), point.z()));
    }
    return bounds;
}

void MeshCutViewManager::initialize(Entry& theEntry)
{
    if (!theEntry.Roots[0].IsNull()) { return; }
    if ((theEntry.Definition.Shape == MeshCutShape::Sphere || theEntry.Definition.Shape == MeshCutShape::Cylinder)
        && !validQuadricMesh()) {
        throw std::invalid_argument("Curved cuts currently support only linear triangle/tetrahedron meshes");
    }
    Handle(RWCae::MeshScalarPropertyBase) isoValues;
    if (theEntry.Definition.Shape == MeshCutShape::Isosurface) {
        isoValues = this->isoValues(theEntry);
    }
    for (size_t i = 0; i < theEntry.Roots.size(); ++i) {
        theEntry.Roots[i] = makeSourceGeometry(isoValues.IsNull());
        theEntry.Roots[i]->RetainCacheLease(theEntry.Lease);
        bool nonSimplex = false;
        for (int t = 0; t < RWCae::MeshElemNbTypes; ++t) {
            const auto type = (RWCae::MeshElemType)t;
            nonSimplex |= type != RWCae::MeshElemType::Triangle && type != RWCae::MeshElemType::Tetrahedron && myMesh->NbElems(type) > 0;
        }
        if ((theEntry.Definition.Shape == MeshCutShape::Plane && needsGeneralPlane())
            || ((theEntry.Definition.Shape == MeshCutShape::Sphere || theEntry.Definition.Shape == MeshCutShape::Cylinder) && nonSimplex)) {
            theEntry.Portions[i] = new MeshGeometryVtkCut(myBvh,theEntry.Definition,(MeshCutPortion)i,generalSource());
        } else if (!isoValues.IsNull() && needsGeneralIso()) {
            theEntry.Portions[i] = new MeshGeometryVtkIso(myBvh,theEntry.Definition,(MeshCutPortion)i,isoValues,myGeometryOptions,myIsoOptions);
        } else
        if (theEntry.Definition.Shape == MeshCutShape::Sphere || theEntry.Definition.Shape == MeshCutShape::Cylinder) {
            theEntry.Portions[i] = new MeshGeometryQuadric(myBvh,theEntry.Definition,(MeshCutPortion)i,myGeometryOptions);
        } else if (!isoValues.IsNull()) {
            theEntry.Portions[i] = new ScalarCutGeometry(myBvh, isoValues, theEntry.Definition.IsoValue,
                i == 0 ? -1 : i == 1 ? 0 : 1);
        } else if (i == 1) {
            Handle(MeshGeometrySlice) aSlice = new MeshGeometrySlice(myBvh);
            Graphic3d_SequenceOfHClipPlane aPlanes;
            aPlanes.Append(new Graphic3d_ClipPlane(theEntry.Definition.Plane));
            aSlice->SetSlicePlanes(aPlanes);
            theEntry.Portions[i] = aSlice;
        } else {
            Handle(MeshGeometryClip) aClip = new MeshGeometryClip(myBvh);
            aClip->SetCut(true);
            aClip->SetClipPlane(new Graphic3d_ClipPlane(i == 0 ? -theEntry.Definition.Plane : theEntry.Definition.Plane));
            theEntry.Portions[i] = aClip;
        }
        theEntry.Roots[i]->Append(theEntry.Portions[i]);
        theEntry.Portions[i]->RetainCacheLease(theEntry.Lease);
        theEntry.Portions[i]->RetainCacheLease(mySourceLease);
    }
}

Handle(RWCae::MeshNVecProperty) MeshCutViewManager::project(const Handle(RWCae::MeshNVecProperty)& theField)
{
    if (myProjectedSource == theField && !myProjectedField.IsNull()) { return myProjectedField; }
    if (theField->HasNodeVecComponents() || (theField->Type() == RWCae::MeshNVecPropertyType::Tensor
        && myRecoveryOptions.Tensor.Measure != MeshCutTensorMeasure::Components)
        || myRecoveryOptions.ThresholdPercent != 100
        || !myRecoveryOptions.Regions.empty() || myRecoveryOptions.InvariantsBeforeAveraging) {
        auto recovered = RecoverCutField(myMesh,theField,myRecoveryOptions);
        myProjectedSource = theField; myProjectedField = recovered; return recovered;
    }
    Handle(RWCae::MeshNVecProperty) aResult = new RWCae::MeshNVecProperty(
        theField->Name(), theField->NbComponents(), RWCae::MeshPropertyBase::MappingType::Nodal, theField->Type());
    aResult->Init<RWCae::MeshScalarProperty>();
    const size_t aNbNodes = myMesh->NbNodes();
    for (int c = 0; c < theField->NbComponents(); ++c) {
        std::vector<double> aMeans(aNbNodes, 0.0);
        std::vector<unsigned> aCounts(aNbNodes, 0);
        auto aSource = theField->GetComponent(c);
        // Equal-weight cell-to-node projection; no mixing across disconnected nodes.
        for (RWCae::MeshEntityId i = 0; i < myMesh->NbIndexedElems(); ++i) {
            const double aValue = aSource->Value(RWCae::MeshPropertyEntityInfo(i));
            if (std::isinf(aValue)) { throw std::invalid_argument("Recovery field contains infinity"); }
            if (!std::isfinite(aValue)) { continue; }
            std::vector<RWCae::MeshEntityId> nodes;
            myMesh->ElementNodes(i,nodes);
            std::sort(nodes.begin(),nodes.end()); nodes.erase(std::unique(nodes.begin(),nodes.end()),nodes.end());
            for (auto aNode : nodes) {
                aMeans.at(aNode) = CutFieldTools::Mean(aMeans.at(aNode),aValue,++aCounts.at(aNode));
            }
        }
        auto aTarget = Handle(RWCae::MeshScalarProperty)::DownCast(aResult->GetComponent(c));
        aTarget->Init((int)aNbNodes);
        aTarget->SetName(aSource->Name());
        aTarget->SetDescription(aSource->Description());
        aTarget->SetDisplayUnits(aSource->DisplayUnits());
        aTarget->SetEnabled(aSource->IsEnabled());
        for (size_t n = 0; n < aNbNodes; ++n) {
            aTarget->SetValue(RWCae::MeshPropertyEntityInfo((int)n), aCounts[n] == 0
                ? std::numeric_limits<double>::quiet_NaN() : aMeans[n]);
        }
    }
    aResult->SetTime(theField->Time(), theField->TimeStep());
    aResult->SetTimeStepDescription(theField->TimeStepDescription());
    aResult->SetTimeDisplayUnits(theField->TimeDisplayUnits());
    aResult->SetDescription(theField->Description());
    if (aResult->IsVector()) {
        CutFieldTools::CopyMetadata(theField->GetRawComponent(0),aResult->GetRawComponent(0));
    }
    aResult->UpdateComponents();
    myProjectedSource = theField;
    myProjectedField = aResult;
    return aResult;
}

MeshCutResult MeshCutViewManager::Evaluate(const std::string& theId,
    const Handle(RWCae::MeshNVecProperty)& theField)
{
    return evaluate(theId,theField,true);
}

MeshCutResult MeshCutViewManager::EvaluatePhysical(const std::string& theId,
    const Handle(RWCae::MeshNVecProperty)& theField)
{
    return evaluate(theId,theField,false);
}

MeshCutResult MeshCutViewManager::EvaluateSectionForDisplay(const std::string& theId,
    const Handle(RWCae::MeshNVecProperty)& theField)
{
    return evaluate(theId,theField,true,2);
}

MeshCutResult MeshCutViewManager::EvaluateFieldForDisplay(const std::string& theId,
    const Handle(RWCae::MeshNVecProperty)& theField)
{
    return evaluate(theId,theField,true,0);
}

MeshCutResult MeshCutViewManager::evaluate(const std::string& theId,
    const Handle(RWCae::MeshNVecProperty)& theField, bool display, unsigned requestedMask)
{
    Entry* anEntry = find(theId);
    if (anEntry == nullptr) { throw std::invalid_argument("Unknown cut id: " + theId); }
    const auto request=myRequestGeneration; checkRequest(request); myTimings={};
    MeshCutResult aResult;
    aResult.Definition = anEntry->Definition;
    aResult.Revision = anEntry->Revision;
    const unsigned mask = requestedMask == 8 ? anEntry->Definition.Visibility : requestedMask;
    if (!anEntry->Definition.Enabled || (mask == 0 && requestedMask != 0)) { return aResult; }
    if (mask != 0) { prepareCache(*anEntry,theField); }
    if (!theField.IsNull()) { CutFieldTools::Validate(myMesh,theField); }
    using Clock=std::chrono::steady_clock;
    auto phase=Clock::now(); auto elapsed=[&] { const auto end=Clock::now(); const double value=std::chrono::duration<double,std::milli>(end-phase).count(); phase=end; return value; };
    if (mask != 0) { initialize(*anEntry); }
    myTimings.SourceMs+=elapsed(); checkRequest(request);
    const bool projected = display && !theField.IsNull() && theField->IsElemental()
        && anEntry->Definition.CellMode == MeshCutCellMode::Projected;
    aResult.TensorDerived = display && !theField.IsNull() && theField->Type() == RWCae::MeshNVecPropertyType::Tensor
        && myRecoveryOptions.Tensor.Measure != MeshCutTensorMeasure::Components;
    if (aResult.TensorDerived && projected && myRecoveryOptions.InvariantsBeforeAveraging) {
        aResult.SourceField = project(DeriveCutTensorField(theField,myRecoveryOptions.Tensor));
    } else {
        aResult.SourceField = projected ? project(theField) : theField;
        if (aResult.TensorDerived) { aResult.SourceField = DeriveCutTensorField(aResult.SourceField,myRecoveryOptions.Tensor); }
    }
    aResult.DisplayRecovered = aResult.SourceField != theField;
    myTimings.FieldsMs+=elapsed();
    for (size_t i = 0; i < anEntry->Portions.size(); ++i) {
        if ((mask & (1u << i)) == 0) { continue; }
        aResult.Roots[i] = anEntry->Roots[i];
        aResult.Portions[i] = anEntry->Portions[i];
        const auto cancellable=Handle(MeshGeometrySlice)::DownCast(aResult.Portions[i]);
        if(!cancellable.IsNull()) { cancellable->SetCancellation(myCancellation); }
        const size_t workGeneration=cancellable.IsNull()?0:cancellable->WorkGeneration();
        aResult.Portions[i]->GetMeshData();
        if(!cancellable.IsNull() && cancellable->WorkGeneration()!=workGeneration) {
            const auto& work=cancellable->LastWork();
            myTimings.BvhMs+=work.BvhMs; myTimings.NodePrepareMs+=work.PrepareMs;
            myTimings.CellCutMs+=work.CutMs; myTimings.MergeMs+=work.MergeMs;
            myTimings.Candidates+=work.Candidates; myTimings.Fallbacks+=work.Fallbacks;
        }
        { const double time=elapsed(); myTimings.IntersectionMs+=time; myTimings.SliceMs+=time; } checkRequest(request);
        if (auto quadric = dynamic_cast<MeshGeometryQuadric*>(aResult.Portions[i].get())) {
            aResult.Volumes[i] = quadric->Volume(); aResult.Areas[i] = quadric->Area();
            aResult.BoundaryMesh = true;
        }
        if (auto iso = dynamic_cast<MeshGeometryVtkIso*>(aResult.Portions[i].get())) {
            aResult.Volumes[i] = iso->Volume; aResult.Areas[i] = iso->Area; aResult.SourceApproximate = true;
            aResult.IsoFieldTolerance = std::max(aResult.IsoFieldTolerance,iso->FieldTolerance);
            aResult.IsoFieldResidual = std::max(aResult.IsoFieldResidual,iso->FieldResidual);
            aResult.IsoGeometryResidual = std::max(aResult.IsoGeometryResidual,iso->GeometryResidual);
            aResult.IsoSubdivisionDepth = std::max(aResult.IsoSubdivisionDepth,iso->Depth);
            aResult.IsoPrimitives = std::max(aResult.IsoPrimitives,iso->Primitives);
        }
        if (auto general = dynamic_cast<MeshGeometryVtkCut*>(aResult.Portions[i].get())) {
            aResult.Volumes[i] = general->Volume(); aResult.Areas[i] = general->Area(); aResult.BoundaryMesh = true;
            aResult.SourceApproximate = generalSource()->Work.NumericalCells > 0;
        }
        anEntry->CachedMask |= 1u << i;
        if (i == 1) {
            if (anEntry->TraceSection.IsNull() || anEntry->TraceRevision != anEntry->Revision) {
                if (!myTraceSource) {
                    myTraceSource = std::make_shared<MeshCutTraceSource>(myMesh,makeSourceGeometry(false),myGeometryOptions
                        ,myGeneralSource
                    );
                    myTimings.SourceMs+=elapsed(); checkRequest(request);
                }
                if (anEntry->TraceSection.IsNull()) {
                    anEntry->TraceSection = new MeshGeometryTraceSection(myBvh,Handle(MeshGeometrySlice)::DownCast(anEntry->Portions[1]),
                        anEntry->Roots[1],myTraceSource);
                    anEntry->TraceRoot = new MeshGeometryBase(myBvh,myMesh); anEntry->TraceRoot->Append(anEntry->TraceSection);
                    anEntry->TraceRoot->RetainCacheLease(anEntry->Lease); anEntry->TraceSection->RetainCacheLease(anEntry->Lease);
                    anEntry->TraceRoot->RetainCacheLease(mySourceLease); anEntry->TraceSection->RetainCacheLease(mySourceLease);
                } else { anEntry->TraceSection->Refresh(Handle(MeshGeometrySlice)::DownCast(anEntry->Portions[1]),myTraceSource); }
                anEntry->TraceRevision = anEntry->Revision;
                const auto& work=anEntry->TraceSection->TraceWork();
                myTimings.TraceOriginsMs=work.OriginsMs; myTimings.TraceClassifyMs=work.ClassifyMs;
                myTimings.TracePrepareMs=work.PrepareMs; myTimings.TraceMapsMs=work.MapsMs;
                myTimings.TraceFillMs=work.FillMs; myTimings.TraceGeneralMs=work.GeneralMs;
                { const double time=elapsed(); myTimings.IntersectionMs+=time; myTimings.TraceMs+=time; } checkRequest(request);
            }
            aResult.Roots[1] = anEntry->TraceRoot; aResult.Portions[1] = anEntry->TraceSection;
            const auto definition = anEntry->Definition;
            const auto iso = definition.Shape == MeshCutShape::Isosurface ? isoValues(*anEntry) : Handle(RWCae::MeshScalarPropertyBase)();
            MeshGeometryTraceSection::SignedValue signedValue = [definition,iso](const MeshCutPrimitiveTrace& trace) {
                const auto p = trace.Interior;
                if (definition.Shape == MeshCutShape::Plane) {
                    return definition.Plane.x()*p.x()+definition.Plane.y()*p.y()+definition.Plane.z()*p.z()+definition.Plane.w();
                }
                if (definition.Shape == MeshCutShape::Isosurface) {
                    double value = 0;
                    for (const auto& w : trace.InteriorWeights.BaseNodes()) {
                        if (w.second != 0) { value += w.second*iso->Value(RWCae::MeshPropertyEntityInfo(w.first)); }
                    }
                    return value-definition.IsoValue;
                }
                auto radius = p-definition.Center;
                if (definition.Shape == MeshCutShape::Cylinder) { radius -= definition.Axis*radius.Dot(definition.Axis); }
                return radius.Modulus()-definition.Radius;
            };
            double sideTolerance = myGeometryOptions.AbsoluteTolerance;
            if (!iso.IsNull()) {
                double low = 0, high = 0; iso->ValueRange().GetBounds(low,high);
                sideTolerance = std::max(myIsoOptions.AbsoluteTolerance,myIsoOptions.RelativeTolerance*(high-low));
            }
            aResult.TraceTolerance = sideTolerance;
            aResult.PhysicalProvenance = [section = anEntry->TraceSection,definition,signedValue,sideTolerance](size_t primitive, bool lines) -> const MeshCutPrimitiveTrace& {
                return section->PhysicalProvenance(primitive,definition.PhysicalTrace,signedValue,sideTolerance,lines);
            };
            // Resolve physical options even with no field so invalid edits can
            // be preflighted independently of contour binding.
            anEntry->TraceSection->MapPhysical(nullptr,definition.PhysicalTrace,signedValue,sideTolerance);
            anEntry->TraceSection->MapPhysical(nullptr,definition.PhysicalTrace,signedValue,sideTolerance,true);
            if (display) { anEntry->TraceSection->SetDisplaySelection(definition.DisplayTrace,signedValue,sideTolerance); }
            if (!theField.IsNull()) {
                aResult.PhysicalFields[1] = anEntry->TraceSection->MapPhysical(theField,definition.PhysicalTrace,signedValue,sideTolerance);
                aResult.PhysicalLineField = anEntry->TraceSection->MapPhysical(theField,definition.PhysicalTrace,signedValue,sideTolerance,true);
                const bool same=!display || (aResult.SourceField==theField && definition.DisplayTrace.Policy==definition.PhysicalTrace.Policy
                    && definition.DisplayTrace.Region==definition.PhysicalTrace.Region && definition.DisplayTrace.Regions==definition.PhysicalTrace.Regions);
                aResult.Fields[1] = same ? aResult.PhysicalFields[1] : anEntry->TraceSection->CalculateProperty(aResult.SourceField);
                aResult.LineField = same ? aResult.PhysicalLineField : anEntry->TraceSection->CalculateLineProperty(aResult.SourceField);
            }
            myTimings.FieldsMs+=elapsed(); checkRequest(request);
            continue;
        }
        if (!aResult.SourceField.IsNull()) {
            aResult.Fields[i] = aResult.Portions[i]->CalculateProperty(aResult.SourceField);
            aResult.PhysicalFields[i] = aResult.DisplayRecovered ? aResult.Portions[i]->CalculateProperty(theField) : aResult.Fields[i];
            if (i == 1) {
                aResult.LineField = Handle(MeshGeometrySlice)::DownCast(aResult.Portions[i])
                    ->CalculateLineProperty(aResult.SourceField);
            }
        }
    }
    myTimings.FieldsMs+=elapsed(); checkRequest(request);
    return aResult;
}

std::vector<MeshCutResult> MeshCutViewManager::EvaluateVisible(const Handle(RWCae::MeshNVecProperty)& theField)
{
    std::vector<MeshCutResult> aResults;
    for (const auto& anEntry : myEntries) {
        if (anEntry.second->Definition.Enabled && anEntry.second->Definition.Visibility != 0) {
            aResults.push_back(Evaluate(anEntry.first, theField));
        }
    }
    return aResults;
}

std::shared_ptr<MeshCutPresentation> MeshCutViewManager::EvaluatePresentation(const Handle(RWCae::MeshNVecProperty)& field)
{
    const auto cuts = EvaluateVisible(field);
    if (cuts.empty()) { return std::make_shared<MeshCutPresentation>(); }
    auto source = makeSourceGeometry(false);
    if (!myTraceSource) {
        myTraceSource = std::make_shared<MeshCutTraceSource>(myMesh,source,myGeometryOptions
            ,myGeneralSource
        );
    }
    return BuildCutPresentationUnion(myMesh,source,myBvh,cuts,myGeometryOptions,myTraceSource
        ,myGeneralSource
    );
}

void MeshCutViewManager::InvalidateFields()
{
    ++myFieldEpoch;
    myProjectedSource.Nullify();
    myProjectedField.Nullify();
    for (auto& anEntry : myEntries) {
        if (anEntry.second->Definition.Shape == MeshCutShape::Isosurface) {
            release(*anEntry.second);
            anEntry.second->IsoValues.Nullify();
        }
        ++anEntry.second->Revision;
    }
}

void MeshCutViewManager::InvalidateGeometry()
{
    myTraceSource.reset();
    myGeneralSource.reset();
    ++mySourceEpoch;
    if (!mySourceGeometry.IsNull()) { mySourceGeometry->RemoveChildren(); mySourceGeometry.Nullify(); }
    for (auto& anEntry : myEntries) {
        release(*anEntry.second);
        ++anEntry.second->Revision;
    }
    myBvh->Clear();
    mySourceLease.reset(); mySourceReserved=0;
    InvalidateFields();
}

void MeshCutViewManager::Clear()
{
    CancelRequest();
    myTraceSource.reset();
    myGeneralSource.reset();
    ++mySourceEpoch;
    if (!mySourceGeometry.IsNull()) { mySourceGeometry->RemoveChildren(); mySourceGeometry.Nullify(); }
    for (auto& anEntry : myEntries) { release(*anEntry.second); }
    myEntries.clear();
    myBvh->Clear();
    mySourceLease.reset(); mySourceReserved=0;
    InvalidateFields();
}

bool MeshCutViewManager::ShareCacheFrom(const MeshCutViewManager& other)
{
    if(mySourceLease || std::any_of(myEntries.begin(),myEntries.end(),[](const auto& e) { return bool(e.second->Lease); })) { return false; }
    myCache=other.myCache; return true;
}
void MeshCutViewManager::reserveSource()
{
    const auto bytes=EstimatedSourceBytes(); if(bytes<=mySourceReserved) { return; }
    auto extra=myCache.Reserve(bytes-mySourceReserved,true);
    if(!mySourceLease) { mySourceLease=std::move(extra); }
    else { mySourceLease=std::shared_ptr<void>(new std::pair<std::shared_ptr<void>,std::shared_ptr<void>>(mySourceLease,extra),[](void* p) { delete static_cast<std::pair<std::shared_ptr<void>,std::shared_ptr<void>>*>(p); }); }
    mySourceReserved=bytes;
    if(!mySourceGeometry.IsNull()) { mySourceGeometry->RetainCacheLease(mySourceLease); }
    for(const auto& entry:myEntries) { for(const auto& root:entry.second->Roots) { if(!root.IsNull()) { root->RetainCacheLease(mySourceLease); } } for(const auto& portion:entry.second->Portions) { if(!portion.IsNull()) { portion->RetainCacheLease(mySourceLease); } } if(!entry.second->TraceRoot.IsNull()) { entry.second->TraceRoot->RetainCacheLease(mySourceLease); entry.second->TraceSection->RetainCacheLease(mySourceLease); } }
}
uint64_t MeshCutViewManager::EstimatedSourceBytes() const
{
    bool compact=true;
    for(int t=0;t<RWCae::MeshElemNbTypes;++t) { const auto type=(RWCae::MeshElemType)t; using T=RWCae::MeshElemType;
        if(myMesh->NbElems(type)>0 && type!=T::Elem0d&&type!=T::Triangle&&type!=T::Quad&&type!=T::Tetrahedron&&type!=T::Pyramid&&type!=T::Prism&&type!=T::Hexahedron) { compact=false; }
    }
    bool nonSimplex=false;
    for(int t=0;t<RWCae::MeshElemNbTypes;++t) { const auto type=(RWCae::MeshElemType)t; nonSimplex|=myMesh->NbElems(type)>0 && type!=RWCae::MeshElemType::Triangle && type!=RWCae::MeshElemType::Tetrahedron; }
    if(nonSimplex) { for(const auto& entry:myEntries) { if(entry.second->Definition.Shape==MeshCutShape::Sphere || entry.second->Definition.Shape==MeshCutShape::Cylinder) { compact=false; } } }
    return 65536 + (uint64_t)myMesh->NbNodes()*128 + (uint64_t)myMesh->NbIndexedElems()*(compact?128:65536)
        + (compact?std::min<uint64_t>((uint64_t)myMesh->NbIndexedElems()+2,8194)*65536+std::min<uint64_t>(myMesh->NbIndexedElems(),100000)*512:myGeometryOptions.MaxSourcePrimitives*512);
}
uint64_t MeshCutViewManager::EstimatedResultBytes(const Handle(RWCae::MeshNVecProperty)& field) const
{
    const uint64_t components=field.IsNull()?0:(uint64_t)field->NbRawComponents();
    const uint64_t cells=myMesh->NbIndexedElems(),nodes=myMesh->NbNodes();
    if(components>1024 || cells>100000000 || nodes>100000000) { throw std::length_error("Cache estimate exceeds supported resource schema"); }
    const bool complex=needsGeneralPlane() || needsGeneralIso() || myMesh->NbElems(RWCae::MeshElemType::Polygon)>0 || myMesh->NbElems(RWCae::MeshElemType::Polyhedron)>0;
    // Bound small general sources by their actual connectivity as well as the
    // global refinement limit. Reserving the entire global limit for every cut
    // otherwise rejects a few cuts on a two-cell fixture with pinned results.
    uint64_t primitives=0;
    if(complex) {
        uint64_t support=0,linearSupport=0;
        for(const auto& cell:myMesh->GenericCells()) {
            const bool linear=(cell.Type>=1&&cell.Type<=16)||cell.Type==41||cell.Type==42;
            auto& count=linear?linearSupport:support;
            count+=cell.Nodes.size(); for(const auto& face:cell.Faces) { count+=face.size(); }
        }
        if(myMesh->NbElems(RWCae::MeshElemType::Polygon)>0||myMesh->NbElems(RWCae::MeshElemType::Polyhedron)>0) { linearSupport+=nodes*cells; }
        if(needsGeneralIso()&&myMesh->GenericCells().empty()) { support+=nodes*cells; }
        const uint64_t refinement=std::max<uint64_t>(1ULL<<(2*myGeometryOptions.FaceSubdivisionLimit),1ULL<<myGeometryOptions.LineSubdivisionLimit);
        const auto limit=myGeometryOptions.MaxSourcePrimitives;
        const auto curved=support>limit/refinement?limit:support*refinement;
        const auto linear=linearSupport>limit/8?limit:linearSupport*8;
        primitives=curved>=limit-linear?limit:curved+linear;
    }
    return 65536+cells*(4096+components*512)+nodes*(256+components*64)+primitives*4096;
}
void MeshCutViewManager::prepareCache(Entry& entry,const Handle(RWCae::MeshNVecProperty)& field)
{
    if(EstimatedSourceBytes()>mySourceReserved && !myCache.CanReserve(EstimatedSourceBytes()-mySourceReserved,true)) { throw std::length_error("Cut source cache budget exceeded before preparation"); }
    if(!entry.Lease) {
        const auto bytes=EstimatedResultBytes(field);
        if(!myCache.CanReserve(bytes,false)) {
            std::vector<Entry*> old;
            for(auto& e:myEntries) { if(e.second.get()!=&entry && e.second->Lease) { old.push_back(e.second.get()); } }
            std::sort(old.begin(),old.end(),[](const auto* a,const auto* b) { return a->Access<b->Access; });
            for(auto* victim:old) { release(*victim); myCache.Evicted(); if(myCache.CanReserve(bytes,false)) { break; } }
        }
        entry.Lease=myCache.Reserve(bytes,false);
        entry.Reserved=bytes;
    } else {
        const auto bytes=EstimatedResultBytes(field);
        if(bytes>entry.Reserved) {
            auto extra=myCache.Reserve(bytes-entry.Reserved,false);
            entry.Lease=std::shared_ptr<void>(new std::pair<std::shared_ptr<void>,std::shared_ptr<void>>(entry.Lease,extra),[](void* p) { delete static_cast<std::pair<std::shared_ptr<void>,std::shared_ptr<void>>*>(p); });
            entry.Reserved=bytes;
            for(const auto& root:entry.Roots) { if(!root.IsNull()) { root->RetainCacheLease(entry.Lease); } }
            for(const auto& portion:entry.Portions) { if(!portion.IsNull()) { portion->RetainCacheLease(entry.Lease); } }
            if(!entry.TraceRoot.IsNull()) { entry.TraceRoot->RetainCacheLease(entry.Lease); entry.TraceSection->RetainCacheLease(entry.Lease); }
        }
    }
    entry.Access=++myAccess;
}
uint64_t MeshCutViewManager::BeginRequest(const std::shared_ptr<MeshCutCancellation>& token)
{
    CancelRequest(); myCancellation=token; return myRequestGeneration;
}
void MeshCutViewManager::CancelRequest() { if(myCancellation) { myCancellation->Cancel(); } myCancellation.reset(); ++myRequestGeneration; }
void MeshCutViewManager::checkRequest(uint64_t generation) const
{
    if(generation!=myRequestGeneration || (myCancellation&&myCancellation->Cancelled.load())) { throw std::runtime_error("Cut frame cancelled or superseded before publication"); }
}

std::shared_ptr<MeshVtkCutSource> MeshCutViewManager::generalSource()
{
    if (!myGeneralSource) { myGeneralSource = std::make_shared<MeshVtkCutSource>(mySourceEpoch,myGeometryOptions); }
    return myGeneralSource;
}

MeshCutStatistics MeshCutViewManager::Statistics() const
{
    if (myGeneralSource) { return myGeneralSource->Work; }
    MeshCutStatistics result; result.SourceEpoch = mySourceEpoch; return result;
}

bool MeshCutGeometryOptions::IsValid() const
{
    return (int)Mode >= 0 && (int)Mode <= 2 && std::isfinite(AbsoluteTolerance) && AbsoluteTolerance > 0
        && std::isfinite(RelativeTolerance) && RelativeTolerance >= 0
        && FaceSubdivisionLimit >= 0 && FaceSubdivisionLimit <= 10
        && LineSubdivisionLimit >= 0 && LineSubdivisionLimit <= 20
        && MaxSourcePrimitives > 0 && MaxSourcePrimitives <= 100000000;
}

double MeshCutGeometryOptions::Tolerance(double scale) const
{
    return std::max(AbsoluteTolerance,RelativeTolerance*scale);
}

bool MeshCutViewManager::SetGeometryOptions(const MeshCutGeometryOptions& options)
{
    if (!options.IsValid()) { return false; }
    if (options.Mode == myGeometryOptions.Mode && options.AbsoluteTolerance == myGeometryOptions.AbsoluteTolerance
        && options.RelativeTolerance == myGeometryOptions.RelativeTolerance
        && options.FaceSubdivisionLimit == myGeometryOptions.FaceSubdivisionLimit
        && options.LineSubdivisionLimit == myGeometryOptions.LineSubdivisionLimit
        && options.MaxSourcePrimitives == myGeometryOptions.MaxSourcePrimitives) { return true; }
    myGeometryOptions = options;
    InvalidateGeometry();
    return true;
}

bool MeshCutViewManager::SetRecoveryOptions(const MeshCutRecoveryOptions& options)
{
    if (!options.IsValid(myMesh->NbIndexedElems())) { return false; }
    if (options.ThresholdPercent == myRecoveryOptions.ThresholdPercent
        && options.InvariantsBeforeAveraging == myRecoveryOptions.InvariantsBeforeAveraging
        && options.Tensor.Measure == myRecoveryOptions.Tensor.Measure
        && options.Tensor.Layout == myRecoveryOptions.Tensor.Layout
        && options.Tensor.EngineeringShear == myRecoveryOptions.Tensor.EngineeringShear
        && options.Regions == myRecoveryOptions.Regions) { return true; }
    myRecoveryOptions = options; InvalidateFields(); return true;
}

bool MeshCutIsoOptions::IsValid() const
{
    return std::isfinite(AbsoluteTolerance) && AbsoluteTolerance > 0 && std::isfinite(RelativeTolerance) && RelativeTolerance >= 0
        && MinimumDepth >= 1 && MinimumDepth <= SubdivisionLimit && SubdivisionLimit <= 8
        && MaxPrimitives > 0 && MaxPrimitives <= 10000000;
}

bool MeshCutViewManager::SetIsoOptions(const MeshCutIsoOptions& options)
{
    if (!options.IsValid()) { return false; }
    if (options.AbsoluteTolerance == myIsoOptions.AbsoluteTolerance && options.RelativeTolerance == myIsoOptions.RelativeTolerance
        && options.MinimumDepth == myIsoOptions.MinimumDepth && options.SubdivisionLimit == myIsoOptions.SubdivisionLimit
        && options.MaxPrimitives == myIsoOptions.MaxPrimitives) { return true; }
    myIsoOptions = options;
    for (auto& entry : myEntries) {
        if (entry.second->Definition.Shape == MeshCutShape::Isosurface) { release(*entry.second); ++entry.second->Revision; }
    }
    return true;
}
}

#include <ViewCaeGeom/MeshCutMaterial.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <cmath>
#include <stdexcept>

namespace ViewCaeGeom {
bool SameMaterialTopology(const Handle(RWCae::MeshData)& a, const Handle(RWCae::MeshData)& b)
{
    if (a == b) { return true; }
    if (a.IsNull() || b.IsNull() || a->NbNodes() != b->NbNodes() || a->NbIndexedElems() != b->NbIndexedElems()) { return false; }
    std::vector<RWCae::MeshEntityId> left, right;
    for (RWCae::MeshEntityId e = 0; e < a->NbIndexedElems(); ++e) {
        const auto& x = a->IndexedElements()[e]; const auto& y = b->IndexedElements()[e];
        if (x.Type != y.Type) { return false; }
        a->ElementNodes(e,left); b->ElementNodes(e,right); if (left != right) { return false; }
        if (x.Type == RWCae::MeshElemType::Generic) {
            const auto& p = a->GenericCells().at(x.Index); const auto& q = b->GenericCells().at(y.Index);
            if (p.Type != q.Type || p.Faces != q.Faces || p.Degrees != q.Degrees || p.RationalWeights != q.RationalWeights) { return false; }
        }
    }
    return true;
}
const MeshCutResult& MeshCutMaterialCache::Get(unsigned portion)
{
    if (portion > 2) { throw std::invalid_argument("Invalid material portion"); }
    if (!(Mask & (1u << portion))) {
        Cuts->SetVisibility("reference",1u << portion);
        Results[portion] = Cuts->Evaluate("reference");
        Mask |= 1u << portion; ++ReferenceBuilds;
    }
    return Results[portion];
}
Handle(RWCae::MeshData) CopyCutTopology(const Handle(RWCae::MeshData)& input,
    const std::function<Graphic3d_Vec3(RWCae::MeshEntityId)>& position)
{
    Handle(RWCae::MeshData) output = new RWCae::MeshData();
    for (RWCae::MeshEntityId n = 0; n < input->NbNodes(); ++n) { output->AddNodePosition(position(n)); }
    for (const auto& indexed : input->IndexedElements()) {
        if (indexed.Type == RWCae::MeshElemType::Generic) { output->AddGenericCell(input->GenericCells().at(indexed.Index)); }
        else if (!RWCae::MeshAdaptorsIteratorAllElems::Switch(indexed.Type,[&](const auto& a) {
            output->AddElement(a.GetElement(indexed.Index)); return true;
        },input)) { throw std::invalid_argument("Cannot copy material reference topology"); }
    }
    RWCae::MeshAdaptorsIterator2dElems::For([&](const auto& a) {
        for (RWCae::MeshEntityId e = 0; e < a.NbElements(); ++e) { output->AddSurfaceElement(a.GetElement(e)); }
    },input,true);
    return output;
}
MeshGeometryMaterial::MeshGeometryMaterial(const Handle(MeshBvhData)& bvh,
    std::shared_ptr<MeshCutMaterialCache> reference, unsigned portion,
    const Handle(MeshGeometry)& current, MeshCutViewManager& accounting)
    : MeshGeometrySlice(bvh), myReference(std::move(reference)), myPortion(portion), myCurrent(current), myAccounting(accounting)
{}
void MeshGeometryMaterial::generateMeshData()
{
    mySamplingLeases.clear(); myRefined = false; Tolerance = Residual = 0; Depth = 0;
    myElementTraces.clear(); myLineTraces.clear(); myNormals.clear(); myLineNormals.clear();
    try {
    const auto& reference = myReference->Get(myPortion);
    const auto geometry = reference.Portions[myPortion]; const auto input = geometry->GetMeshData();
    myBoundary = reference.BoundaryMesh;
    auto lease = myAccounting.ReserveDisplayBytes((uint64_t)input->NbNodes()*320 + (uint64_t)input->NbIndexedElems()*256);
    const auto source = myCurrent->GetMeshData();
    NodesMap nodes; nodes.ReSize(input->NbNodes()); ElementsMap elements;
    auto mapped = CopyCutTopology(input,[&](RWCae::MeshEntityId n) {
        MeshInterpolatedNode origin;
        if (!geometry->OriginNode(n,origin)) { throw std::runtime_error("Material reference has no FE node weights"); }
        Graphic3d_Vec3d sum(0);
        for (const auto& w : origin.BaseNodes()) {
            if (w.second == 0) { continue; }
            for (const auto* manager : {&myAccounting,myReference->Cuts.get()}) {
                const auto& field = manager->DeformationField();
                if (!field.IsNull() && manager->DeformationScale() != 0) {
                    Graphic3d_Vec3d value; field->GetVector(RWCae::MeshPropertyEntityInfo(w.first),value);
                    for (int a = 0; a < 3; ++a) { if (!std::isfinite(value[a])) { throw std::invalid_argument("Nonfinite material displacement"); } }
                }
            }
            auto p = source->Nodes()[w.first]; myCurrent->ModifyNode(w.first,p);
            for (int a = 0; a < 3; ++a) { if (!std::isfinite(p[a])) { throw std::invalid_argument("Nonfinite material deformation"); } }
            sum += Graphic3d_Vec3d(p)*w.second;
        }
        nodes.Bind(n,std::make_pair(myCurrent.get(),std::move(origin)));
        return Graphic3d_Vec3(sum);
    });
    auto map = [&](const auto& a) {
        for (RWCae::MeshEntityId e = 0; e < a.NbElements(); ++e) {
            const MeshGeomElem key(e,a.GetElementType(),a.IsSurface()); RWCae::MeshEntityId owner;
            if (!geometry->OriginElement(key,owner)) { throw std::runtime_error("Material reference has no FE owner"); }
            const auto& original = source->IndexedElements()[owner];
            elements.Bind(key,std::make_pair(myCurrent.get(),MeshGeomElem(original.Index,original.Type,false)));
        }
    };
    RWCae::MeshAdaptorsIteratorAllElems::For(map,input); RWCae::MeshAdaptorsIterator2dElems::For(map,input,true);
    const auto slice = Handle(MeshGeometrySlice)::DownCast(geometry);
    myLines = slice.IsNull() ? std::vector<MeshSliceLine>() : slice->Lines();
    myMeshData = mapped; myNodesMap.Exchange(nodes); myElementsMap.Exchange(elements);
    if (myPortion == 1) { refineSection(reference); }
    RetainCacheLease(lease);
    ++myReference->MappingBuilds; myReference->MappedNodes += myMeshData->NbNodes();
    } catch (...) {
        mySamplingLeases.clear(); myMeshData.Nullify(); myNodesMap.Clear(); myElementsMap.Clear();
        throw;
    }
}
bool MeshGeometryMaterial::acceptElement(const Handle(RWCae::MeshData)&, RWCae::MeshEntityId e, const MeshGeometry* geometry) const
{
    if (geometry != myCurrent.get()) { return true; }
    const auto& ref = myReference->Get(myPortion);
    return ref.Roots[myPortion]->AcceptElement(e);
}
bool MeshGeometryMaterial::acceptNode(RWCae::MeshEntityId n, const MeshGeometry* geometry, const Graphic3d_Vec3&) const
{
    if (geometry != myCurrent.get()) { return true; }
    const auto& ref = myReference->Get(myPortion); const auto root = ref.Roots[myPortion];
    auto p = root->GetMeshData()->Nodes()[n]; root->ModifyNode(n,p); return root->AcceptNode(n,p);
}
}

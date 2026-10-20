#include <ViewCaeGeom/MeshCutTrace.h>
#include <ViewCaeGeom/MeshCutViewManager.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>
#include <RWCae/MeshNodeVecProperty.h>
#include "MeshCutFieldTools.h"
#include <ViewCaeGeom/MeshVtkCutSource.h>
#include <algorithm>
#include <set>
#include <tuple>
#include <cmath>
#include <stdexcept>
#include <OSD_ThreadPool.hxx>
#include <unordered_map>
#include <array>
#include <NCollection_IncAllocator.hxx>
#include <chrono>

namespace ViewCaeGeom {
namespace {
// Native slice origins already index the root mesh. Avoid repeatedly walking
// that one-edge chain and contending on OCCT Handle reference counts in workers.
bool sectionOrigin(const MeshGeometrySlice& section,const MeshGeometry& source,RWCae::MeshEntityId node,MeshInterpolatedNode& origin)
{
    const auto* direct=section.GetNodesMap().Seek(node);
    if(source.GetParent().IsNull() && direct && direct->first==&source) { origin=direct->second; return true; }
    return section.OriginNode(node,origin);
}
bool sectionElement(const MeshGeometrySlice& section,const MeshGeometry& source,const RWCae::MeshData& mesh,
    const RWCae::MeshData::IndexedElement& element,RWCae::MeshEntityId& original)
{
    const MeshGeomElem key(element.Index,element.Type,false);
    const auto* direct=section.GetElementsMap().Seek(key);
    if(source.GetParent().IsNull() && direct && direct->first==&source && !direct->second.IsSurface) {
        original=mesh.Elements(direct->second.Type).GlobalIndex(direct->second.Index); return true;
    }
    return section.OriginElement(key,original);
}
// Compact faces have <=4 original nodes. Five distinct nonzero supports prove
// an interior section; no allocation/sort is needed to establish this fact.
template<class Origin> bool interiorSupport(const std::vector<RWCae::MeshEntityId>& nodes,const Origin& origin,
    const RWCae::MeshData& mesh,RWCae::MeshEntityId element)
{
    std::array<RWCae::MeshEntityId,5> support; size_t count=0;
    for(auto n:nodes) { for(const auto& w:origin(n).BaseNodes()) {
        if(std::abs(w.second)<=1e-6) { continue; }
        if(std::find(support.begin(),support.begin()+count,w.first)!=support.begin()+count) { continue; }
        support[count++]=w.first; if(count==5) { return true; }
    } }
    const auto& indexed=mesh.IndexedElements()[element];
    if(indexed.Type==RWCae::MeshElemType::Tetrahedron) { return count>3; }
    if(indexed.Type!=RWCae::MeshElemType::Hexahedron || count==0) { return false; }
    // Four supports may span a hex corner, not a face. Test the six original
    // faces without constructing/sorting dynamic face vectors. The same test
    // is used by general Traces(), so actual interfaces still take that path.
    const auto& cells=mesh.Elements(indexed.Type); RWCae::MeshElemEntityOffset begin,end;
    cells.GetElemNodesRange(indexed.Index,&begin,&end); if(end-begin!=8) { return false; }
    static constexpr int faces[6][4]={{0,1,2,3},{4,5,6,7},{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7}};
    for(const auto& face:faces) {
        bool contains=true;
        for(size_t s=0;s<count && contains;++s) {
            bool found=false; for(int n:face) { found|=cells.Nodes()[begin+n]==support[s]; }
            contains=found;
        }
        if(contains) { return false; }
    }
    return true;
}
template<class Work> void sectionWork(int count,const Work& work)
{
    if(count>=2048) { OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4); launcher.Perform(0,count,work); }
    else { for(int i=0;i<count;++i) { work(0,i); } }
}
}
const MeshCutPrimitiveTrace& MeshGeometryTraceSection::PhysicalProvenance(size_t primitive,
    const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance, bool lines) const
{
    return select((lines ? myLineTraces : myElementTraces).at(primitive),options,signedValue,tolerance);
}
bool MeshCutTraceOptions::IsValid(size_t elements) const
{
    return Policy >= MeshCutTracePolicy::Lowest && Policy <= MeshCutTracePolicy::Region
        && (Regions.empty() || Regions.size() == elements)
        && (Policy != MeshCutTracePolicy::Region || Regions.size() == elements);
}
struct MeshCutTraceSource::Data {
    struct Boundary { std::vector<RWCae::MeshEntityId> Nodes; std::vector<RWCae::MeshEntityId> Owners; bool Trusted = false; };
    struct Cell {
        int Dimension = 0;
        Graphic3d_Vec3d Interior = Graphic3d_Vec3d(0);
        MeshInterpolatedNode InteriorWeights;
        std::vector<size_t> Boundaries;
        std::unique_ptr<MeshVtkCell> Owned;
        MeshVtkCell* Geometry = nullptr;
    };
    Handle(RWCae::MeshData) Mesh;
    Handle(MeshGeometry) Source;
    ~Data() {
        // This topology owns a dedicated source chain. Deformation adds owning
        // parent/child handles; dropping only Source leaves that cycle and its
        // source-budget lease alive after every deformation epoch (or failure).
        if (!Source.IsNull()) { Source->RemoveChildren(); }
    }
    std::vector<Cell> Cells;
    std::vector<Boundary> Boundaries;
    std::shared_ptr<MeshVtkCutSource> Prepared;
    bool Compact = false;
    bool ModifyNodes = true;
    std::vector<size_t> NodeOffsets;
    std::vector<RWCae::MeshEntityId> NodeOwners;
    mutable std::map<RWCae::MeshEntityId,Cell> NativeCells;
    std::unordered_map<RWCae::MeshEntityId,Cell> Interiors;
    MeshCutGeometryOptions Options;
    MeshCutTraceCacheStatistics CacheWork;
    Cell NativeCellValue(RWCae::MeshEntityId e) const
    {
        Cell cell; const auto type=Mesh->IndexedElements()[e].Type;
        cell.Dimension=type==RWCae::MeshElemType::Elem0d?0:(int)type>=RWCae::MeshElem3dLowerType?3:2;
        const auto& entry=Mesh->IndexedElements()[e]; const auto& elements=Mesh->Elements(entry.Type);
        RWCae::MeshElemEntityOffset begin,end; elements.GetElemNodesRange(entry.Index,&begin,&end);
        const auto count=end-begin; cell.InteriorWeights.BaseNodes().reserve(count);
        for(auto i=begin;i<end;++i) { const auto n=elements.Nodes()[i]; auto p=Mesh->Nodes()[n]; if(ModifyNodes) { Source->ModifyNode(n,p); } cell.Interior+=Graphic3d_Vec3d(p); cell.InteriorWeights.AddBaseNode(n,1.0/count); }
        if(count) { cell.Interior/=(double)count; }
        return cell;
    }
    const Cell& NativeCell(RWCae::MeshEntityId e) const
    {
        const auto found=NativeCells.find(e); if(found!=NativeCells.end()) { return found->second; }
        return NativeCells.emplace(e,NativeCellValue(e)).first->second;
    }
    std::vector<std::vector<RWCae::MeshEntityId>> NativeFaces(RWCae::MeshEntityId e) const
    {
        std::vector<std::vector<RWCae::MeshEntityId>> faces; const auto& entry=Mesh->IndexedElements()[e];
        const int dim=entry.Type==RWCae::MeshElemType::Elem0d?0:(int)entry.Type>=RWCae::MeshElem3dLowerType?3:2;
        if(dim==3) {
            RWCae::MeshAdaptorsIterator3dElems::Switch(entry.Type,[&](const auto& adaptor) {
                const auto& element=adaptor.GetElement(entry.Index);
                for(RWCae::MeshElemEntityId f=0;f<adaptor.NbFaces(element);++f) { std::vector<RWCae::MeshEntityId> nodes; RWCae::MeshElemType type; adaptor.FaceNodes(element,f,nodes,type); std::sort(nodes.begin(),nodes.end()); faces.push_back(std::move(nodes)); } return true;
            },Mesh);
        } else if(dim==2) {
            std::vector<RWCae::MeshEntityId> nodes; Mesh->ElementNodes(e,nodes);
            for(size_t n=0;n<nodes.size();++n) { auto a=nodes[n],b=nodes[(n+1)%nodes.size()]; faces.push_back({std::min(a,b),std::max(a,b)}); }
        }
        return faces;
    }
    MeshInterpolatedNode NativeOrigin(RWCae::MeshEntityId e,const Graphic3d_Vec3d& p) const
    {
        auto& cell=NativeCells.at(e);
        if(!cell.Owned) { cell.Owned.reset(new MeshVtkCell(Mesh,e,Source.get(),Options.Tolerance((Mesh->BoundingBox().CornerMax().XYZ()-Mesh->BoundingBox().CornerMin().XYZ()).Modulus()),Options,0,false)); }
        return cell.Owned->Origin(p);
    }
};

MeshCutTraceSource::MeshCutTraceSource(const Handle(RWCae::MeshData)& mesh,
    const Handle(MeshGeometry)& source, const MeshCutGeometryOptions& options,
    const std::shared_ptr<MeshVtkCutSource>& prepared) : myData(new Data)
{
    auto& data = *myData; data.Mesh = mesh; data.Source = source; data.Prepared = prepared;
    data.ModifyNodes=source->HasNodeModifiers();
    data.Options=options;
    data.Compact=std::all_of(mesh->IndexedElements().begin(),mesh->IndexedElements().end(),[](const auto& e) {
        using T=RWCae::MeshElemType;
        return e.Type==T::Elem0d || e.Type==T::Triangle || e.Type==T::Quad || e.Type==T::Tetrahedron || e.Type==T::Pyramid || e.Type==T::Prism || e.Type==T::Hexahedron;
    });
    if(data.Compact) {
        // Compact incidence replaces millions of VTK evaluators and global face-map
        // records. Exact shared-face queries and inverse charts remain lazy.
        data.NodeOffsets.resize((size_t)mesh->NbNodes()+1,0); std::vector<RWCae::MeshEntityId> nodes;
        for(RWCae::MeshEntityId e=0;e<mesh->NbIndexedElems();++e) { mesh->ElementNodes(e,nodes); for(auto n:nodes) { ++data.NodeOffsets.at((size_t)n+1); } }
        for(size_t n=1;n<data.NodeOffsets.size();++n) { data.NodeOffsets[n]+=data.NodeOffsets[n-1]; }
        data.NodeOwners.resize(data.NodeOffsets.back()); auto next=data.NodeOffsets;
        for(RWCae::MeshEntityId e=0;e<mesh->NbIndexedElems();++e) { mesh->ElementNodes(e,nodes); for(auto n:nodes) { data.NodeOwners[next[n]++]=e; } }
        return;
    }
    data.Cells.resize(mesh->NbIndexedElems());
    using Key = std::pair<int,std::vector<RWCae::MeshEntityId>>;
    std::map<Key,size_t> keys;
    const double tolerance = options.Tolerance(mesh->BoundingBox().IsVoid() ? 0
        : (mesh->BoundingBox().CornerMax().XYZ()-mesh->BoundingBox().CornerMin().XYZ()).Modulus());
    for (RWCae::MeshEntityId e = 0; e < mesh->NbIndexedElems(); ++e) {
        auto& cell = data.Cells[e]; std::vector<std::vector<RWCae::MeshEntityId>> boundaries;
        if (prepared && prepared->Work.PreparedCells == mesh->NbIndexedElems()) { cell.Geometry = prepared->GetCell(e).Geometry.get(); }
        else {
            cell.Owned.reset(new MeshVtkCell(mesh,e,source.get(),tolerance,options,0,false)); cell.Geometry = cell.Owned.get();
        }
        cell.Dimension = cell.Geometry->Dimension(); cell.Interior = cell.Geometry->InteriorPoint();
        if (cell.Dimension == 0) {
            // Empty/polyvertex cells have no FE interior chart or interfaces.
            std::vector<RWCae::MeshEntityId> nodes; mesh->ElementNodes(e,nodes);
            for (auto n : nodes) { cell.InteriorWeights.AddBaseNode(n,1.0/nodes.size()); }
        } else { cell.InteriorWeights = cell.Geometry->Origin(cell.Interior); }
        boundaries = cell.Geometry->BoundaryNodes();
        for (auto& boundaryNodes : boundaries) {
            std::sort(boundaryNodes.begin(),boundaryNodes.end()); const Key key(cell.Dimension,boundaryNodes);
            auto found = keys.find(key);
            if (found == keys.end()) {
                found = keys.emplace(key,data.Boundaries.size()).first;
                data.Boundaries.push_back({boundaryNodes,{},false});
            }
            data.Boundaries[found->second].Owners.push_back(e); cell.Boundaries.push_back(found->second);
        }
    }
    for (auto& boundary : data.Boundaries) {
        const auto& owners = boundary.Owners;
        if (owners.size() != 2 || owners[0] == owners[1]) { continue; }
        const auto& a = data.Cells[owners[0]]; const auto& b = data.Cells[owners[1]];
        if (a.Dimension == 3) {
            if (prepared && prepared->Work.PreparedCells == mesh->NbIndexedElems()) {
                const auto& faces = prepared->GetCell(owners[0]).Faces;
                for (size_t f = 0; f < a.Geometry->Faces.size(); ++f) {
                    auto nodes = a.Geometry->Faces[f].Nodes; std::sort(nodes.begin(),nodes.end());
                    if (nodes == boundary.Nodes && faces[f].Internal && faces[f].NeighborElement == owners[1]) { boundary.Trusted = true; }
                }
                continue;
            }
            if (boundary.Nodes.size() < 3) { continue; }
            Graphic3d_Vec3d points[3];
            for (int n = 0; n < 3; ++n) { auto p = mesh->Nodes()[boundary.Nodes[n]]; source->ModifyNode(boundary.Nodes[n],p); points[n] = Graphic3d_Vec3d(p); }
            auto normal = Graphic3d_Vec3d::Cross(points[1]-points[0],points[2]-points[0]);
            if (normal.Modulus() <= 1e-15) { continue; } normal /= normal.Modulus();
            boundary.Trusted = (a.Interior-points[0]).Dot(normal)*(b.Interior-points[0]).Dot(normal) < -1e-14;
        } else {
            auto p = mesh->Nodes()[boundary.Nodes.front()]; source->ModifyNode(boundary.Nodes.front(),p);
            auto x = a.Interior-Graphic3d_Vec3d(p), y = b.Interior-Graphic3d_Vec3d(p);
            if (a.Dimension == 2 && boundary.Nodes.size() >= 2) {
                auto q = mesh->Nodes()[boundary.Nodes.back()]; source->ModifyNode(boundary.Nodes.back(),q);
                auto direction = Graphic3d_Vec3d(q)-Graphic3d_Vec3d(p);
                if (direction.Modulus() <= 1e-15) { continue; } direction /= direction.Modulus();
                x -= direction*x.Dot(direction); y -= direction*y.Dot(direction);
            }
            // Opposite support prevents pairing duplicate cells on the same side.
            boundary.Trusted = x.Dot(y) < -1e-14;
        }
    }
}
MeshCutTraceSource::~MeshCutTraceSource() = default;
bool MeshCutTraceSource::IsCompact() const { return myData->Compact; }
MeshCutTraceCacheStatistics MeshCutTraceSource::CacheStatistics() const { return myData->CacheWork; }
void MeshCutTraceSource::PrepareInteriors(const std::vector<RWCae::MeshEntityId>& elements)
{
    auto& data=*myData; if(!data.Compact) { return; }
    std::vector<RWCae::MeshEntityId> missing; missing.reserve(elements.size());
    for(auto e:elements) { if(!data.Interiors.count(e)) { missing.push_back(e); } else { ++data.CacheWork.Hits; } }
    std::sort(missing.begin(),missing.end()); missing.erase(std::unique(missing.begin(),missing.end()),missing.end());
    data.CacheWork.Misses+=missing.size();
    if(data.Interiors.size()+missing.size()>100000) {
        // Dense source IDs permit a compact membership mask. Evict only enough
        // stale cells to meet the limit, not an entire historical working set
        // in one long frame. All current request cells stay pinned through fill.
        std::vector<uint64_t> requested(((size_t)data.Mesh->NbIndexedElems()+63)/64,0); size_t active=0;
        for(auto e:elements) { auto& word=requested.at(e/64); const auto bit=uint64_t(1)<<(e%64); if(!(word&bit)) { ++active; } word|=bit; }
        const size_t limit=std::max<size_t>(100000,active);
        for(auto i=data.Interiors.begin();i!=data.Interiors.end() && data.Interiors.size()+missing.size()>limit;) {
            if(!(requested[i->first/64]&(uint64_t(1)<<(i->first%64)))) { i=data.Interiors.erase(i); ++data.CacheWork.Evictions; }
            else { ++i; }
        }
    }
    data.Interiors.reserve(data.Interiors.size()+missing.size());
    if(!data.ModifyNodes && missing.size()>=2048) {
        std::vector<Data::Cell> cells(missing.size());
        sectionWork((int)missing.size(),[&](int,int i) { cells[i]=data.NativeCellValue(missing[i]); });
        for(size_t i=0;i<missing.size();++i) { data.Interiors.emplace(missing[i],std::move(cells[i])); }
    } else {
        for(auto e:missing) { data.Interiors.emplace(e,data.NativeCellValue(e)); }
    }
}
void MeshCutTraceSource::UpdateInteriorTrace(MeshCutPrimitiveTrace& trace,RWCae::MeshEntityId element,
    const std::vector<RWCae::MeshEntityId>& nodes,const MeshGeometry::NodesMap& origins) const
{
    const auto& cell=myData->Interiors.at(element); trace.Element=element; trace.Ambiguous=false;
    trace.Interior=cell.Interior; trace.InteriorWeights=cell.InteriorWeights; trace.Weights.Assign(nodes,origins);
}

std::vector<MeshCutPrimitiveTrace> MeshCutTraceSource::Traces(RWCae::MeshEntityId element,
    const std::vector<MeshInterpolatedNode>& weights, const std::vector<Graphic3d_Vec3d>& positions,
    const std::vector<RWCae::MeshEntityId>* sampleNodes) const
{
    const auto& data = *myData;
    Data::Cell local;
    const auto cached=data.Interiors.find(element);
    if(data.Compact && cached==data.Interiors.end()) { local=data.NativeCellValue(element); }
    const auto& cell = data.Compact ? cached==data.Interiors.end()?local:cached->second : data.Cells.at(element);
    if (weights.size() != positions.size()) { throw std::invalid_argument("Trace sample size mismatch"); }
    std::vector<MeshCutPrimitiveTrace> result(1); result[0].Element = element; result[0].Interior = cell.Interior;
    result[0].InteriorWeights = cell.InteriorWeights;
    result[0].Weights.reserve(weights.size());
    for (size_t n = 0; n < weights.size(); ++n) { result[0].Weights.emplace(sampleNodes ? sampleNodes->at(n) : (RWCae::MeshEntityId)n,weights[n]); }
    std::vector<RWCae::MeshEntityId> support; support.reserve(16);
    for (const auto& weight : weights) {
        for (const auto& node : weight.BaseNodes()) { if (std::abs(node.second) > 1e-6) { support.push_back(node.first); } }
    }
    std::sort(support.begin(),support.end()); support.erase(std::unique(support.begin(),support.end()),support.end());
    if (support.empty()) { return result; }
    if(data.Compact) {
        if(support.size()>4) { return result; }
        if(data.NativeCells.size()>=8192) { data.NativeCells.clear(); }
        for(const auto& face:data.NativeFaces(element)) {
            if(!std::includes(face.begin(),face.end(),support.begin(),support.end())) { continue; }
            std::vector<RWCae::MeshEntityId> owners={element};
            for(size_t i=data.NodeOffsets[face.front()];i<data.NodeOffsets[(size_t)face.front()+1];++i) {
                const auto other=data.NodeOwners[i]; const auto type=data.Mesh->IndexedElements()[other].Type;
                const int dim=type==RWCae::MeshElemType::Elem0d?0:(int)type>=RWCae::MeshElem3dLowerType?3:2;
                if(other==element || dim!=cell.Dimension) { continue; }
                const auto faces=data.NativeFaces(other); if(std::find(faces.begin(),faces.end(),face)!=faces.end()) { owners.push_back(other); }
            }
            if(owners.size()==1) { continue; }
            if(owners.size()!=2) { result[0].Ambiguous=true; continue; }
            const auto neighbor=owners[1]; const auto& adjacent=data.NativeCell(neighbor);
            auto p=data.Mesh->Nodes()[face.front()]; data.Source->ModifyNode(face.front(),p); const auto origin=Graphic3d_Vec3d(p);
            auto x=cell.Interior-origin,y=adjacent.Interior-origin; bool trusted=false;
            if(cell.Dimension==3 && face.size()>=3) {
                auto b=data.Mesh->Nodes()[face[1]],c=data.Mesh->Nodes()[face[2]]; data.Source->ModifyNode(face[1],b); data.Source->ModifyNode(face[2],c);
                auto normal=Graphic3d_Vec3d::Cross(Graphic3d_Vec3d(b)-origin,Graphic3d_Vec3d(c)-origin);
                if(normal.Modulus()>1e-15) { normal/=normal.Modulus(); trusted=x.Dot(normal)*y.Dot(normal)<-1e-14; }
            } else if(cell.Dimension==2 && face.size()==2) {
                auto b=data.Mesh->Nodes()[face[1]]; data.Source->ModifyNode(face[1],b); auto direction=Graphic3d_Vec3d(b)-origin;
                if(direction.Modulus()>1e-15) { direction/=direction.Modulus(); x-=direction*x.Dot(direction); y-=direction*y.Dot(direction); trusted=x.Dot(y)<-1e-14; }
            }
            if(!trusted) { result[0].Ambiguous=true; continue; }
            MeshCutPrimitiveTrace other; other.Element=neighbor; other.Interior=adjacent.Interior; other.InteriorWeights=adjacent.InteriorWeights;
            other.Weights.reserve(weights.size());
            for(size_t n=0;n<weights.size();++n) { other.Weights.emplace(sampleNodes ? sampleNodes->at(n) : (RWCae::MeshEntityId)n,data.NativeOrigin(neighbor,positions[n])); }
            result.push_back(std::move(other)); break;
        }
        std::sort(result.begin(),result.end(),[](const auto& a,const auto& b) { return a.Element<b.Element; }); return result;
    }
    for (auto id : cell.Boundaries) {
        const auto& boundary = data.Boundaries[id];
        if (!std::includes(boundary.Nodes.begin(),boundary.Nodes.end(),support.begin(),support.end()) || boundary.Owners.size() <= 1) { continue; }
        if (!boundary.Trusted) { result[0].Ambiguous = true; continue; }
        const auto neighbor = boundary.Owners[boundary.Owners[0] == element ? 1 : 0];
        MeshCutPrimitiveTrace other; other.Element = neighbor; other.Interior = data.Cells[neighbor].Interior;
        other.Weights.reserve(weights.size());
        other.InteriorWeights = data.Cells[neighbor].InteriorWeights;
        for (size_t n = 0; n < weights.size(); ++n) {
            other.Weights.emplace(sampleNodes ? sampleNodes->at(n) : (RWCae::MeshEntityId)n,data.Cells[neighbor].Geometry->Origin(positions[n]));
        }
        result.push_back(std::move(other)); break;
    }
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b) { return a.Element < b.Element; });
    return result;
}

MeshGeometryTraceSection::MeshGeometryTraceSection(const Handle(MeshBvhData)& bvh,
    const Handle(MeshGeometrySlice)& section, const Handle(MeshGeometry)& source,
    const std::shared_ptr<MeshCutTraceSource>& topology) : MeshGeometrySlice(bvh), mySource(source)
{
    auto phase=std::chrono::steady_clock::now();
    const auto elapsed=[&]() { const auto now=std::chrono::steady_clock::now(); const double ms=std::chrono::duration<double,std::milli>(now-phase).count(); phase=now; return ms; };
    const auto input = section->GetMeshData(); myInput = input; myMeshData = input; myIsUpdated = true;
    const auto originalMesh=source->GetMeshData();
    if(input->NbIndexedElems()>=2048 && section->SlicePlanes().Size()==1) { myElementsMap.Clear(new NCollection_IncAllocator()); myElementsMap.ReSize(input->NbIndexedElems()); }
    bool deduplicated=false;
    std::vector<std::vector<MeshCutPrimitiveTrace>> interiorTraces;
    auto& origins=myOriginScratch; origins.resize(input->NbNodes());
    sectionWork(input->NbNodes(),[&](int,int n) { if(!sectionOrigin(*section,*source,n,origins[n])) { throw std::runtime_error("Section node has no original FE provenance"); } });
    myNodesMap.ReSize(input->NbNodes());
    for(RWCae::MeshEntityId n=0;n<input->NbNodes();++n) { myNodesMap.Bind(n,std::make_pair(source.get(),std::move(origins[n]))); }
    myTraceWork.OriginsMs=elapsed();
    if(input->NbIndexedElems()>=2048 && section->Lines().empty() && topology->IsCompact()) {
        struct Primitive { RWCae::MeshEntityId Element; std::vector<RWCae::MeshEntityId> Nodes; };
        std::vector<Primitive> primitives(input->NbIndexedElems()); std::vector<unsigned char> interiors(primitives.size());
        sectionWork(input->NbIndexedElems(),[&](int,int e) {
            const auto& indexed=input->IndexedElements()[e]; auto& primitive=primitives[e];
            if(!sectionElement(*section,*source,*originalMesh,indexed,primitive.Element)) { throw std::runtime_error("Missing section element provenance"); }
            input->ElementNodes(e,primitive.Nodes);
            interiors[e]=interiorSupport(primitive.Nodes,[&](auto n)->const MeshInterpolatedNode& { return myNodesMap.Find(n).second; },*originalMesh,primitive.Element);
        });
        const bool interiorOnly=std::all_of(interiors.begin(),interiors.end(),[](auto value) { return value!=0; });
        myTraceWork.ClassifyMs=elapsed();
        if(interiorOnly) {
            // Compact source faces have at most four nodes. These primitives
            // cannot lie on an interface; pure FE work is independent and keeps
            // stable input order. AIS publication stays on the Viewer thread.
            myElementTraces.resize(primitives.size());
            std::vector<RWCae::MeshEntityId> elements; elements.reserve(primitives.size()); for(const auto& p:primitives) { elements.push_back(p.Element); } topology->PrepareInteriors(elements);
            myTraceWork.PrepareMs=elapsed();
            for(RWCae::MeshEntityId e=0;e<input->NbIndexedElems();++e) { const auto& indexed=input->IndexedElements()[e]; const auto& original=source->GetMeshData()->IndexedElements()[primitives[e].Element]; myElementsMap.Bind(MeshGeomElem(indexed.Index,indexed.Type,false),std::make_pair(source.get(),MeshGeomElem(original.Index,original.Type,false))); }
            myTraceWork.MapsMs=elapsed();
            const auto work=[&](int,int e) { const auto& primitive=primitives[e]; myElementTraces[e].resize(1);
                topology->UpdateInteriorTrace(myElementTraces[e][0],primitive.Element,primitive.Nodes,myNodesMap);
            };
            OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4); launcher.Perform(0,(int)primitives.size(),work);
            myTraceWork.FillMs=elapsed();
            return;
        }
        // A handful of near-node/interface primitives must not force every
        // interior primitive back through the serial general trace path.
        std::vector<RWCae::MeshEntityId> elements;
        for(size_t e=0;e<primitives.size();++e) { if(interiors[e]) { elements.push_back(primitives[e].Element); } }
        topology->PrepareInteriors(elements); interiorTraces.resize(primitives.size());
        myTraceWork.PrepareMs=elapsed();
        sectionWork((int)primitives.size(),[&](int,int e) {
            if(!interiors[e]) { return; }
            interiorTraces[e].resize(1);
            topology->UpdateInteriorTrace(interiorTraces[e][0],primitives[e].Element,primitives[e].Nodes,myNodesMap);
        });
        myTraceWork.FillMs=elapsed();
    }
    using VertexKey = std::vector<std::pair<RWCae::MeshEntityId,int64_t>>;
    using Key = std::tuple<RWCae::MeshEntityId,RWCae::MeshEntityId,std::vector<VertexKey>>;
    std::set<Key> faces, lines;
    auto records = [&](RWCae::MeshEntityId element,const std::vector<RWCae::MeshEntityId>& nodes,std::set<Key>& seen) {
        std::vector<MeshInterpolatedNode> weights; std::vector<Graphic3d_Vec3d> positions; weights.reserve(nodes.size()); positions.reserve(nodes.size());
        for (auto n : nodes) { weights.push_back(myNodesMap.Find(n).second); positions.emplace_back(input->Nodes()[n]); }
        auto traces = topology->Traces(element,weights,positions,&nodes);
        if (traces.size() == 2) {
            std::vector<VertexKey> vertices;
            for (size_t n = 0; n < nodes.size(); ++n) {
                VertexKey vertex;
                for (const auto& weight : traces[0].Weights.at(nodes[n]).BaseNodes()) {
                    if (std::abs(weight.second) > 1e-6) { vertex.push_back({weight.first,(int64_t)std::llround(weight.second*1e7)}); }
                }
                std::sort(vertex.begin(),vertex.end()); vertices.push_back(std::move(vertex));
            }
            std::sort(vertices.begin(),vertices.end());
            if (!seen.emplace(traces[0].Element,traces[1].Element,vertices).second) { traces.clear(); return traces; }
        }
        return traces;
    };
    for (RWCae::MeshEntityId e = 0; e < input->NbIndexedElems(); ++e) {
        const auto& indexed = input->IndexedElements()[e]; RWCae::MeshEntityId origin;
        if (!sectionElement(*section,*source,*originalMesh,indexed,origin)) { throw std::runtime_error("Section element has no original provenance"); }
        std::vector<RWCae::MeshEntityId> nodes; input->ElementNodes(e,nodes);
        auto traces = !interiorTraces.empty() && !interiorTraces[e].empty()
            ? std::move(interiorTraces[e]) : records(origin,nodes,faces);
        if (traces.empty()) {
            if(!deduplicated) { deduplicated=true; myMeshData=new RWCae::MeshData(); for(const auto& p:input->Nodes()) { myMeshData->AddNodePosition(p); }
                for(RWCae::MeshEntityId previous=0;previous<e;++previous) { const auto& old=input->IndexedElements()[previous]; std::vector<RWCae::MeshEntityId> oldNodes; input->ElementNodes(previous,oldNodes);
                    if(old.Type==RWCae::MeshElemType::Elem0d) { myMeshData->AddElement(RWCae::MeshElement0d(oldNodes.data())); }
                    else if(oldNodes.size()==3) { myMeshData->AddElement(RWCae::MeshElementTriangle(oldNodes.data())); }
                    else if(oldNodes.size()==4) { myMeshData->AddElement(RWCae::MeshElementQuad(oldNodes.data())); }
                    else { myMeshData->AddElement(RWCae::MeshElementPolygon(oldNodes.data(),(RWCae::MeshElemEntityId)oldNodes.size())); }
                }
            } continue;
        }
        if (deduplicated && indexed.Type == RWCae::MeshElemType::Elem0d) { myMeshData->AddElement(RWCae::MeshElement0d(nodes.data())); }
        else if (deduplicated && nodes.size() == 3) { myMeshData->AddElement(RWCae::MeshElementTriangle(nodes.data())); }
        else if (deduplicated && nodes.size() == 4) { myMeshData->AddElement(RWCae::MeshElementQuad(nodes.data())); }
        else if(deduplicated) { myMeshData->AddElement(RWCae::MeshElementPolygon(nodes.data(),(RWCae::MeshElemEntityId)nodes.size())); }
        const auto& added = deduplicated ? myMeshData->IndexedElements()[myMeshData->NbIndexedElems()-1] : indexed;
        const auto& original = source->GetMeshData()->IndexedElements()[traces[0].Element];
        myElementsMap.Bind(MeshGeomElem(added.Index,added.Type,false),std::make_pair(source.get(),MeshGeomElem(original.Index,original.Type,false)));
        myElementTraces.push_back(std::move(traces));
    }
    for (const auto& line : section->Lines()) {
        auto traces = records(line.SourceElement,{line.Nodes[0],line.Nodes[1]},lines); if (traces.empty()) { continue; }
        myLines.push_back({line.Nodes,traces[0].Element}); myLineTraces.push_back(std::move(traces));
    }
    myTraceWork.GeneralMs=elapsed();
}

void MeshGeometryTraceSection::Refresh(const Handle(MeshGeometrySlice)& section,
    const std::shared_ptr<MeshCutTraceSource>& topology)
{
    myTraceWork={}; auto phase=std::chrono::steady_clock::now();
    const auto elapsed=[&]() { const auto now=std::chrono::steady_clock::now(); const double ms=std::chrono::duration<double,std::milli>(now-phase).count(); phase=now; return ms; };
    const auto input = section->GetMeshData();
    const auto originalMesh=mySource->GetMeshData();
    bool unchanged = input == myInput && input != myMeshData && input->NbNodes() == myMeshData->NbNodes();
    for (RWCae::MeshEntityId n = 0; unchanged && n < input->NbNodes(); ++n) {
        unchanged = input->Nodes()[n] == myMeshData->Nodes()[n];
    }
    if (unchanged) { return; }
    if(input!=myInput && input->NbIndexedElems()>=2048 && section->Lines().empty() && topology->IsCompact()
        && myLineTraces.empty() && std::all_of(myElementTraces.begin(),myElementTraces.end(),[](const auto& t) { return t.size()==1 && !t[0].Ambiguous; })) {
        struct Primitive { RWCae::MeshEntityId Element; std::vector<RWCae::MeshEntityId> Nodes; };
        std::vector<Primitive> primitives(input->NbIndexedElems()); auto& origins=myOriginScratch; origins.resize(input->NbNodes()); std::vector<unsigned char> interiors(primitives.size());
        sectionWork(input->NbNodes(),[&](int,int n) { if(!sectionOrigin(*section,*mySource,n,origins[n])) { throw std::runtime_error("Missing updated section provenance"); } });
        myTraceWork.OriginsMs=elapsed();
        sectionWork(input->NbIndexedElems(),[&](int,int e) {
            auto& primitive=primitives[e]; const auto& indexed=input->IndexedElements()[e]; if(!sectionElement(*section,*mySource,*originalMesh,indexed,primitive.Element)) { throw std::runtime_error("Missing updated section element provenance"); }
            input->ElementNodes(e,primitive.Nodes);
            interiors[e]=interiorSupport(primitive.Nodes,[&](auto n)->const MeshInterpolatedNode& { return origins[n]; },*originalMesh,primitive.Element);
        });
        const bool interiorOnly=std::all_of(interiors.begin(),interiors.end(),[](auto value) { return value!=0; });
        myTraceWork.ClassifyMs=elapsed();
        if(interiorOnly) {
            std::vector<RWCae::MeshEntityId> elements; elements.reserve(primitives.size()); for(const auto& p:primitives) { elements.push_back(p.Element); } topology->PrepareInteriors(elements);
            myTraceWork.PrepareMs=elapsed();
            const auto oldCount=myMeshData->NbNodes();
            for(RWCae::MeshEntityId n=0;n<input->NbNodes();++n) { auto* origin=myNodesMap.ChangeSeek(n); if(origin) { origin->second=origins[n]; } else { myNodesMap.Bind(n,std::make_pair(mySource.get(),origins[n])); } }
            for(RWCae::MeshEntityId n=input->NbNodes();n<oldCount;++n) { myNodesMap.UnBind(n); }
            myElementsMap.Clear(new NCollection_IncAllocator()); myElementsMap.ReSize((int)primitives.size());
            for(RWCae::MeshEntityId e=0;e<input->NbIndexedElems();++e) { const auto& indexed=input->IndexedElements()[e]; const auto& original=mySource->GetMeshData()->IndexedElements()[primitives[e].Element]; myElementsMap.Bind(MeshGeomElem(indexed.Index,indexed.Type,false),std::make_pair(mySource.get(),MeshGeomElem(original.Index,original.Type,false))); }
            myTraceWork.MapsMs=elapsed();
            myElementTraces.resize(primitives.size());
            const auto work=[&](int,int e) { myElementTraces[e].resize(1); topology->UpdateInteriorTrace(myElementTraces[e][0],primitives[e].Element,primitives[e].Nodes,myNodesMap); };
            OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4); launcher.Perform(0,(int)primitives.size(),work);
            myTraceWork.FillMs=elapsed();
            myInput=input; myMeshData=input; return;
        }
    }
    if(input==myInput && input->NbNodes()==myMeshData->NbNodes()
        && std::all_of(myElementTraces.begin(),myElementTraces.end(),[](const auto& t) { return t.size()==1 && !t[0].Ambiguous; })
        && std::all_of(myLineTraces.begin(),myLineTraces.end(),[](const auto& t) { return t.size()==1 && !t[0].Ambiguous; })) {
        // Only the conservative native translation cache mutates the same input
        // mesh. It has already validated edge partitions and unchanged topology.
        for(RWCae::MeshEntityId n=0;n<input->NbNodes();++n) {
            MeshInterpolatedNode origin; if(!sectionOrigin(*section,*mySource,n,origin)) { throw std::runtime_error("Missing refreshed section FE provenance"); }
            myNodesMap.ChangeFind(n).second=origin; myMeshData->SetNodePosition(n,input->Nodes()[n]);
        }
        auto update=[&](auto& records) { sectionWork((int)records.size(),[&](int,int e) { for(auto& weight:records[e].front().Weights) { weight.second=myNodesMap.Find(weight.first).second; } }); };
        myTraceWork.OriginsMs=elapsed();
        update(myElementTraces); update(myLineTraces); myTraceWork.FillMs=elapsed(); return;
    }
    MeshGeometryTraceSection next(myBvhData,section,mySource,topology);
    myTraceWork=next.myTraceWork;
    bool reuse = input == myInput && myMeshData->NbNodes() == next.myMeshData->NbNodes()
        && myMeshData->NbIndexedElems() == next.myMeshData->NbIndexedElems() && myLines.size() == next.myLines.size();
    for (RWCae::MeshEntityId e = 0; reuse && e < myMeshData->NbIndexedElems(); ++e) {
        std::vector<RWCae::MeshEntityId> a,b; myMeshData->ElementNodes(e,a); next.myMeshData->ElementNodes(e,b);
        reuse = a == b && myMeshData->IndexedElements()[e].Type == next.myMeshData->IndexedElements()[e].Type;
    }
    for (size_t l = 0; reuse && l < myLines.size(); ++l) { reuse = myLines[l].Nodes == next.myLines[l].Nodes; }
    if (reuse) {
        for (RWCae::MeshEntityId n = 0; n < input->NbNodes(); ++n) { myMeshData->SetNodePosition(n,next.myMeshData->Nodes()[n]); }
    } else { myMeshData = next.myMeshData; }
    myInput = input; myNodesMap.Exchange(next.myNodesMap); myElementsMap.Exchange(next.myElementsMap);
    myLines = std::move(next.myLines); myElementTraces = std::move(next.myElementTraces); myLineTraces = std::move(next.myLineTraces);
}

Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::mapTraces(
    const Handle(RWCae::MeshNVecProperty)& field, bool lines, const MeshCutTraceOptions& options,
    const SignedValue& signedValue, double tolerance, const MeshGeomElem* element,
    const std::vector<RWCae::MeshEntityId>* nodes) const
{
    if (field.IsNull() || (lines && myLineTraces.empty()) || myMeshData->NbNodes() == 0) { return nullptr; }
    const auto& records = lines ? myLineTraces : myElementTraces;
    if (!field->IsNodal() && records.empty()) { return nullptr; }
    Handle(RWCae::MeshNVecProperty) result = new RWCae::MeshNVecProperty(field->Name(),field->NbComponents(),field->MapType(),field->Type());
    if (field->HasNodeVecComponents()) { result->Init<RWCae::MeshNodeVecProperty>(); }
    else { result->Init<RWCae::MeshScalarProperty>(); }
    RWCae::MeshEntityId selected = 0;
    if (element && !field->IsNodal()) {
        if (element->IsSurface || myMeshData->NbElems(element->Type) <= element->Index) { return nullptr; }
        selected = myMeshData->Elements(element->Type).GlobalIndex(element->Index);
    }
    const int count = field->IsNodal() ? (nodes ? (int)nodes->size() : (int)myMeshData->NbNodes()) : element ? 1 : (int)records.size();
    std::vector<const MeshInterpolatedNode*> nodalWeights;
    if(field->IsNodal()) {
        nodalWeights.resize(count);
        for(int n=0;n<count;++n) { nodalWeights[n]=&myNodesMap.Find(nodes?nodes->at(n):(RWCae::MeshEntityId)n).second; }
        auto selectedWeights=[&](const auto& traces) {
            for(const auto& record:traces) {
                const auto& trace=select(record,options,signedValue,tolerance);
                for(const auto& sample:trace.Weights) {
                    if(!nodes) { nodalWeights.at(sample.first)=&sample.second; }
                    else { const auto found=std::find(nodes->begin(),nodes->end(),sample.first); if(found!=nodes->end()) { nodalWeights[found-nodes->begin()]=&sample.second; } }
                }
            }
        };
        selectedWeights(records); if(!lines) { selectedWeights(myLineTraces); }
    }
    for (int c = 0; c < field->NbComponents(); ++c) {
        const auto from = field->GetComponent(c); const auto to = result->GetComponent(c);
        if (field->HasNodeVecComponents()) { Handle(RWCae::MeshNodeVecProperty)::DownCast(to)->Init(count,0,false,false); }
        else { Handle(RWCae::MeshScalarProperty)::DownCast(to)->Init(count); }
        auto value = [&](const MeshCutPrimitiveTrace& trace,const MeshInterpolatedNode& weights) {
            if (!field->IsNodal() && !field->HasNodeVecComponents()) { return from->Value(RWCae::MeshPropertyEntityInfo(trace.Element)); }
            double sum = 0;
            for (const auto& w : weights.BaseNodes()) {
                if (w.second == 0) { continue; }
                sum += w.second*from->Value(RWCae::MeshPropertyEntityInfo(field->IsNodal() ? w.first : trace.Element,
                    field->HasNodeVecComponents() ? (int)w.first : -1));
            }
            return sum;
        };
        if (field->IsNodal()) {
            const auto scalar=Handle(RWCae::MeshScalarProperty)::DownCast(to);
            if(count>=4096 && !scalar.IsNull() && !Handle(RWCae::MeshScalarProperty)::DownCast(from).IsNull()) {
                // Initialized scalar slots are disjoint. Workers do not mutate
                // the shared value range; each block reduces its own range.
                const int blockSize=1024,blocks=(count+blockSize-1)/blockSize;
                std::vector<Bnd_Range> ranges(blocks);
                OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4);
                launcher.Perform(0,blocks,[&](int,int block) {
                    auto& range=ranges[block];
                    for(int n=block*blockSize;n<std::min(count,(block+1)*blockSize);++n) {
                        const double v=value(MeshCutPrimitiveTrace(),*nodalWeights[n]);
                        scalar->SetValue(RWCae::MeshPropertyEntityInfo(n),v,false);
                        if(!std::isnan(v)) { range.Add(v); }
                    }
                });
                Bnd_Range range; for(const auto& block:ranges) { range.Add(block); } scalar->SetValueRange(range);
            } else {
                for(int n=0;n<count;++n) { to->SetValue(RWCae::MeshPropertyEntityInfo(n),value(MeshCutPrimitiveTrace(),*nodalWeights[n])); }
            }
        }
        auto apply = [&](const auto& traces) {
            for (size_t e = 0; e < traces.size(); ++e) {
                if (element && !field->IsNodal() && e != selected) { continue; }
                const auto& trace = select(traces[e],options,signedValue,tolerance);
                for (const auto& sample : trace.Weights) {
                    RWCae::MeshEntityId target = sample.first;
                    if (field->IsNodal() && nodes) {
                        const auto found = std::find(nodes->begin(),nodes->end(),sample.first); if (found == nodes->end()) { continue; }
                        target = (RWCae::MeshEntityId)(found-nodes->begin());
                    }
                    to->SetValue(RWCae::MeshPropertyEntityInfo(field->IsNodal() ? target : element ? 0 : (RWCae::MeshEntityId)e,
                        field->HasNodeVecComponents() ? (int)sample.first : -1),value(trace,sample.second));
                }
            }
        };
        if(!field->IsNodal()) { apply(records); }
        CutFieldTools::CopyMetadata(from,to);
    }
    result->SetDescription(field->Description()); result->SetTime(field->Time(),field->TimeStep());
    result->SetTimeStepDescription(field->TimeStepDescription()); result->SetTimeDisplayUnits(field->TimeDisplayUnits());
    updateGenPropComponents(field,result); return result;
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)& field, const MeshGeomElem* element, const std::vector<RWCae::MeshEntityId>* nodes) const
{
    return mapTraces(field,false,myDisplayOptions,mySignedValue,mySideTolerance,element,nodes);
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::CalculateLineProperty(const Handle(RWCae::MeshNVecProperty)& field) const
{
    return mapTraces(field,true,myDisplayOptions,mySignedValue,mySideTolerance);
}

const MeshCutPrimitiveTrace& MeshGeometryTraceSection::select(const std::vector<MeshCutPrimitiveTrace>& traces,
    const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance) const
{
    if (traces.empty()) { throw std::invalid_argument("Section primitive has no trace"); }
    if (options.Policy == MeshCutTracePolicy::Lowest) { return traces.front(); }
    for (const auto& trace : traces) {
        if (trace.Ambiguous) { throw std::invalid_argument("Trace selection requires a trusted two-sided interface"); }
    }
    if (traces.size() == 1) { return traces.front(); }
    const MeshCutPrimitiveTrace* chosen = nullptr;
    for (const auto& trace : traces) {
        bool matches = false;
        if (options.Policy == MeshCutTracePolicy::Region) { matches = options.Regions.at(trace.Element) == options.Region; }
        else {
            const double value = signedValue(trace);
            if (!std::isfinite(value) || std::abs(value) <= tolerance) { throw std::invalid_argument("Interface side is ambiguous at the source interior"); }
            matches = options.Policy == MeshCutTracePolicy::Negative ? value < 0 : value > 0;
        }
        if (matches) {
            if (chosen) { throw std::invalid_argument("Trace policy matches more than one interface source"); }
            chosen = &trace;
        }
    }
    if (!chosen) { throw std::invalid_argument("Trace policy matches no interface source"); }
    return *chosen;
}
void MeshGeometryTraceSection::SetDisplaySelection(const MeshCutTraceOptions& options,
    SignedValue signedValue, double tolerance)
{
    for (const auto& traces : myElementTraces) { select(traces,options,signedValue,tolerance); }
    for (const auto& traces : myLineTraces) { select(traces,options,signedValue,tolerance); }
    myDisplayOptions = options; mySignedValue = std::move(signedValue); mySideTolerance = tolerance;
}
Handle(RWCae::MeshNVecProperty) MeshGeometryTraceSection::MapPhysical(const Handle(RWCae::MeshNVecProperty)& field,
    const MeshCutTraceOptions& options, const SignedValue& signedValue, double tolerance, bool lines) const
{
    for (const auto& traces : lines ? myLineTraces : myElementTraces) { select(traces,options,signedValue,tolerance); }
    return mapTraces(field,lines,options,signedValue,tolerance);
}
}

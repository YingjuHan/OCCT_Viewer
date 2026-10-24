#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <ViewCaeGeom/MeshGeometryTools.h>
#include <ViewCaeGeom/MeshCutCache.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>

#include <BVH_Traverse.hxx>
#include <NCollection_Map.hxx>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>
#include <chrono>
#include <cstdlib>
#include <OSD_ThreadPool.hxx>
#include <NCollection_IncAllocator.hxx>

namespace ViewCaeGeom {
struct MeshLinearSliceCache {
    struct Pattern {
        RWCae::MeshEntityId Element=std::numeric_limits<RWCae::MeshEntityId>::max();
        uint16_t Mask=0; unsigned char Count=0;
        RWCae::MeshElemType Type=RWCae::MeshElemType::Tetrahedron;
        std::array<unsigned char,12> Edges;
    };
    Handle(MeshBvhData::MeshBvhBoxSet) Source;
    static constexpr size_t Capacity=65536;
    std::vector<Pattern> Cells=std::vector<Pattern>(Capacity);
    static size_t Index(RWCae::MeshEntityId element) { return (uint64_t(element)*0x9e3779b97f4a7c15ULL)>>48; }
};
static_assert(sizeof(MeshLinearSliceCache::Pattern)<=32,"Update the native topology cache budget");
}

namespace {
static constexpr double CONFUSION = 1e-5;
static constexpr int    BVH_DIM   = 3;

static constexpr const char* NAME_PLANES = "planes";
static constexpr const char* NAME_X      = "x";
static constexpr const char* NAME_Y      = "y";
static constexpr const char* NAME_Z      = "z";
static constexpr const char* NAME_W      = "w";

using NodesMap           = ViewCaeGeom::MeshGeometry::NodesMap;
using ElementsMap        = ViewCaeGeom::MeshGeometry::ElementsMap;
using TypedMapOfContours = ViewCaeGeom::MeshGeometryTools::TypedMapOfContours;
using AddedNodeMap       = ViewCaeGeom::MeshGeometryTools::AddedNodeMap;

//! Selector for overlapped with slice plane mesh elements.
struct MeshSliceSelector : public ViewCaeGeom::MeshHBvhBoxSetTraverse<ViewCaeGeom::MeshBvhData::MeshBvhBoxSet> {
    enum class FallbackReason { Unsupported, NearPlane, Warped, Degenerate, NearEndpoint };
    struct Vertex { RWCae::MeshEntityId A,B,Node; double T,Angle; Graphic3d_Vec3d Point; int Edge=0; };
    struct LinearInput { RWCae::MeshEntityId Ids[8]; Graphic3d_Vec3d Points[8]; RWCae::MeshElemType Type; int Count=0; bool Valid=false; };
    struct LinearSection { std::array<Vertex,12> Vertices; size_t Count=0; bool Valid=false,Reused=false; uint16_t Mask=0; FallbackReason Reason=FallbackReason::Unsupported; };
public:
    //! Constructor
    explicit MeshSliceSelector(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                               const Handle(Graphic3d_ClipPlane)&        theSlicePlane,
                               const Handle(RWCae::MeshData)&          theSliceMeshData,
                               NodesMap*                                 theGenNodes,
                               ElementsMap*                              theGenElems,
                               std::vector<ViewCaeGeom::MeshSliceLine>*   theLines,
                               const std::shared_ptr<ViewCaeGeom::MeshCutCancellation>& cancellation,
                               const std::shared_ptr<const std::vector<Graphic3d_Vec3>>& positions=nullptr,
                               const std::shared_ptr<ViewCaeGeom::MeshLinearSliceCache>& topology=nullptr)
        : myMeshGeom(theMeshGeom),
          mySlicePlane(theSlicePlane),
          mySliceMeshData(theSliceMeshData),
          myGenNodes(theGenNodes),
          myGenElems(theGenElems),
          myLines(theLines),myCancellation(cancellation),myPreparedPositions(positions),myTopology(topology)
    {
        myEquation=mySlicePlane->GetEquation();
        myModifyNodes=!myPreparedPositions && myMeshGeom->HasNodeModifiers();
        myTolerance=CONFUSION*std::sqrt(myEquation.x()*myEquation.x()+myEquation.y()*myEquation.y()+myEquation.z()*myEquation.z());
        const int hint=(int)std::min<size_t>(myMeshGeom->GetMeshData()->NbNodes(),32768);
        myAddedNodes.ReSize(hint); myAffineEdges.reserve(hint);
        myGenNodes->ReSize(myGenNodes->Extent()+hint); myGenElems->ReSize(myGenElems->Extent()+hint);
    }

public:
    void CollectCandidates(bool enabled)
    {
        myCollect=enabled; myUseAddedNodes=!enabled;
        if(enabled) { myNodePositions.reserve(32768); myAddedNodes.Clear(new NCollection_IncAllocator()); myAddedNodes.ReSize(32768); }
    }
    const std::vector<RWCae::MeshEntityId>& Candidates() const { return myCandidates; }
    void ProcessCandidates(ViewCaeGeom::MeshSliceWork& work)
    {
        using Clock=std::chrono::steady_clock;
        const auto mesh=myMeshGeom->GetMeshData();
        // Bound scratch independently of candidate count. Modifiers and lazy
        // geometry are evaluated on the caller thread; workers only read snapshots.
        const size_t blockSize=4096;
        std::vector<LinearInput> inputs(std::min(blockSize,myCandidates.size()));
        std::vector<LinearSection> sections(inputs.size());
        OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4);
        for(size_t begin=0;begin<myCandidates.size();begin+=blockSize) {
            if(myNodePositions.size()>32768) { myNodePositions.clear(); }
            const size_t count=std::min(blockSize,myCandidates.size()-begin);
            auto start=Clock::now();
            if(myModifyNodes) {
                for(size_t i=0;i<count;++i) {
                    checkCancelled();
                    const auto& element=mesh->IndexedElements()[myCandidates[begin+i]];
                    inputs[i].Valid=prepareLinear(element,inputs[i]);
                }
            }
            work.PrepareMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
            start=Clock::now();
            const auto cut=[&](int,int i) {
                checkCancelled(); sections[i].Count=0; sections[i].Reused=false; sections[i].Reason=FallbackReason::Unsupported;
                if(!myModifyNodes) {
                    const auto& element=mesh->IndexedElements()[myCandidates[begin+i]];
                    inputs[i].Valid=prepareLinear(element,inputs[i]);
                }
                sections[i].Valid=inputs[i].Valid && computeLinear(inputs[i],sections[i],myCandidates[begin+i]);
            };
            if(count>=512) { launcher.Perform(0,(int)count,cut); }
            else { for(size_t i=0;i<count;++i) { cut(0,(int)i); } }
            work.CutMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
            start=Clock::now();
            if(myTopology) {
                // Only this thread publishes cache entries, after worker reads
                // have finished. Cache limits affect reuse, never geometry.
                for(size_t i=0;i<count;++i) {
                    const auto& section=sections[i];
                    if(section.Reused) { ++work.TopologyCellHits; } else { ++work.TopologyCellMisses; }
                    if(!section.Valid) { continue; }
                    const auto id=myCandidates[begin+i];
                    auto& pattern=myTopology->Cells[ViewCaeGeom::MeshLinearSliceCache::Index(id)];
                    pattern.Element=id; pattern.Type=inputs[i].Type; pattern.Mask=section.Mask; pattern.Count=(unsigned char)section.Count;
                    for(size_t n=0;n<section.Count;++n) { pattern.Edges[n]=(unsigned char)section.Vertices[n].Edge; }
                }
            } else { work.TopologyCellMisses+=count; }
            appendBatch(begin,count,sections,work,launcher);
            for(size_t i=0;i<count;++i) {
                checkCancelled(); const auto id=myCandidates[begin+i]; const auto& element=mesh->IndexedElements()[id];
                if(!sections[i].Valid) {
                    const auto fallbackStart=Clock::now();
                    switch(sections[i].Reason) {
                        case FallbackReason::Unsupported: ++work.Unsupported; break;
                        case FallbackReason::NearPlane: ++work.NearPlane; break;
                        case FallbackReason::Warped: ++work.Warped; break;
                        case FallbackReason::Degenerate: ++work.Degenerate; break;
                        case FallbackReason::NearEndpoint: ++work.NearEndpoint; break;
                    }
                    ++work.Fallbacks; prepareFallback(id);
                    const auto first=mySliceMeshData->NbNodes(); processIndexedElement(element,id);
                    if(!myUseAddedNodes) {
                        for(auto node=first;node<mySliceMeshData->NbNodes();++node) {
                            const auto& weights=myGenNodes->Find(node).second.BaseNodes();
                            if(weights.size()==2 && std::abs(weights[0].second)>1e-12 && std::abs(weights[1].second)>1e-12) {
                                const uint64_t key=((uint64_t)std::min(weights[0].first,weights[1].first)<<32)|std::max(weights[0].first,weights[1].first);
                                myAffineEdges.emplace(key,node);
                            }
                        }
                    }
                    work.FallbackMs+=std::chrono::duration<double,std::milli>(Clock::now()-fallbackStart).count();
                }
            }
            work.MergeMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        }
    }
    //! Defines the rules for node rejection by bounding box.
    virtual Standard_Boolean RejectNode(const BVH_Vec3f&  theCornerMin,
                                        const BVH_Vec3f&  theCornerMax,
                                        Standard_Boolean& theIsInside) const override
    {
        theIsInside = false;
        return !boxAccepted(theCornerMin,theCornerMax);
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const ViewCaeGeom::MeshBvhData::MeshBvhBoxSet::SubBoxSet* theBoxSet,
                                    const Standard_Integer                                     theIndex,
                                    const Standard_Boolean                                     theIsInside) override
    {
        (void)theIsInside;
        if((++myVisited%256)==0 && myCancellation && myCancellation->Cancelled.load()) { throw std::runtime_error("Native section request cancelled during intersection"); }

        BVH_Box<float, BVH_DIM> aBoxFrom = theBoxSet->Box(theIndex);
        if (!boxAccepted(aBoxFrom.CornerMin(),aBoxFrom.CornerMax())) {
            return false;
        }

        RWCae::MeshEntityId anIndex = theBoxSet->Element(theIndex);
        if (!myMeshGeom->AcceptElement(anIndex)) { return false; }

        if(myCollect) { myCandidates.push_back(anIndex); return true; }

        const RWCae::MeshData::IndexedElement& anElem = myMeshGeom->GetMeshData()->IndexedElements()[anIndex];
        if (linearSlice(anElem)) { return true; }
        return processIndexedElement(anElem, anIndex);
    }

protected:
    template<class Vector> bool boxAccepted(const Vector& minimum,const Vector& maximum) const
    {
        double low=myEquation.w(),high=low;
        for(int axis=0;axis<3;++axis) { const double a=myEquation[axis]*minimum[axis],b=myEquation[axis]*maximum[axis]; low+=std::min(a,b); high+=std::max(a,b); }
        return low<=myTolerance && high>=-myTolerance;
    }
    //! Return true if bounding box is acceptable.
    virtual bool isBndBoxAccepted(const Graphic3d_BndBox3d& theBox) const
    {
        const auto& aPlane = mySlicePlane->GetEquation();
        double aMin = aPlane.w(), aMax = aPlane.w();
        for (int i = 0; i < 3; ++i) {
            const double a = aPlane[i] * theBox.CornerMin()[i];
            const double b = aPlane[i] * theBox.CornerMax()[i];
            aMin += std::min(a, b);
            aMax += std::max(a, b);
        }
        const double aTolerance = CONFUSION
            * std::sqrt(aPlane.x()*aPlane.x() + aPlane.y()*aPlane.y() + aPlane.z()*aPlane.z());
        return aMin <= aTolerance && aMax >= -aTolerance;
    }

    //! Return a node position.
    template<class Elem_T>
    inline void getNodePosition(const RWCae::MeshAdaptor<Elem_T>& theMesh,
                                Graphic3d_Vec3*                     theNode,
                                RWCae::MeshEntityId               theNodeIndex) const
    {
        theMesh.NodePosition(*theNode, theNodeIndex);
        myMeshGeom->ModifyNode(theNodeIndex, *theNode);
    }

    //! Process a separate mesh element.
    virtual bool processIndexedElement(const RWCae::MeshData::IndexedElement& theElem,
                                       RWCae::MeshEntityId                    theGlobalIndex)
    {
        auto process = [this, &theElem, theGlobalIndex](const auto& theAdaptor) {
            return AcceptElementSlice(theAdaptor, theGlobalIndex, theElem.Index);
        };

        if ((int)theElem.Type >= RWCae::MeshElem3dLowerType) {
            return RWCae::MeshAdaptorsIterator3dElems::Switch(theElem.Type, process, myMeshGeom->GetMeshData());
        }
        return RWCae::MeshAdaptorsIterator2dElems::Switch(theElem.Type, process, myMeshGeom->GetMeshData());
    }

protected:
    Handle(ViewCaeGeom::MeshGeometry) myMeshGeom;
    Handle(Graphic3d_ClipPlane)        mySlicePlane;
    Handle(RWCae::MeshData)          mySliceMeshData;
    TypedMapOfContours                 myCachedMapOfContours;
    NodesMap*                          myGenNodes;
    ElementsMap*                       myGenElems;
    AddedNodeMap                       myAddedNodes;
    std::vector<ViewCaeGeom::MeshSliceLine>* myLines;
    std::shared_ptr<ViewCaeGeom::MeshCutCancellation> myCancellation;
    std::shared_ptr<const std::vector<Graphic3d_Vec3>> myPreparedPositions;
    std::shared_ptr<ViewCaeGeom::MeshLinearSliceCache> myTopology;
    size_t myVisited=0;
    Graphic3d_Vec4d myEquation;
    double myTolerance=0;
    bool myCollect=false;
    bool myModifyNodes=true;
    bool myUseAddedNodes=true;
    std::vector<RWCae::MeshEntityId> myCandidates;
    mutable std::unordered_map<RWCae::MeshEntityId,Graphic3d_Vec3d> myNodePositions;

private:
    // Prefix ranges are fixed before any worker starts writing. OCCT vectors
    // and provenance maps are grown only on this thread; workers never append
    // or change hash tables. Fallbacks subsequently reuse these canonical edges.
    void appendBatch(size_t begin,size_t count,std::vector<LinearSection>& sections,
        ViewCaeGeom::MeshSliceWork& work,OSD_ThreadPool::Launcher& launcher)
    {
        using Clock=std::chrono::steady_clock;
        auto start=Clock::now();
        const auto mesh=myMeshGeom->GetMeshData();
        const size_t firstNode=mySliceMeshData->NbNodes(),firstElement=mySliceMeshData->NbIndexedElems();
        std::vector<Vertex*> fresh; fresh.reserve(count*3);
        for(size_t i=0;i<count;++i) {
            auto& section=sections[i]; if(!section.Valid || section.Count<3) { continue; }
            for(size_t n=0;n<section.Count;++n) {
                auto& vertex=section.Vertices[n];
                const uint64_t key=((uint64_t)std::min(vertex.A,vertex.B)<<32)|std::max(vertex.A,vertex.B);
                const auto found=myAffineEdges.find(key);
                if(found!=myAffineEdges.end()) { vertex.Node=found->second; continue; }
                bool reused=false;
                if(myUseAddedNodes) {
                    ViewCaeGeom::MeshInterpolatedNode origin; origin.AddBaseNode(vertex.A,1-vertex.T); origin.AddBaseNode(vertex.B,vertex.T);
                    reused=myAddedNodes.Find(origin,vertex.Node);
                }
                if(!reused) { vertex.Node=(RWCae::MeshEntityId)(firstNode+fresh.size()); fresh.push_back(&vertex); }
                myAffineEdges.emplace(key,vertex.Node);
            }
        }
        auto grow=[](auto& values,size_t end,const auto& value) {
            if(end>(size_t)std::numeric_limits<int>::max()) { throw std::length_error("Section output exceeds native index capacity"); }
            if(end>(size_t)values.Size()) { values.SetValue((int)end-1,value); }
        };
        auto& positions=mySliceMeshData->ChangeNodes();
        grow(positions,firstNode+fresh.size(),Graphic3d_Vec3(0));
        // Empty copies avoid the default one-weight allocation before each
        // worker creates the actual two-weight edge sample.
        std::vector<ViewCaeGeom::MeshInterpolatedNode> origins(fresh.size(),ViewCaeGeom::MeshInterpolatedNode(0));
        const auto fillNode=[&](int,int i) {
            const auto& vertex=*fresh[i]; positions[vertex.Node]=Graphic3d_Vec3(vertex.Point);
            auto& weights=origins[i].BaseNodes(); weights.resize(2);
            weights[0]={vertex.A,1-vertex.T}; weights[1]={vertex.B,vertex.T};
        };
        if(fresh.size()>=512) { launcher.Perform(0,(int)fresh.size(),fillNode); }
        else { for(size_t i=0;i<fresh.size();++i) { fillNode(0,(int)i); } }
        for(size_t i=0;i<fresh.size();++i) {
            if(myUseAddedNodes) { myAddedNodes.Bind(origins[i],fresh[i]->Node); }
            auto* mapped=myGenNodes->Bound(fresh[i]->Node,std::make_pair(myMeshGeom.get(),ViewCaeGeom::MeshInterpolatedNode(0)));
            mapped->second=std::move(origins[i]);
        }
        mySliceMeshData->SetCachedMinMax(Bnd_Box());
        work.SharedPointMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
        start=Clock::now();
        const RWCae::MeshElemType types[]={RWCae::MeshElemType::Triangle,RWCae::MeshElemType::Quad,RWCae::MeshElemType::Polygon};
        struct Span { size_t Global=0,Local=0,Nodes=0; int Type=-1; };
        std::vector<Span> spans(count);
        size_t elementEnds[3],nodeEnds[3],output=firstElement;
        for(int t=0;t<3;++t) { const auto& cells=mySliceMeshData->Elements(types[t]); elementEnds[t]=cells.NbElements(); nodeEnds[t]=cells.Nodes().Size(); }
        for(size_t i=0;i<count;++i) {
            const auto& section=sections[i]; if(!section.Valid || section.Count<3) { continue; }
            const int t=section.Count==3?0:section.Count==4?1:2;
            spans[i]={output++,elementEnds[t]++,nodeEnds[t],t}; nodeEnds[t]+=section.Count;
        }
        auto& indexed=mySliceMeshData->ChangeIndexedElements(); grow(indexed,output,RWCae::MeshData::IndexedElement());
        for(int t=0;t<3;++t) {
            auto& cells=mySliceMeshData->ChangeElements(types[t]);
            grow(cells.Nodes(),nodeEnds[t],RWCae::MeshEntityId(0)); grow(cells.GlobalIndices(),elementEnds[t],0);
            if(t==2) { grow(cells.NodesOffsets(),elementEnds[t],RWCae::MeshElemEntityOffset(0)); }
        }
        const auto fillElement=[&](int,int i) {
            const auto& span=spans[i]; if(span.Type<0) { return; }
            const auto& section=sections[i]; auto& cells=mySliceMeshData->ChangeElements(types[span.Type]);
            for(size_t n=0;n<section.Count;++n) { cells.Nodes()[(int)(span.Nodes+n)]=section.Vertices[n].Node; }
            cells.GlobalIndices()[(int)span.Local]=(int)span.Global;
            if(span.Type==2) { cells.NodesOffsets()[(int)span.Local]=(RWCae::MeshElemEntityOffset)(span.Nodes+section.Count); }
            indexed[(int)span.Global]=RWCae::MeshData::IndexedElement(types[span.Type],(RWCae::MeshEntityId)span.Local);
        };
        if(count>=512) { launcher.Perform(0,(int)count,fillElement); }
        else { for(size_t i=0;i<count;++i) { fillElement(0,(int)i); } }
        for(size_t i=0;i<count;++i) {
            const auto& span=spans[i]; if(span.Type<0) { continue; }
            const auto& source=mesh->IndexedElements()[myCandidates[begin+i]];
            myGenElems->Bind(ViewCaeGeom::MeshGeomElem((RWCae::MeshEntityId)span.Local,types[span.Type],false),
                std::make_pair(myMeshGeom.get(),ViewCaeGeom::MeshGeomElem(source.Index,source.Type,false)));
        }
        work.AppendMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    }
    void prepareFallback(RWCae::MeshEntityId element)
    {
        if(myUseAddedNodes) { return; }
        std::vector<RWCae::MeshEntityId> nodes; myMeshGeom->GetMeshData()->ElementNodes(element,nodes);
        if(nodes.size()>32) { enableAddedNodes(); return; }
        // Only affine edges incident to this fallback cell can be shared with
        // its contours. Avoid duplicating every section origin into a transient
        // weighted-key map for a handful of exceptional cells.
        for(size_t a=0;a<nodes.size();++a) { for(size_t b=a+1;b<nodes.size();++b) {
            const uint64_t key=((uint64_t)std::min(nodes[a],nodes[b])<<32)|std::max(nodes[a],nodes[b]);
            const auto found=myAffineEdges.find(key); if(found==myAffineEdges.end()) { continue; }
            const auto& origin=myGenNodes->Find(found->second).second;
            if(!myAddedNodes.IsBound(origin)) { myAddedNodes.Bind(origin,found->second); }
        } }
    }
    void enableAddedNodes()
    {
        if(myUseAddedNodes) { return; }
        for(const auto& edge:myAffineEdges) { myAddedNodes.Bind(myGenNodes->Find(edge.second).second,edge.second); }
        myUseAddedNodes=true;
    }
    void checkCancelled() const
    {
        if(myCancellation && myCancellation->Cancelled.load()) { throw std::runtime_error("Native section request cancelled during intersection"); }
    }
    // Verified convex linear cells have planar faces. Their section is the ordered set
    // of crossed original edges; retain the original edge FE weights exactly.
    // Warped, degenerate and near-node cases use the general contour cutter.
    bool linearSlice(const RWCae::MeshData::IndexedElement& element)
    {
        LinearInput input; LinearSection section;
        if(!prepareLinear(element,input) || !computeLinear(input,section)) { return false; }
        appendLinear(element,section); return true;
    }
    bool prepareLinear(const RWCae::MeshData::IndexedElement& element,LinearInput& input) const
    {
        const auto& mesh=myMeshGeom->GetMeshData(); const auto& elements=mesh->Elements(element.Type);
        using T=RWCae::MeshElemType;
        input.Type=element.Type;
        input.Count=element.Type==T::Tetrahedron?4:element.Type==T::Pyramid?5:element.Type==T::Prism?6:element.Type==T::Hexahedron?8:0;
        RWCae::MeshElemEntityOffset begin,end; elements.GetElemNodesRange(element.Index,&begin,&end);
        if(input.Count==0 || end-begin!=input.Count) { return false; }
        for(int i=0;i<input.Count;++i) {
            const auto id=elements.Nodes()[(int)begin+i]; input.Ids[i]=id;
            if(myCollect && myModifyNodes) {
                const auto found=myNodePositions.find(id);
                if(found!=myNodePositions.end()) { input.Points[i]=found->second; continue; }
            }
            auto point=myPreparedPositions ? (*myPreparedPositions)[id] : mesh->Nodes()[id];
            if(myModifyNodes) { myMeshGeom->ModifyNode(id,point); } input.Points[i]=Graphic3d_Vec3d(point);
            if(myCollect && myModifyNodes) { myNodePositions.emplace(id,input.Points[i]); }
        }
        return true;
    }
    bool computeLinear(const LinearInput& input,LinearSection& section,RWCae::MeshEntityId element=0) const
    {
        const auto& p=input.Points; const auto& ids=input.Ids; double values[8],magnitude=1;
        const auto& plane=myEquation;
        section.Mask=0;
        for(int i=0;i<input.Count;++i) { magnitude=std::max(magnitude,p[i].Dot(p[i])); values[i]=plane.x()*p[i].x()+plane.y()*p[i].y()+plane.z()*p[i].z()+plane.w(); if(std::abs(values[i])<=CONFUSION) { section.Reason=FallbackReason::NearPlane; return false; } if(values[i]>0) { section.Mask|=uint16_t(1)<<i; } }
        using T=RWCae::MeshElemType;
        const ViewCaeGeom::MeshLinearSliceCache::Pattern* pattern=nullptr;
        if(myTopology) {
            const auto& cached=myTopology->Cells[ViewCaeGeom::MeshLinearSliceCache::Index(element)];
            if(cached.Element==element && cached.Type==input.Type) { pattern=&cached; }
        }
        if(!pattern) {
            const auto x=p[1]-p[0];
            const auto y=(input.Type==T::Hexahedron || input.Type==T::Pyramid?p[3]:p[2])-p[0];
            const auto z=(input.Type==T::Hexahedron || input.Type==T::Pyramid?p[4]:p[3])-p[0];
            const double tolerance=8*std::numeric_limits<float>::epsilon()*std::sqrt(magnitude);
            if(std::abs(x.Dot(Graphic3d_Vec3d::Cross(y,z)))<=1e-12*x.Modulus()*y.Modulus()*z.Modulus()) { section.Reason=FallbackReason::Degenerate; return false; }
            if(input.Type==T::Hexahedron) {
                if((p[2]-p[0]-x-y).Modulus()>tolerance || (p[5]-p[0]-x-z).Modulus()>tolerance
                    || (p[7]-p[0]-y-z).Modulus()>tolerance || (p[6]-p[0]-x-y-z).Modulus()>tolerance) { section.Reason=FallbackReason::Warped; return false; }
            } else if(input.Type==T::Prism) {
                if((p[4]-p[1]-z).Modulus()>tolerance || (p[5]-p[2]-z).Modulus()>tolerance) { section.Reason=FallbackReason::Warped; return false; }
            } else if(input.Type==T::Pyramid) {
                const auto normal=Graphic3d_Vec3d::Cross(x,y);
                if(std::abs(normal.Dot(p[2]-p[0]))>tolerance*normal.Modulus()) { section.Reason=FallbackReason::Warped; return false; }
                for(int i=0;i<4;++i) {
                    const auto a=p[(i+1)%4]-p[i],b=p[(i+2)%4]-p[(i+1)%4];
                    if(Graphic3d_Vec3d::Cross(a,b).Dot(normal)<=1e-12*a.Modulus()*b.Modulus()*normal.Modulus()) { section.Reason=FallbackReason::Degenerate; return false; }
                }
            }
        }
        static constexpr int hexEdges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        static constexpr int tetraEdges[6][2]={{0,1},{1,2},{2,0},{0,3},{1,3},{2,3}};
        static constexpr int prismEdges[9][2]={{0,1},{1,2},{2,0},{3,4},{4,5},{5,3},{0,3},{1,4},{2,5}};
        static constexpr int pyramidEdges[8][2]={{0,1},{1,2},{2,3},{3,0},{0,4},{1,4},{2,4},{3,4}};
        const int (*edges)[2]=input.Type==T::Tetrahedron?tetraEdges:input.Type==T::Prism?prismEdges:input.Type==T::Pyramid?pyramidEdges:hexEdges;
        const int edgeCount=input.Type==T::Tetrahedron?6:input.Type==T::Prism?9:input.Type==T::Pyramid?8:12;
        auto& vertices=section.Vertices; auto& count=section.Count; Graphic3d_Vec3d center(0);
        const auto intersect=[&](int e) {
            int a=edges[e][0],b=edges[e][1]; if(ids[a]>ids[b]) { std::swap(a,b); }
            const double t=values[a]/(values[a]-values[b]); if(t<=.001||t>=.999) { section.Reason=FallbackReason::NearEndpoint; return false; }
            const auto point=p[a]*(1-t)+p[b]*t; vertices[count++]={ids[a],ids[b],0,t,0,point,e}; center+=point; return true;
        };
        if(pattern && pattern->Mask==section.Mask) {
            for(int n=0;n<pattern->Count;++n) { if(!intersect(pattern->Edges[n])) { return false; } }
            section.Reused=true; return true;
        }
        for(int e=0;e<edgeCount;++e) { int a=edges[e][0],b=edges[e][1]; if((values[a]>0)==(values[b]>0)) { continue; }
            // Canonical original-edge direction yields identical FE weights and
            // positions in adjacent cells, independent of worker scheduling.
            if(!intersect(e)) { return false; }
        }
        if(count<3) { return true; }
        center/=(double)count; const Graphic3d_Vec3d normal(plane.x(),plane.y(),plane.z());
        auto u=Graphic3d_Vec3d::Cross(normal,std::abs(normal.x())<.8?Graphic3d_Vec3d(1,0,0):Graphic3d_Vec3d(0,1,0)); u/=u.Modulus(); const auto v=Graphic3d_Vec3d::Cross(normal,u);
        for(size_t i=0;i<count;++i) { auto& vertex=vertices[i]; vertex.Angle=std::atan2((vertex.Point-center).Dot(v),(vertex.Point-center).Dot(u)); }
        std::sort(vertices.begin(),vertices.begin()+count,[](const auto& a,const auto& b) { return a.Angle<b.Angle; });
        return true;
    }
    void appendLinear(const RWCae::MeshData::IndexedElement& element,LinearSection& section,ViewCaeGeom::MeshSliceWork* work=nullptr)
    {
        auto& vertices=section.Vertices; const auto count=section.Count;
        if(count<3) { return; }
        const auto start=std::chrono::steady_clock::now();
        std::array<RWCae::MeshEntityId,12> nodes; size_t index=0;
        for(size_t i=0;i<count;++i) { auto& vertex=vertices[i]; const uint64_t key=((uint64_t)std::min(vertex.A,vertex.B)<<32)|std::max(vertex.A,vertex.B); const auto found=myAffineEdges.find(key);
            if(found!=myAffineEdges.end()) { vertex.Node=found->second; }
            else { ViewCaeGeom::MeshInterpolatedNode origin; origin.AddBaseNode(vertex.A,1-vertex.T); origin.AddBaseNode(vertex.B,vertex.T);
                // Share nodes with any neighboring cell routed through fallback.
                if(!myUseAddedNodes || !myAddedNodes.Find(origin,vertex.Node)) { vertex.Node=mySliceMeshData->AddNodePosition(Graphic3d_Vec3(vertex.Point)); if(myUseAddedNodes) { myAddedNodes.Bind(origin,vertex.Node); } myGenNodes->Bind(vertex.Node,std::make_pair(myMeshGeom.get(),origin)); }
                myAffineEdges.emplace(key,vertex.Node);
            } nodes[index++]=vertex.Node;
        }
        const auto sharedEnd=std::chrono::steady_clock::now();
        if(work) { work->SharedPointMs+=std::chrono::duration<double,std::milli>(sharedEnd-start).count(); }
        if(count==3) { mySliceMeshData->AddElement(RWCae::MeshElementTriangle(nodes.data())); }
        else if(count==4) { mySliceMeshData->AddElement(RWCae::MeshElementQuad(nodes.data())); }
        else { mySliceMeshData->AddElement(RWCae::MeshElementPolygon(nodes.data(),(RWCae::MeshElemEntityId)count)); }
        const auto& added=mySliceMeshData->IndexedElements()[mySliceMeshData->NbIndexedElems()-1];
        myGenElems->Bind(ViewCaeGeom::MeshGeomElem(added.Index,added.Type,false),std::make_pair(myMeshGeom.get(),ViewCaeGeom::MeshGeomElem(element.Index,element.Type,false)));
        if(work) { work->AppendMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-sharedEnd).count(); }
    }
    std::unordered_map<uint64_t,RWCae::MeshEntityId> myAffineEdges;
    //! Check and process a mesh element.
    template<class Elem_T>
    bool AcceptElementSlice(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                            RWCae::MeshEntityId               theGlobalIndex,
                            RWCae::MeshEntityId               theLocalIndex)
    {
        if (!myMeshGeom->AcceptElement(theGlobalIndex)) {
            return false;
        }

        const Elem_T& anElem = theAdaptor.GetElement(theLocalIndex);

        auto nodeValueFunc = [this, &theAdaptor, &anElem](RWCae::MeshElemEntityId theIdx) -> double {
            Graphic3d_Vec3 aPoint;
            getNodePosition(theAdaptor, &aPoint, anElem[theIdx]);
            const auto& aPlane = mySlicePlane->GetEquation();
            return (aPlane.x()*aPoint.x() + aPlane.y()*aPoint.y() + aPlane.z()*aPoint.z() + aPlane.w())
                 / std::sqrt(aPlane.x()*aPlane.x() + aPlane.y()*aPlane.y() + aPlane.z()*aPlane.z());
        };

        using Cutter = ViewCaeGeom::MeshGeometryTools::MeshElementCutter<Elem_T>;
        Cutter aCutter(theAdaptor, anElem, nodeValueFunc, &myCachedMapOfContours, 0.0, CONFUSION);
        if (!aCutter.Perform()) {
            return false;
        }

        aCutter.AddCutSurf(myMeshGeom, mySliceMeshData, myGenNodes, myGenElems, &myAddedNodes, theLocalIndex);

        if ((int)Elem_T::Type < RWCae::MeshElem3dLowerType) {
            RWCae::MeshEntityId aSourceElement = 0;
            if (!myMeshGeom->OriginElement(ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false),
                                           aSourceElement)) {
                return false;
            }
            for (const auto& aContour : aCutter.CutContourNodes()) {
                // A coplanar surface cell is already emitted as a face above.
                if (aContour.size() != 2) { continue; }
                std::array<RWCae::MeshEntityId, 2> aNodes;
                for (size_t i = 0; i < aNodes.size(); ++i) {
                    if (!myAddedNodes.Find(aContour[i], aNodes[i])) {
                        Graphic3d_Vec3 aPosition(0.0f, 0.0f, 0.0f);
                        for (const auto& aBase : aContour[i].BaseNodes()) {
                            Graphic3d_Vec3 aNode;
                            getNodePosition(theAdaptor, &aNode, aBase.first);
                            aPosition += aNode * (float)aBase.second;
                        }
                        aNodes[i] = mySliceMeshData->AddNodePosition(aPosition);
                        myAddedNodes.Bind(aContour[i], aNodes[i]);
                        myGenNodes->Bind(aNodes[i], std::make_pair(myMeshGeom.get(), aContour[i]));
                    }
                }
                if (aNodes[0] != aNodes[1]) { myLines->push_back({aNodes, aSourceElement}); }
            }
        }

        return true;
    }
};

}

namespace ViewCaeGeom {

// =======================================================================
// function : ~MeshGeometrySlice
// =======================================================================
MeshGeometrySlice::~MeshGeometrySlice() = default;

// =======================================================================
// function : MeshGeometrySlice
// =======================================================================
MeshGeometrySlice::MeshGeometrySlice(const Handle(MeshBvhData)& theBvhData) : MeshGeometry(theBvhData)
{
    //
}

// =======================================================================
// function : generateMeshData
// =======================================================================
void MeshGeometrySlice::generateMeshData()
{
    myLastWork={};
    ++myWorkGeneration;
    myMeshData = new RWCae::MeshData();
    const auto parentMesh=myParentGeometry.IsNull()?Handle(RWCae::MeshData)():myParentGeometry->GetFirstParent()->GetMeshData();
    const bool pooled=mySlicePlanes.Size()==1 && !parentMesh.IsNull()
        && parentMesh->NbIndexedElems()>=65536;
    const bool reuseTopology=pooled && CellTopologyReuseEnabled();
    if(reuseTopology && !myLinearCache) { myLinearCache=std::make_shared<MeshLinearSliceCache>(); }
    if(!reuseTopology) { myLinearCache.reset(); }
    if(pooled) {
        // Both maps are fully replaced on a rebuild. Their node storage can be
        // freed in bulk with the old allocator, without accumulating generations.
        myNodesMap.Clear(new NCollection_IncAllocator());
        myElementsMap.Clear(new NCollection_IncAllocator());
    } else { myNodesMap.Clear(); myElementsMap.Clear(); }
    myLines.clear();

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        const Handle(RWCae::MeshData)& aParentMeshData = aParent->GetMeshData();
        if (!aParentMeshData.IsNull()) {
            for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(mySlicePlanes); anIter.More(); anIter.Next()) {
                const auto boxes=myBvhData->BvhBoxSet(aParent);
                const auto topology=reuseTopology?myLinearCache:nullptr;
                if(topology && topology->Source!=boxes) { topology->Cells.assign(MeshLinearSliceCache::Capacity,MeshLinearSliceCache::Pattern()); topology->Source=boxes; }
                MeshSliceSelector aSelector(aParent, anIter.Value(), myMeshData, &myNodesMap, &myElementsMap, &myLines,myCancellation,
                    myBvhData->PreparedNodePositions(aParent),topology);
                aSelector.SetBVHSet(boxes.get());
                const bool staged=mySlicePlanes.Size()==1 && aParentMeshData->NbIndexedElems()>=65536;
                aSelector.CollectCandidates(staged);
                const auto start=std::chrono::steady_clock::now();
                aSelector.Select();
                if(staged) {
                    myLastWork.BvhMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                    myLastWork.Candidates+=aSelector.Candidates().size();
                    aSelector.ProcessCandidates(myLastWork);
                }
            }
        }
        if (!aParent->IsModifier()) {
            break;
        }
    }
}

bool MeshGeometrySlice::CellTopologyReuseEnabled()
{
    const auto* value=std::getenv("OCCTV_CUT_CELL_TOPOLOGY_CACHE");
    return value && value[0]=='1' && value[1]=='\0';
}

size_t MeshGeometrySlice::CountPlaneCandidates(const Handle(MeshBvhData)& bvh,
    const Handle(MeshGeometry)& source, const Graphic3d_Vec4d& plane)
{
    NodesMap nodes; ElementsMap elements; std::vector<MeshSliceLine> lines;
    MeshSliceSelector selector(source,new Graphic3d_ClipPlane(plane),new RWCae::MeshData(),&nodes,&elements,&lines,nullptr);
    selector.SetBVHSet(bvh->BvhBoxSet(source).get()); selector.CollectCandidates(true); selector.Select();
    return selector.Candidates().size();
}

// =======================================================================
// function : CalculateProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) MeshGeometrySlice::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    return calculateCommonProperty(theProp, theElem, theNodes);
}

Handle(RWCae::MeshNVecProperty) MeshGeometrySlice::CalculateLineProperty(
    const Handle(RWCae::MeshNVecProperty)& theProp) const
{
    if (theProp.IsNull() || !theProp->IsInitialized()
        || (!theProp->HasScalarComponents() && !theProp->HasNodeVecComponents()) || myLines.empty()) {
        return nullptr;
    }
    if (theProp->IsNodal()) { return CalculateProperty(theProp); }
    Handle(RWCae::MeshNVecProperty) aResult = new RWCae::MeshNVecProperty(
        theProp->Name(), theProp->NbComponents(), theProp->MapType(), theProp->Type());
    if (theProp->HasNodeVecComponents()) {
        aResult->Init<RWCae::MeshNodeVecProperty>();
        for (int c = 0; c < theProp->NbComponents(); ++c) {
            auto source = theProp->GetComponent(c);
            auto target = Handle(RWCae::MeshNodeVecProperty)::DownCast(aResult->GetComponent(c));
            target->Init((int)myLines.size(),0,false,false);
            for (size_t i = 0; i < myLines.size(); ++i) {
                target->ReserveValueMap((int)i,2);
                for (auto n : myLines[i].Nodes) {
                    MeshInterpolatedNode origin;
                    if (!OriginNode(n,origin)) { continue; }
                    double value = 0;
                    for (const auto& base : origin.BaseNodes()) {
                        if (base.second == 0) { continue; }
                        value += base.second * source->Value(RWCae::MeshPropertyEntityInfo(myLines[i].SourceElement,base.first));
                    }
                    target->SetValue(RWCae::MeshPropertyEntityInfo((int)i,n),value,false);
                }
            }
        }
        updateGenPropComponents(theProp,aResult);
        return aResult;
    }
    aResult->Init<RWCae::MeshScalarProperty>();
    for (int c = 0; c < theProp->NbComponents(); ++c) {
        auto aSource = theProp->GetComponent(c);
        auto aTarget = Handle(RWCae::MeshScalarProperty)::DownCast(aResult->GetComponent(c));
        aTarget->Init((int)myLines.size());
        for (size_t i = 0; i < myLines.size(); ++i) {
            aTarget->SetValue(RWCae::MeshPropertyEntityInfo((int)i),
                             aSource->Value(RWCae::MeshPropertyEntityInfo(myLines[i].SourceElement)), false);
        }
    }
    updateGenPropComponents(theProp, aResult);
    return aResult;
}

// =======================================================================
// function : ToJson
// =======================================================================
void MeshGeometrySlice::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    theJson.Key(NAME_PLANES);
    theJson.StartArray();
    for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(mySlicePlanes); anIter.More(); anIter.Next()) {
        theJson.StartObject();

        const Graphic3d_Vec4d& aPlane = anIter.Value()->GetEquation();
        theJson.Key(NAME_X);
        theJson.Double(aPlane.x());
        theJson.Key(NAME_Y);
        theJson.Double(aPlane.y());
        theJson.Key(NAME_Z);
        theJson.Double(aPlane.z());
        theJson.Key(NAME_W);
        theJson.Double(aPlane.w());

        theJson.EndObject();
    }
    theJson.EndArray();
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshGeometrySlice::FromJson(const RWBase::JsonHelper::Value& theJson,
                                 const Handle(RWCae::MeshData)&,
                                 int,
                                 bool& theIsChanged)
{
    const RWBase::JsonHelper::Value* aPlanesField = RWBase::JsonHelper::FindArrayMember(theJson, NAME_PLANES);
    if (aPlanesField == nullptr) {
        return false;
    }

    Graphic3d_SequenceOfHClipPlane::Iterator aPrevSlicesIter(mySlicePlanes);

    Graphic3d_SequenceOfHClipPlane aCurSlices;

    bool isChanged = false;
    for (rapidjson::Value::ConstValueIterator aPlanesIter = aPlanesField->Begin(); aPlanesIter != aPlanesField->End();
         ++aPlanesIter, aPrevSlicesIter.Next()) {
        const RWBase::JsonHelper::Value* aXField = RWBase::JsonHelper::FindDoubleMember(*aPlanesIter, NAME_X);
        const RWBase::JsonHelper::Value* aYField = RWBase::JsonHelper::FindDoubleMember(*aPlanesIter, NAME_Y);
        const RWBase::JsonHelper::Value* aZField = RWBase::JsonHelper::FindDoubleMember(*aPlanesIter, NAME_Z);
        const RWBase::JsonHelper::Value* aWField = RWBase::JsonHelper::FindDoubleMember(*aPlanesIter, NAME_W);
        if (aXField == nullptr || aYField == nullptr || aZField == nullptr || aWField == nullptr) {
            return false;
        }

        Graphic3d_Vec4d aCurSlicePlane(aXField->GetDouble(),
                                       aYField->GetDouble(),
                                       aZField->GetDouble(),
                                       aWField->GetDouble());
        aCurSlices.Append(new Graphic3d_ClipPlane(aCurSlicePlane));

        if (isChanged || aCurSlices.Size() > mySlicePlanes.Size()) {
            continue;
        }

        const Graphic3d_Vec4d& aPrevSlicePlane = aPrevSlicesIter.Value()->GetEquation();

        isChanged = !::IsEqual(aCurSlicePlane.x(), aPrevSlicePlane.x())
                 || !::IsEqual(aCurSlicePlane.y(), aPrevSlicePlane.y())
                 || !::IsEqual(aCurSlicePlane.z(), aPrevSlicePlane.z())
                 || !::IsEqual(aCurSlicePlane.w(), aPrevSlicePlane.w());
    }

    if (isChanged || aCurSlices.Size() != mySlicePlanes.Size()) {
        SetSlicePlanes(aCurSlices);
        theIsChanged = true;
    }

    return true;
}

} // namespace

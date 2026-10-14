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
#include <OSD_ThreadPool.hxx>
#include <NCollection_IncAllocator.hxx>

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
    struct Vertex { RWCae::MeshEntityId A,B,Node; double T,Angle; Graphic3d_Vec3d Point; };
    struct HexInput { RWCae::MeshEntityId Ids[8]; Graphic3d_Vec3d Points[8]; bool Valid=false; };
    struct HexSection { std::array<Vertex,12> Vertices; size_t Count=0; bool Valid=false; };
public:
    //! Constructor
    explicit MeshSliceSelector(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                               const Handle(Graphic3d_ClipPlane)&        theSlicePlane,
                               const Handle(RWCae::MeshData)&          theSliceMeshData,
                               NodesMap*                                 theGenNodes,
                               ElementsMap*                              theGenElems,
                               std::vector<ViewCaeGeom::MeshSliceLine>*   theLines,
                               const std::shared_ptr<ViewCaeGeom::MeshCutCancellation>& cancellation)
        : myMeshGeom(theMeshGeom),
          mySlicePlane(theSlicePlane),
          mySliceMeshData(theSliceMeshData),
          myGenNodes(theGenNodes),
          myGenElems(theGenElems),
          myLines(theLines),myCancellation(cancellation)
    {
        myEquation=mySlicePlane->GetEquation();
        myModifyNodes=myMeshGeom->HasNodeModifiers();
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
        std::vector<HexInput> inputs(std::min(blockSize,myCandidates.size()));
        std::vector<HexSection> sections(inputs.size());
        OSD_ThreadPool::Launcher launcher(*OSD_ThreadPool::DefaultPool(),4);
        for(size_t begin=0;begin<myCandidates.size();begin+=blockSize) {
            if(myNodePositions.size()>32768) { myNodePositions.clear(); }
            const size_t count=std::min(blockSize,myCandidates.size()-begin);
            auto start=Clock::now();
            if(myModifyNodes) {
                for(size_t i=0;i<count;++i) {
                    checkCancelled();
                    const auto& element=mesh->IndexedElements()[myCandidates[begin+i]];
                    inputs[i].Valid=element.Type==RWCae::MeshElemType::Hexahedron && prepareHex(element,inputs[i]);
                }
            }
            work.PrepareMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
            start=Clock::now();
            const auto cut=[&](int,int i) {
                checkCancelled(); sections[i].Count=0;
                if(!myModifyNodes) {
                    const auto& element=mesh->IndexedElements()[myCandidates[begin+i]];
                    inputs[i].Valid=element.Type==RWCae::MeshElemType::Hexahedron && prepareHex(element,inputs[i]);
                }
                sections[i].Valid=inputs[i].Valid && computeHex(inputs[i],sections[i]);
            };
            if(count>=512) { launcher.Perform(0,(int)count,cut); }
            else { for(size_t i=0;i<count;++i) { cut(0,(int)i); } }
            work.CutMs+=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
            start=Clock::now();
            for(size_t i=0;i<count;++i) {
                checkCancelled(); const auto id=myCandidates[begin+i]; const auto& element=mesh->IndexedElements()[id];
                if(sections[i].Valid) { appendHex(element,sections[i]); }
                else {
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
        if (anElem.Type == RWCae::MeshElemType::Hexahedron && affineHexSlice(anElem)) { return true; }
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
    size_t myVisited=0;
    Graphic3d_Vec4d myEquation;
    double myTolerance=0;
    bool myCollect=false;
    bool myModifyNodes=true;
    bool myUseAddedNodes=true;
    std::vector<RWCae::MeshEntityId> myCandidates;
    mutable std::unordered_map<RWCae::MeshEntityId,Graphic3d_Vec3d> myNodePositions;

private:
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
    // Affine hexes have convex planar faces. Their section is the ordered set
    // of crossed original edges; retain the original edge FE weights exactly.
    // Warped, degenerate and near-node cases use the general contour cutter.
    bool affineHexSlice(const RWCae::MeshData::IndexedElement& element)
    {
        HexInput input; HexSection section;
        if(!prepareHex(element,input) || !computeHex(input,section)) { return false; }
        appendHex(element,section); return true;
    }
    bool prepareHex(const RWCae::MeshData::IndexedElement& element,HexInput& input) const
    {
        const auto& mesh=myMeshGeom->GetMeshData(); const auto& elements=mesh->Elements(element.Type);
        RWCae::MeshElemEntityOffset begin,end; elements.GetElemNodesRange(element.Index,&begin,&end); if(end-begin!=8) { return false; }
        for(int i=0;i<8;++i) {
            const auto id=elements.Nodes()[(int)begin+i]; input.Ids[i]=id;
            if(myCollect && myModifyNodes) {
                const auto found=myNodePositions.find(id);
                if(found!=myNodePositions.end()) { input.Points[i]=found->second; continue; }
            }
            auto point=mesh->Nodes()[id]; if(myModifyNodes) { myMeshGeom->ModifyNode(id,point); } input.Points[i]=Graphic3d_Vec3d(point);
            if(myCollect && myModifyNodes) { myNodePositions.emplace(id,input.Points[i]); }
        }
        return true;
    }
    bool computeHex(const HexInput& input,HexSection& section) const
    {
        const auto& p=input.Points; const auto& ids=input.Ids; double values[8],magnitude=1;
        const auto& plane=myEquation;
        for(int i=0;i<8;++i) { magnitude=std::max(magnitude,p[i].Dot(p[i])); values[i]=plane.x()*p[i].x()+plane.y()*p[i].y()+plane.z()*p[i].z()+plane.w(); if(std::abs(values[i])<=CONFUSION) { return false; } }
        const auto x=p[1]-p[0],y=p[3]-p[0],z=p[4]-p[0];
        const double tolerance=8*std::numeric_limits<float>::epsilon()*std::sqrt(magnitude);
        if((p[2]-p[0]-x-y).Modulus()>tolerance || (p[5]-p[0]-x-z).Modulus()>tolerance
            || (p[7]-p[0]-y-z).Modulus()>tolerance || (p[6]-p[0]-x-y-z).Modulus()>tolerance
            || std::abs(x.Dot(Graphic3d_Vec3d::Cross(y,z)))<=1e-12*x.Modulus()*y.Modulus()*z.Modulus()) { return false; }
        static constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        auto& vertices=section.Vertices; auto& count=section.Count; Graphic3d_Vec3d center(0);
        for(const auto& edge:edges) { int a=edge[0],b=edge[1]; if((values[a]>0)==(values[b]>0)) { continue; }
            // Canonical original-edge direction yields identical FE weights and
            // positions in adjacent cells, independent of worker scheduling.
            if(ids[a]>ids[b]) { std::swap(a,b); }
            const double t=values[a]/(values[a]-values[b]); if(t<=.001||t>=.999) { return false; }
            const auto point=p[a]*(1-t)+p[b]*t; vertices[count++]={ids[a],ids[b],0,t,0,point}; center+=point;
        }
        if(count<3) { return true; }
        center/=(double)count; const Graphic3d_Vec3d normal(plane.x(),plane.y(),plane.z());
        auto u=Graphic3d_Vec3d::Cross(normal,std::abs(normal.x())<.8?Graphic3d_Vec3d(1,0,0):Graphic3d_Vec3d(0,1,0)); u/=u.Modulus(); const auto v=Graphic3d_Vec3d::Cross(normal,u);
        for(size_t i=0;i<count;++i) { auto& vertex=vertices[i]; vertex.Angle=std::atan2((vertex.Point-center).Dot(v),(vertex.Point-center).Dot(u)); }
        std::sort(vertices.begin(),vertices.begin()+count,[](const auto& a,const auto& b) { return a.Angle<b.Angle; });
        return true;
    }
    void appendHex(const RWCae::MeshData::IndexedElement& element,HexSection& section)
    {
        auto& vertices=section.Vertices; const auto count=section.Count;
        if(count<3) { return; }
        std::array<RWCae::MeshEntityId,12> nodes; size_t index=0;
        for(size_t i=0;i<count;++i) { auto& vertex=vertices[i]; const uint64_t key=((uint64_t)std::min(vertex.A,vertex.B)<<32)|std::max(vertex.A,vertex.B); const auto found=myAffineEdges.find(key);
            if(found!=myAffineEdges.end()) { vertex.Node=found->second; }
            else { ViewCaeGeom::MeshInterpolatedNode origin; origin.AddBaseNode(vertex.A,1-vertex.T); origin.AddBaseNode(vertex.B,vertex.T);
                // Share nodes with any neighboring cell routed through fallback.
                if(!myUseAddedNodes || !myAddedNodes.Find(origin,vertex.Node)) { vertex.Node=mySliceMeshData->AddNodePosition(Graphic3d_Vec3(vertex.Point)); if(myUseAddedNodes) { myAddedNodes.Bind(origin,vertex.Node); } myGenNodes->Bind(vertex.Node,std::make_pair(myMeshGeom.get(),origin)); }
                myAffineEdges.emplace(key,vertex.Node);
            } nodes[index++]=vertex.Node;
        }
        if(count==3) { mySliceMeshData->AddElement(RWCae::MeshElementTriangle(nodes.data())); }
        else if(count==4) { mySliceMeshData->AddElement(RWCae::MeshElementQuad(nodes.data())); }
        else { mySliceMeshData->AddElement(RWCae::MeshElementPolygon(nodes.data(),(RWCae::MeshElemEntityId)count)); }
        const auto& added=mySliceMeshData->IndexedElements()[mySliceMeshData->NbIndexedElems()-1];
        myGenElems->Bind(ViewCaeGeom::MeshGeomElem(added.Index,added.Type,false),std::make_pair(myMeshGeom.get(),ViewCaeGeom::MeshGeomElem(element.Index,element.Type,false)));
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
    const auto parentMesh=myParentGeometry.IsNull()?Handle(RWCae::MeshData)():myParentGeometry->GetMeshData();
    const bool pooled=mySlicePlanes.Size()==1 && !parentMesh.IsNull()
        && parentMesh->NbIndexedElems()>=65536;
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
                MeshSliceSelector aSelector(aParent, anIter.Value(), myMeshData, &myNodesMap, &myElementsMap, &myLines,myCancellation);
                aSelector.SetBVHSet(myBvhData->BvhBoxSet(aParent).get());
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

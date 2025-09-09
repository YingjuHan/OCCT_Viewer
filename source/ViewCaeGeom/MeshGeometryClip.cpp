#include <ViewCaeGeom/MeshGeometryClip.h>

#include <ViewCaeGeom/MeshGeometryTools.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <BVH_Traverse.hxx>
#include <NCollection_IncAllocator.hxx>
#include <NCollection_Map.hxx>

namespace {
using NodesMap           = ViewCaeGeom::MeshGeometry::NodesMap;
using ElementsMap        = ViewCaeGeom::MeshGeometry::ElementsMap;
using TypedMapOfContours = ViewCaeGeom::MeshGeometryTools::TypedMapOfContours;
using AddedNodeMap       = ViewCaeGeom::MeshGeometryTools::AddedNodeMap;
using NodesAdjElems      = ViewCaeGeom::MeshGeometryTools::NodesAdjElems;

static constexpr int    BVH_DIM   = 3;
static constexpr double CONFUSION = 1e-5;

static constexpr const char* NAME_HASPLANE = "hasplane";
static constexpr const char* NAME_X        = "x";
static constexpr const char* NAME_Y        = "y";
static constexpr const char* NAME_Z        = "z";
static constexpr const char* NAME_W        = "w";
static constexpr const char* NAME_CUT      = "cut";

//! Selector for overlapped with clip plane mesh elements.
struct MeshClipSelector : ViewCaeGeom::MeshHBvhBoxSetTraverse<ViewCaeGeom::MeshBvhData::MeshBvhBoxSet> {
public:
    //! Constructor
    explicit MeshClipSelector(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                              const Handle(Graphic3d_ClipPlane)&        theClipPlane,
                              const Handle(RWCae::MeshData)&          theMeshDataOut,
                              NodesMap*                                 theOutNodes,
                              ElementsMap*                              theOutElems,
                              TColStd_PackedMapOfInteger*               theExtraClippedNodes,
                              TColStd_PackedMapOfInteger*               theNotClippedNodes,
                              bool                                      theToCut)
        : myMeshGeom(theMeshGeom),
          myClipPlane(theClipPlane),
          myMeshDataOut(theMeshDataOut),
          myOutNodes(theOutNodes),
          myOutElems(theOutElems),
          myExtraClippedNodes(theExtraClippedNodes),
          myNotClippedNodes(theNotClippedNodes),
          myToCut(theToCut),
          myAlloc(new NCollection_IncAllocator())
    {
    }

public:
    //! Defines the rules for node rejection by bounding box.
    virtual Standard_Boolean RejectNode(const BVH_Vec3f&  theCornerMin,
                                        const BVH_Vec3f&  theCornerMax,
                                        Standard_Boolean& theIsInside) const override
    {
        theIsInside = false;
        Graphic3d_BndBox3d aBox{BVH_Vec3d(theCornerMin), BVH_Vec3d(theCornerMax)};
        return !isBoxAccepted(aBox);
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const ViewCaeGeom::MeshBvhData::MeshBvhBoxSet::SubBoxSet* theBoxSet,
                                    const Standard_Integer                                     theIndex,
                                    const Standard_Boolean                                     theIsInside) override
    {
        (void)theIsInside;

        BVH_Box<float, BVH_DIM> aBoxFrom = theBoxSet->Box(theIndex);
        Graphic3d_BndBox3d      aBoxTo(BVH_Vec3d(aBoxFrom.CornerMin()), BVH_Vec3d(aBoxFrom.CornerMax()));
        if (!isBoxAccepted(aBoxTo)) {
            return false;
        }

        RWCae::MeshEntityId anIndex = theBoxSet->Element(theIndex);

        return processIndexedElement(anIndex, true);
    }

protected:
    //! Return true if bounding box is acceptable.
    virtual bool isBoxAccepted(const Graphic3d_BndBox3d& theBox) const
    {
        int aRes = myClipPlane->ProbeBox(theBox);
        return aRes == Graphic3d_ClipState_On;
    }

    //! Process a separate mesh element.
    virtual bool processIndexedElement(RWCae::MeshEntityId theGlobalIndex, bool theIsOriginElem)
    {
        const RWCae::MeshData::IndexedElement& anElem = myMeshGeom->GetMeshData()->IndexedElements()[theGlobalIndex];

        if (myToCut) {
            auto processCut = [this, &anElem, theGlobalIndex, theIsOriginElem](const auto& theAdaptor) {
                return AcceptElementCut(theAdaptor, theGlobalIndex, anElem.Index, theIsOriginElem);
            };

            return RWCae::MeshAdaptorsIteratorAllElems::Switch(anElem.Type, processCut, myMeshGeom->GetMeshData());
        }
        auto process = [this, &anElem, theGlobalIndex](const auto& theAdaptor) {
            return AcceptElementFilter(theAdaptor, theGlobalIndex, anElem.Index);
        };

        return RWCae::MeshAdaptorsIterator3dElems::Switch(anElem.Type, process, myMeshGeom->GetMeshData());
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

protected:
    Handle(ViewCaeGeom::MeshGeometry) myMeshGeom;
    Handle(Graphic3d_ClipPlane)        myClipPlane;
    Handle(RWCae::MeshData)          myMeshDataOut;
    TypedMapOfContours                 myCachedMapOfContours;
    NodesMap*                          myOutNodes;
    ElementsMap*                       myOutElems;
    TColStd_PackedMapOfInteger*        myExtraClippedNodes;
    TColStd_PackedMapOfInteger*        myNotClippedNodes;
    AddedNodeMap                       myAddedNodes;
    bool                               myToCut = false;

    Handle(NCollection_IncAllocator) myAlloc;

private:
    //! Add new 2d element.
    template<class Elem_T>
    void AddSurfElem(const std::vector<RWCae::MeshEntityId>& thePnts,
                     RWCae::MeshElemType                     theType,
                     RWCae::MeshEntityId                     theLocalIndex)
    {
        RWCae::MeshEntityId aNewElemId = myMeshDataOut->NbSurfElems();
        switch (theType) {
            case RWCae::MeshElemType::Triangle: {
                RWCae::MeshElementTriangle aSurfElem(thePnts.data(), aNewElemId);
                myMeshDataOut->AddSurfaceElement(aSurfElem);
                break;
            }
            case RWCae::MeshElemType::Triangle2nd: {
                RWCae::MeshElementTriangle2nd aSurfElem(thePnts.data(), aNewElemId);
                myMeshDataOut->AddSurfaceElement(aSurfElem);
                break;
            }
            case RWCae::MeshElemType::Quad: {
                RWCae::MeshElementQuad aSurfElem(thePnts.data(), aNewElemId);
                myMeshDataOut->AddSurfaceElement(aSurfElem);
                break;
            }
            case RWCae::MeshElemType::Quad2nd: {
                RWCae::MeshElementQuad2nd aSurfElem(thePnts.data(), aNewElemId);
                myMeshDataOut->AddSurfaceElement(aSurfElem);
                break;
            }
            case RWCae::MeshElemType::Polygon: {
                RWCae::MeshElementPolygon aSurfElem(thePnts.data(),
                                                      static_cast<RWCae::MeshElemEntityId>(thePnts.size()),
                                                      aNewElemId);
                myMeshDataOut->AddSurfaceElement(aSurfElem);
                break;
            }
            default: return;
        }

        myOutElems->Bind(ViewCaeGeom::MeshGeomElem(myMeshDataOut->NbSurfElems(theType) - 1, theType, true),
                         std::make_pair(myMeshGeom.get(),
                                        ViewCaeGeom::MeshGeomElem(theLocalIndex, Elem_T::Type, false)));
    }

    //! Add element's face to output mesh data.
    template<class Elem_T>
    void AddElemFace(const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                     const std::vector<RWCae::MeshEntityId>& theRefFace,
                     RWCae::MeshEntityId                     theIndex)
    {
        const Elem_T& anElem = theAdaptor.GetElement(theIndex);

        Graphic3d_Vec3  aPoint;
        Graphic3d_Vec4d aPointToProbe;
        for (RWCae::MeshElemEntityId i = 0; i < anElem.NbNodes(); i++) {
            getNodePosition(theAdaptor, &aPoint, anElem[i]);
            aPointToProbe.SetValues(aPoint.x(), aPoint.y(), aPoint.z(), 1.0);

            if (myClipPlane->ProbePoint(aPointToProbe) == Graphic3d_ClipState_Out) {
                return;
            }
        }

        std::vector<RWCae::MeshEntityId> aFaceNodes;

        RWCae::MeshElemEntityId aNbFaces = theAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceId = 0; aFaceId < aNbFaces; aFaceId++) {
            RWCae::MeshElemType aType;
            theAdaptor.FaceNodes(anElem, aFaceId, aFaceNodes, aType);

            std::vector<RWCae::MeshEntityId> aSortedFaceNodes = aFaceNodes;
            RWCae::SortedMeshElem2d::Sort(aSortedFaceNodes);

            if (!RWCae::MeshElement2dHasher::IsEqual(theRefFace, aSortedFaceNodes)) {
                continue;
            }

            for (RWCae::MeshEntityId& aFaceNode : aFaceNodes) {
                myNotClippedNodes->Add(aFaceNode);

                Graphic3d_Vec3 aPnt;
                getNodePosition(theAdaptor, &aPnt, aFaceNode);

                RWCae::MeshEntityId              aNewPnt = myMeshDataOut->AddNodePosition(aPnt);
                ViewCaeGeom::MeshInterpolatedNode anInterNode;
                anInterNode.AddBaseNode(aFaceNode, 1.0);
                aFaceNode = aNewPnt;
                myOutNodes->Bind(aFaceNode, std::make_pair(myMeshGeom.get(), anInterNode));
            }

            AddSurfElem<Elem_T>(aFaceNodes, aType, theIndex);

            break;
        }
    }

    //! Process adjacent element.
    bool ProcessAdjElem(const std::vector<RWCae::MeshEntityId>& theFaceNodes, RWCae::MeshEntityId theIdx)
    {
        RWCae::MeshData::IndexedElement anAdjIndElem = myMeshGeom->GetMeshData()->IndexedElements()[theIdx];
        if ((int)anAdjIndElem.Type < RWCae::MeshElem3dLowerType
            || (int)anAdjIndElem.Type > RWCae::MeshElem3dUpperType) {
            return false;
        }

        auto compute = [this, &theFaceNodes, &anAdjIndElem](const auto& theAdjAdaptor) {
            AddElemFace(theAdjAdaptor, theFaceNodes, anAdjIndElem.Index);
            return true;
        };

        RWCae::MeshAdaptorsIterator3dElems::Switch(anAdjIndElem.Type, compute, myMeshGeom->GetMeshData());
        return true;
    }

    //! Gather nodes inside clip plane.
    template<class Elem_T>
    bool GatherNodesIn(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                       const Elem_T&                       theElem,
                       std::vector<bool>*                  theNodesIn,
                       RWCae::MeshElemEntityId*          theNbIn)
    {
        Graphic3d_Vec3  aPoint;
        Graphic3d_Vec4d aPointToProbe;

        theNodesIn->resize(theElem.NbNodes(), false);

        *theNbIn = 0;

        for (RWCae::MeshElemEntityId i = 0; i < theElem.NbNodes(); i++) {
            getNodePosition(theAdaptor, &aPoint, theElem[i]);
            aPointToProbe.SetValues(aPoint.x(), aPoint.y(), aPoint.z(), 1.0);

            bool isIn = myClipPlane->ProbePoint(aPointToProbe) != Graphic3d_ClipState_Out;
            if (!isIn) {
                continue;
            }

            (*theNodesIn)[i] = true;
            (*theNbIn)++;

            if (!myToCut) {
                myExtraClippedNodes->Add(theElem[i]);
            }
        }

        return (*theNbIn) > 0 && (*theNbIn) < theElem.NbNodes();
    }

    //! Gather adjacent elements for all element's nodes.
    template<class Elem_T>
    void GatherNodesAdj(const Elem_T& theElem, NodesAdjElems* theAdjElems)
    {
        static constexpr int aNbBuckets = 100;

        NCollection_DataMap<RWCae::MeshEntityId, int> anElemsMap(aNbBuckets, myAlloc);
        for (RWCae::MeshElemEntityId i = 0; i < theElem.NbNodes(); i++) {
            RWCae::MeshElemEntityId    aNbAdj     = 0;
            const RWCae::MeshEntityId* anAdjElems = myMeshGeom->GetMeshData()->NodeAdjElems(theElem[i], aNbAdj);

            for (RWCae::MeshElemEntityId anAdjIter = 0; anAdjIter < aNbAdj; anAdjIter++) {
                if ((int)anAdjElems[anAdjIter] != theElem.GlobalIndex() && !anElemsMap.IsBound(anAdjElems[anAdjIter])) {
                    anElemsMap.Bind(anAdjElems[anAdjIter], anElemsMap.Size());
                }
            }
        }

        theAdjElems->first.clear();
        theAdjElems->first.resize(theElem.NbNodes(), ViewCaeGeom::BitMaskSet<1>(anElemsMap.Size()));

        for (RWCae::MeshElemEntityId i = 0; i < theElem.NbNodes(); i++) {
            RWCae::MeshElemEntityId    aNbAdj     = 0;
            const RWCae::MeshEntityId* anAdjElems = myMeshGeom->GetMeshData()->NodeAdjElems(theElem[i], aNbAdj);

            for (RWCae::MeshElemEntityId anAdjIter = 0; anAdjIter < aNbAdj; anAdjIter++) {
                if ((int)anAdjElems[anAdjIter] != theElem.GlobalIndex()) {
                    theAdjElems->first[i].Set(anElemsMap.Find(anAdjElems[anAdjIter]), 0);
                }
            }
        }

        theAdjElems->second.resize(anElemsMap.Size());
        for (NCollection_DataMap<RWCae::MeshEntityId, int>::Iterator anIter(anElemsMap); anIter.More();
             anIter.Next()) {
            theAdjElems->second[anIter.Value()] = anIter.Key();
        }
    }

    //! Process filtered mesh element.
    template<class Elem_T>
    bool AcceptElementFilter(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                             RWCae::MeshEntityId               theGlobalIndex,
                             RWCae::MeshEntityId               theLocalIndex)
    {
        if (!myMeshGeom->AcceptElement(theGlobalIndex)) {
            return false;
        }

        const Elem_T& anElem = theAdaptor.GetElement(theLocalIndex);

        std::vector<bool>         aNodesIn;
        RWCae::MeshElemEntityId aNbIn = 0;
        if (!GatherNodesIn(theAdaptor, anElem, &aNodesIn, &aNbIn)) {
            return false;
        }

        NodesAdjElems anAdjElems;
        GatherNodesAdj(anElem, &anAdjElems);

        std::vector<RWCae::MeshEntityId> aFaceNodes;

        RWCae::MeshElemEntityId aNbFaces = theAdaptor.NbFaces(anElem);
        for (RWCae::MeshElemEntityId aFaceId = 0; aFaceId < aNbFaces; aFaceId++) {
            RWCae::MeshElemEntityId aNbFaceNodes = theAdaptor.NbFaceNodes(anElem, aFaceId);

            bool isClipped = false;
            for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
                if (!aNodesIn[theAdaptor.FaceNodeIndex(anElem, aFaceId, aNodeIter)]) {
                    isClipped = true;
                    break;
                }
            }
            if (isClipped) {
                continue;
            }

            ViewCaeGeom::BitMaskSet<1> aMask(static_cast<int>(anAdjElems.second.size()));
            aMask.SetAll();
            NCollection_Map<RWCae::MeshEntityId> anAdjFace;
            for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbFaceNodes; aNodeIter++) {
                aMask.Intersected(anAdjElems.first[theAdaptor.FaceNodeIndex(anElem, aFaceId, aNodeIter)]);
            }

            if (aMask.IsZero()) {
                continue;
            }

            RWCae::MeshElemType aFaceType;
            theAdaptor.FaceNodes(anElem, aFaceId, aFaceNodes, aFaceType);
            RWCae::SortedMeshElem2d::Sort(aFaceNodes);

            for (int i = 0; i < aMask.NbValues(); i++) {
                if (!aMask.IsSet(i)) {
                    continue;
                }
                RWCae::MeshEntityId anAdjElem = anAdjElems.second[i];
                if (ProcessAdjElem(aFaceNodes, anAdjElem)) {
                    break;
                }
            }
        }
        return true;
    }

    //! Process cut mesh element.
    template<class Elem_T>
    bool AcceptElementCut(const RWCae::MeshAdaptor<Elem_T>& theAdaptor,
                          RWCae::MeshEntityId               theGlobalIndex,
                          RWCae::MeshEntityId               theIdx,
                          bool                                theIsOriginElem)
    {
        if (!myMeshGeom->AcceptElement(theGlobalIndex)) {
            return false;
        }

        const Elem_T& anElem = theAdaptor.GetElement(theIdx);

        std::vector<bool>         aNodesIn;
        RWCae::MeshElemEntityId aNbIn = 0;

        bool isOn = GatherNodesIn(theAdaptor, anElem, &aNodesIn, &aNbIn);
        if ((theIsOriginElem && !isOn) || (!theIsOriginElem && aNbIn == 0)) {
            return false;
        }

        auto valueFunc = [this, &theAdaptor, &anElem, &aNodesIn](RWCae::MeshElemEntityId theIndex) -> double {
            Graphic3d_Vec3 aPoint;
            getNodePosition(theAdaptor, &aPoint, anElem[theIndex]);
            double aDist = this->myClipPlane->ToPlane().Distance(gp_Pnt(aPoint.x(), aPoint.y(), aPoint.z()))
                         * (aNodesIn[theIndex] ? 1.0 : -1.0);
            return aDist;
        };

        using Cutter = ViewCaeGeom::MeshGeometryTools::MeshElementCutter<Elem_T>;
        Cutter aCutter(theAdaptor, anElem, valueFunc, &myCachedMapOfContours, 0.0, CONFUSION);
        if (!aCutter.Perform()) {
            return false;
        }

        NodesAdjElems anAdjElems;
        if (theAdaptor.NbFaces(anElem) != 1) {
            GatherNodesAdj(anElem, &anAdjElems);
        }

        if (aCutter.AddCutElem(myMeshGeom, myMeshDataOut, anAdjElems, myOutNodes, myOutElems, &myAddedNodes, theIdx)) {
            return true;
        }

        if (!theIsOriginElem) {
            return false;
        }

        for (RWCae::MeshEntityId anAdjElem : aCutter.AdjElemToCheck()) {
            processIndexedElement(anAdjElem, false);
        }

        return true;
    }
};

}

namespace ViewCaeGeom {

// =======================================================================
// function : MeshGeometryClip
// =======================================================================
MeshGeometryClip::MeshGeometryClip(const Handle(MeshBvhData)& theBvhData) : MeshGeometry(theBvhData)
{
    //
}

// =======================================================================
// function : generateMeshData
// =======================================================================
void MeshGeometryClip::generateMeshData()
{
    myMeshDataIsGenerated = true;

    myMeshData = new RWCae::MeshData();
    myNodesMap.Clear();
    myElementsMap.Clear();
    myExtraClippedNodes.Clear();

    Handle(MeshGeometry) aParent = myParentGeometry;
    while (!aParent.IsNull()) {
        if (!aParent->GetMeshData().IsNull()) {
            TColStd_PackedMapOfInteger* aClippedNodes = myExtraClippedNodes.Bound(aParent,
                                                                                  TColStd_PackedMapOfInteger());
            TColStd_PackedMapOfInteger  aNotClippedNodes;

            MeshClipSelector aSelector(aParent,
                                       myClipPlane,
                                       myMeshData,
                                       &myNodesMap,
                                       &myElementsMap,
                                       aClippedNodes,
                                       &aNotClippedNodes,
                                       myToCut);
            aSelector.SetBVHSet(myBvhData->BvhBoxSet(aParent).get());
            aSelector.Select();

            aClippedNodes->Subtract(aNotClippedNodes);
        }
        if (!aParent->IsModifier()) {
            break;
        }
        aParent = aParent->GetParent();
    }

    myMeshDataIsGenerated = false;
}

// =======================================================================
// function : acceptElement
// =======================================================================
bool MeshGeometryClip::acceptElement(const Handle(RWCae::MeshData)& theMeshData,
                                     const RWCae::MeshEntityId      theElemIdx,
                                     const MeshGeometry*              theMeshGeom) const
{
    if (myClipPlane.IsNull() || myMeshDataIsGenerated) {
        return true;
    }

    const RWCae::MeshData::IndexedElement& anIndexedElement = theMeshData->IndexedElements().Value(theElemIdx);

    const RWCae::MeshElements& anElements = theMeshData->Elements(anIndexedElement.Type);

    RWCae::MeshElemEntityOffset aNodesStart = 0;
    RWCae::MeshElemEntityOffset aNodesEnd   = 0;
    anElements.GetElemNodesRange(anIndexedElement.Index, &aNodesStart, &aNodesEnd);

    for (RWCae::MeshElemEntityOffset i = aNodesStart; i < aNodesEnd; i++) {
        RWCae::MeshEntityId aNodeIdx = anElements.Nodes()[static_cast<int>(i)];
        Graphic3d_Vec3        aNode    = theMeshData->Nodes()[aNodeIdx];
        theMeshGeom->ModifyNode(aNodeIdx, aNode);
        if (myClipPlane->ProbePoint(Graphic3d_Vec4d(aNode.x(), aNode.y(), aNode.z(), 1.0)) == Graphic3d_ClipState_Out) {
            return false;
        }
    }

    return true;
}

// =======================================================================
// function : acceptNode
// =======================================================================
bool MeshGeometryClip::acceptNode(const RWCae::MeshEntityId theNodeIdx,
                                  const MeshGeometry*         theMeshGeom,
                                  const Graphic3d_Vec3&       theNode) const
{
    if (myClipPlane.IsNull() || myMeshDataIsGenerated) {
        return true;
    }

    // Check if the node belongs to a clipped element
    const TColStd_PackedMapOfInteger* aClippedNodes = myExtraClippedNodes.Seek(theMeshGeom);
    if (aClippedNodes != nullptr && aClippedNodes->Contains(theNodeIdx)) {
        return false;
    }

    return myClipPlane->ProbePoint(Graphic3d_Vec4d(theNode.x(), theNode.y(), theNode.z(), 1.0))
        != Graphic3d_ClipState_Out;
}

// =======================================================================
// function : CalculateProperty
// =======================================================================
Handle(RWCae::MeshNVecProperty) MeshGeometryClip::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    return calculateCommonProperty(theProp, theElem, theNodes);
}

// =======================================================================
// function : ToJson
// =======================================================================
void MeshGeometryClip::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    theJson.Key(NAME_HASPLANE);
    theJson.Bool(!myClipPlane.IsNull());
    if (!myClipPlane.IsNull()) {
        const Graphic3d_Vec4d& aClipPlane = myClipPlane->GetEquation();
        theJson.Key(NAME_X);
        theJson.Double(aClipPlane.x());
        theJson.Key(NAME_Y);
        theJson.Double(aClipPlane.y());
        theJson.Key(NAME_Z);
        theJson.Double(aClipPlane.z());
        theJson.Key(NAME_W);
        theJson.Double(aClipPlane.w());
    }
    theJson.Key(NAME_CUT);
    theJson.Bool(myToCut);
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshGeometryClip::FromJson(const RWBase::JsonHelper::Value& theJson,
                                const Handle(RWCae::MeshData)&,
                                int,
                                bool& theIsChanged)
{
    const RWBase::JsonHelper::Value* aHasPlaneField = RWBase::JsonHelper::FindBoolMember(theJson, NAME_HASPLANE);
    if (aHasPlaneField == nullptr) {
        return false;
    }
    if (!aHasPlaneField->GetBool()) {
        if (!myClipPlane.IsNull()) {
            theIsChanged = true;
        }
        return true;
    }

    const RWBase::JsonHelper::Value* aXField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_X);
    const RWBase::JsonHelper::Value* aYField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_Y);
    const RWBase::JsonHelper::Value* aZField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_Z);
    const RWBase::JsonHelper::Value* aWField = RWBase::JsonHelper::FindDoubleMember(theJson, NAME_W);
    if (aXField == nullptr || aYField == nullptr || aZField == nullptr || aWField == nullptr) {
        return false;
    }

    Graphic3d_Vec4d aCurClipPlane(aXField->GetDouble(),
                                  aYField->GetDouble(),
                                  aZField->GetDouble(),
                                  aWField->GetDouble());

    if (myClipPlane.IsNull() || !::IsEqual(myClipPlane->GetEquation().x(), aCurClipPlane.x())
        || !::IsEqual(myClipPlane->GetEquation().y(), aCurClipPlane.y())
        || !::IsEqual(myClipPlane->GetEquation().z(), aCurClipPlane.z())
        || !::IsEqual(myClipPlane->GetEquation().w(), aCurClipPlane.w())) {
        SetClipPlane(new Graphic3d_ClipPlane(aCurClipPlane));
        theIsChanged = true;
    }

    const RWBase::JsonHelper::Value* aCutField = RWBase::JsonHelper::FindBoolMember(theJson, NAME_CUT);
    if (aCutField == nullptr) {
        return false;
    }

    if (myToCut != aCutField->GetBool()) {
        myToCut      = aCutField->GetBool();
        theIsChanged = true;
    }

    return true;
}

} // namespace         
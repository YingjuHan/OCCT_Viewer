#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <ViewCaeGeom/MeshGeometryTools.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshScalarProperty.h>

#include <BVH_Traverse.hxx>
#include <NCollection_Map.hxx>

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
public:
    //! Constructor
    explicit MeshSliceSelector(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                               const Handle(Graphic3d_ClipPlane)&        theSlicePlane,
                               const Handle(RWCae::MeshData)&          theSliceMeshData,
                               NodesMap*                                 theGenNodes,
                               ElementsMap*                              theGenElems,
                               std::vector<ViewCaeGeom::MeshSliceLine>*   theLines)
        : myMeshGeom(theMeshGeom),
          mySlicePlane(theSlicePlane),
          mySliceMeshData(theSliceMeshData),
          myGenNodes(theGenNodes),
          myGenElems(theGenElems),
          myLines(theLines)
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
        return !isBndBoxAccepted(aBox);
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const ViewCaeGeom::MeshBvhData::MeshBvhBoxSet::SubBoxSet* theBoxSet,
                                    const Standard_Integer                                     theIndex,
                                    const Standard_Boolean                                     theIsInside) override
    {
        (void)theIsInside;

        BVH_Box<float, BVH_DIM> aBoxFrom = theBoxSet->Box(theIndex);
        Graphic3d_BndBox3d      aBoxTo(BVH_Vec3d(aBoxFrom.CornerMin()), BVH_Vec3d(aBoxFrom.CornerMax()));
        if (!isBndBoxAccepted(aBoxTo)) {
            return false;
        }

        RWCae::MeshEntityId anIndex = theBoxSet->Element(theIndex);

        const RWCae::MeshData::IndexedElement& anElem = myMeshGeom->GetMeshData()->IndexedElements()[anIndex];

        return processIndexedElement(anElem, anIndex);
    }

protected:
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

private:
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
    myMeshData = new RWCae::MeshData();
    myNodesMap.Clear();
    myElementsMap.Clear();
    myLines.clear();

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        const Handle(RWCae::MeshData)& aParentMeshData = aParent->GetMeshData();
        if (!aParentMeshData.IsNull()) {
            for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(mySlicePlanes); anIter.More(); anIter.Next()) {
                MeshSliceSelector aSelector(aParent, anIter.Value(), myMeshData, &myNodesMap, &myElementsMap, &myLines);
                aSelector.SetBVHSet(myBvhData->BvhBoxSet(aParent).get());
                aSelector.Select();
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

#include <ViewCaeGeom/MeshGeometrySlice.h>

#include <ViewCaeGeom/MeshGeometryTools.h>

#include <RWCae/MeshAdaptorsIterator.h>

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
                               ElementsMap*                              theGenElems)
        : myMeshGeom(theMeshGeom),
          mySlicePlane(theSlicePlane),
          mySliceMeshData(theSliceMeshData),
          myGenNodes(theGenNodes),
          myGenElems(theGenElems)
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
        int aRes = mySlicePlane->ProbeBox(theBox);
        return aRes == Graphic3d_ClipState_On || mySlicePlane->ProbeBoxTouch(theBox);
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

        return RWCae::MeshAdaptorsIterator3dElems::Switch(theElem.Type, process, myMeshGeom->GetMeshData());
    }

protected:
    Handle(ViewCaeGeom::MeshGeometry) myMeshGeom;
    Handle(Graphic3d_ClipPlane)        mySlicePlane;
    Handle(RWCae::MeshData)          mySliceMeshData;
    TypedMapOfContours                 myCachedMapOfContours;
    NodesMap*                          myGenNodes;
    ElementsMap*                       myGenElems;
    AddedNodeMap                       myAddedNodes;

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
            bool isIn = this->mySlicePlane->ProbePoint(Graphic3d_Vec4d(aPoint.x(), aPoint.y(), aPoint.z(), 1.0))
                     == Graphic3d_ClipState_In;
            double aDist = this->mySlicePlane->ToPlane().Distance(gp_Pnt(aPoint.x(), aPoint.y(), aPoint.z()))
                         * (isIn ? 1.0 : -1.0);
            return aDist;
        };

        using Cutter = ViewCaeGeom::MeshGeometryTools::MeshElementCutter<Elem_T>;
        Cutter aCutter(theAdaptor, anElem, nodeValueFunc, &myCachedMapOfContours, 0.0, CONFUSION);
        if (!aCutter.Perform()) {
            return false;
        }

        aCutter.AddCutSurf(myMeshGeom, mySliceMeshData, myGenNodes, myGenElems, &myAddedNodes, theLocalIndex);

        return true;
    }
};

}

namespace ViewCaeGeom {

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

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        const Handle(RWCae::MeshData)& aParentMeshData = aParent->GetMeshData();
        if (!aParentMeshData.IsNull()) {
            for (Graphic3d_SequenceOfHClipPlane::Iterator anIter(mySlicePlanes); anIter.More(); anIter.Next()) {
                MeshSliceSelector aSelector(aParent, anIter.Value(), myMeshData, &myNodesMap, &myElementsMap);
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
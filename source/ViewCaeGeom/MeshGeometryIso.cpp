#include <ViewCaeGeom/MeshGeometryIso.h>

#include <ViewCaeGeom/MeshGeometryTools.h>

#include <RWCae/MeshAdaptorsIterator.h>
#include <RWCae/MeshTools.h>

#include <BVH_Traverse.hxx>
#include <NCollection_DoubleMap.hxx>
#include <NCollection_Map.hxx>

namespace {
static constexpr double CONFUSION = 1e-12;

static constexpr const char* NAME_PROP   = "prop";
static constexpr const char* NAME_COMP   = "comp";
static constexpr const char* NAME_VALUES = "values";

using MeshScalarProperty     = RWCae::MeshScalarProperty;
using MeshScalarPropertyBase = RWCae::MeshScalarPropertyBase;
using MeshPropertyEntityInfo = RWCae::MeshPropertyEntityInfo;
using MeshInterpolatedNode   = ViewCaeGeom::MeshInterpolatedNode;
using NodesMap               = ViewCaeGeom::MeshGeometry::NodesMap;
using ElementsMap            = ViewCaeGeom::MeshGeometry::ElementsMap;
using TypedMapOfContours     = ViewCaeGeom::MeshGeometryTools::TypedMapOfContours;
using AddedNodeMap           = ViewCaeGeom::MeshGeometryTools::AddedNodeMap;

//! Selector for mesh elements by its property values.
struct MeshIsoSelector : public ViewCaeGeom::MeshHBvhBoxSetTraverse<ViewCaeGeom::MeshBvhData::MeshBvhValueSet> {
public:
    //! Constructor
    explicit MeshIsoSelector(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom,
                             const Handle(MeshScalarPropertyBase)&     thePropertyIn,
                             double                                    theIsoValue,
                             const Handle(RWCae::MeshData)&          theMeshDataOut,
                             NodesMap*                                 theGenNodes,
                             ElementsMap*                              theGenElems)
        : myMeshGeom(theMeshGeom),
          myPropertyIn(thePropertyIn),
          myMeshDataOut(theMeshDataOut),
          myGenNodes(theGenNodes),
          myGenElems(theGenElems),
          myIsoValue(theIsoValue)
    {
    }

public:
    //! Defines the rules for node rejection by bounding box.
    virtual Standard_Boolean RejectNode(const BVH_Vec1d&  theCornerMin,
                                        const BVH_Vec1d&  theCornerMax,
                                        Standard_Boolean& theIsInside) const override
    {
        theIsInside = false;
        return myIsoValue < theCornerMin - CONFUSION || myIsoValue > theCornerMax + CONFUSION;
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const ViewCaeGeom::MeshBvhData::MeshBvhValueSet::SubBoxSet* theBoxSet,
                                    const Standard_Integer                                       theIndex,
                                    const Standard_Boolean                                       theIsInside) override
    {
        (void)theIsInside;
        const BVH_Box<Standard_Real, 1>& aRange = theBoxSet->Box(theIndex);
        if (myIsoValue < aRange.CornerMin() - CONFUSION || myIsoValue > aRange.CornerMax() + CONFUSION) {
            return false;
        }

        RWCae::MeshEntityId anIndex = theBoxSet->Element(theIndex);

        const RWCae::MeshData::IndexedElement& anElem = myMeshGeom->GetMeshData()->IndexedElements()[anIndex];

        return processIndexedElement(anIndex, anElem);
    }

protected:
    //! Process a separate mesh element.
    virtual bool processIndexedElement(RWCae::MeshEntityId                    theGlobalIndex,
                                       const RWCae::MeshData::IndexedElement& theElem)
    {
        auto process = [this, &theElem, theGlobalIndex](const auto& theAdaptor) {
            return AcceptElementIso(theAdaptor, theGlobalIndex, theElem);
        };

        return RWCae::MeshAdaptorsIterator3dElems::Switch(theElem.Type, process, myMeshGeom->GetMeshData());
    }

protected:
    Handle(ViewCaeGeom::MeshGeometry) myMeshGeom;
    Handle(MeshScalarPropertyBase)     myPropertyIn;
    Handle(RWCae::MeshData)          myMeshDataOut;
    TypedMapOfContours                 myCachedMapOfContours;
    NodesMap*                          myGenNodes;
    ElementsMap*                       myGenElems;
    double                             myIsoValue;
    AddedNodeMap                       myAddedNodes;

private:
    //! Check and process a mesh element.
    template<class Elem_T>
    bool AcceptElementIso(const RWCae::MeshAdaptor<Elem_T>&      theAdaptor,
                          RWCae::MeshEntityId                    theGlobalIndex,
                          const RWCae::MeshData::IndexedElement& theIndexedElem)
    {
        if (!myMeshGeom->AcceptElement(theGlobalIndex)) {
            return false;
        }

        const Elem_T& anElem = theAdaptor.GetElement(theIndexedElem.Index);

        auto nodeValueFunc = [&aPropertyIn = myPropertyIn, &anElem](RWCae::MeshElemEntityId theIdx) -> double {
            return aPropertyIn->Value(MeshPropertyEntityInfo(anElem[theIdx]));
        };

        using Cutter = ViewCaeGeom::MeshGeometryTools::MeshElementCutter<Elem_T>;
        Cutter aCutter(theAdaptor, anElem, nodeValueFunc, &myCachedMapOfContours, myIsoValue, CONFUSION);
        if (!aCutter.Perform()) {
            return false;
        }

        aCutter.AddCutSurf(myMeshGeom, myMeshDataOut, myGenNodes, myGenElems, &myAddedNodes, theIndexedElem.Index);

        return true;
    }
};

}

namespace ViewCaeGeom {

// =======================================================================
// function : MeshGeometryIso
// =======================================================================
MeshGeometryIso::MeshGeometryIso(const Handle(MeshBvhData)& theBvhData) : MeshGeometry(theBvhData)
{
    //
}

// =======================================================================
// function : SetProperty
// =======================================================================
void MeshGeometryIso::SetProperty(const RWCae::MeshPropertyComponent& theProp, bool toClearValues)
{
    myProperty = theProp;
    if (toClearValues) {
        myIsoValues.clear();
    }
    clear();
    SetToUpdate();
}

// =======================================================================
// function : generateMeshData
// =======================================================================
void MeshGeometryIso::generateMeshData()
{
    myMeshData = new RWCae::MeshData();
    myNodesMap.Clear();
    myElementsMap.Clear();

    if (myProperty.Property.IsNull()) {
        return;
    }

    genBaseProperties();

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        const Handle(RWCae::MeshData)& aParentMeshData = aParent->GetMeshData();
        if (!aParentMeshData.IsNull()) {
            for (double aValue : myIsoValues) {
                MeshIsoSelector aSelector(aParent,
                                          myProperties.Find(aParent),
                                          aValue,
                                          myMeshData,
                                          &myNodesMap,
                                          &myElementsMap);
                aSelector.SetBVHSet(myBvhData->BvhValueSet(aParent, myProperties.Find(aParent)).get());
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
Handle(RWCae::MeshNVecProperty) MeshGeometryIso::CalculateProperty(
    const Handle(RWCae::MeshNVecProperty)&  theProp,
    const MeshGeomElem*                       theElem,
    const std::vector<RWCae::MeshEntityId>* theNodes) const
{
    if (theProp.IsNull() || !theProp->IsElemental()) {
        return calculateCommonProperty(theProp, theElem, theNodes);
    }

    Handle(MeshGeometry) aGeom = GetFirstParent();
    if (dynamic_cast<const MeshGeometryBase*>(aGeom.get()) == nullptr) {
        return nullptr;
    }

    RWCae::MeshEntityId anOriginElem;
    if (theElem != nullptr) {
        if (!OriginElement(*theElem, anOriginElem)) {
            return nullptr;
        }
    }

    std::vector<MeshInterpolatedNode> anOriginNodes;
    if (theNodes != nullptr) {
        anOriginNodes.resize(theNodes->size());
        for (size_t aNodeIter = 0; aNodeIter < theNodes->size(); aNodeIter++) {
            if (!OriginNode((*theNodes)[aNodeIter], anOriginNodes[aNodeIter])) {
                return nullptr;
            }
        }
    }

    RWCae::MeshData::EntityMap aNodesMap;

    Handle(RWCae::MeshNVecProperty) aNodalProp =
        new RWCae::MeshNVecProperty(theProp->Name(),
                                      theProp->NbComponents(),
                                      RWCae::MeshPropertyBase::MappingType::Nodal,
                                      theProp->Type());

    aNodalProp->Init<MeshScalarProperty>();

    for (int aCompInd = 0; aCompInd < theProp->NbComponents(); aCompInd++) {
        const Handle(MeshScalarPropertyBase)& aComp = theProp->GetComponent(aCompInd);

        Handle(MeshScalarProperty) aResComp = Handle(MeshScalarProperty)::DownCast(aNodalProp->GetComponent(aCompInd));

        if (theElem != nullptr) {
            genInputNodalPropElem(aGeom, aComp, aResComp, anOriginElem, aNodesMap);
        } else if (theNodes != nullptr) {
            genInputNodalPropNodes(aGeom, aComp, aResComp, anOriginNodes, aNodesMap);
        } else {
            genInputNodalProp(aGeom, aComp, aResComp);
        }
    }

    updateGenPropComponents(theProp, aNodalProp);

    return calculateCommonProperty(aNodalProp, theElem, theNodes, &aNodesMap);
}

// =======================================================================
// function : clear
// =======================================================================
void MeshGeometryIso::clear()
{
    MeshGeometry::clear();

    for (NCollection_DataMap<Handle(MeshGeometry), Handle(MeshScalarPropertyBase)>::Iterator anIter(myProperties);
         anIter.More();
         anIter.Next()) {
        myBvhData->RemoveBvhValueSet(anIter.Key(), anIter.Value());
    }
    myProperties.Clear();
}

// =======================================================================
// function : genBaseProperties
// =======================================================================
void MeshGeometryIso::genBaseProperties()
{
    if (!myProperties.IsEmpty()) {
        return;
    }

    Handle(RWCae::MeshNVecProperty) aBaseProp;
    if (myProperty.IsDefined()) {
        aBaseProp = new RWCae::MeshNVecProperty(myProperty.Property->Name(),
                                                  1,
                                                  RWCae::MeshPropertyBase::MappingType::Nodal);
        if (myProperty.Property->IsElemental()) {
            aBaseProp->Init<MeshScalarProperty>();
            Handle(MeshScalarPropertyBase) aBaseComp = myProperty.GetScalarComp();
            genInputNodalProp(GetFirstParent(),
                              aBaseComp,
                              Handle(MeshScalarProperty)::DownCast(aBaseProp->GetRawComponent(0)));
        } else {
            aBaseProp->ChangeComponents().SetValue(0, myProperty.GetScalarComp());
        }
        aBaseProp->UpdateComponents(false);
    }

    for (MeshGeometry* aParent : MeshGeomParents(myParentGeometry.get())) {
        Handle(MeshScalarPropertyBase)* aGenProp = myProperties.Bound(aParent, Handle(MeshScalarPropertyBase)());

        if (!aParent->GetMeshData().IsNull() && !aBaseProp.IsNull()) {
            (*aGenProp) = aParent->CalculatePropertyComponent(aBaseProp, 0).GetScalarComp();
        }

        if (!aParent->IsModifier()) {
            break;
        }
    }
}

// =======================================================================
// function : genInputNodalProp
// =======================================================================
void MeshGeometryIso::genInputNodalProp(const Handle(MeshGeometry)&           theMeshGeom,
                                        const Handle(MeshScalarPropertyBase)& theProp,
                                        const Handle(MeshScalarProperty)&     theResProp) const
{
    theResProp->Init(theMeshGeom->GetMeshData()->NbNodes(), theProp->ElemType());

    NCollection_Array1<int> aNodesNbValues(0, theMeshGeom->GetMeshData()->NbNodes() - 1);
    aNodesNbValues.Init(0);

    auto compute = [this, &theProp, &theResProp, &aNodesNbValues](const auto& theAdaptor) {
        genInputNodalPropT(theAdaptor, theProp, theResProp, &aNodesNbValues);
    };

    RWCae::MeshAdaptorsIterator3dElems::For(compute, theMeshGeom->GetMeshData());

    for (RWCae::MeshEntityId i = 0; i < theMeshGeom->GetMeshData()->NbNodes(); i++) {
        int aNbValues = aNodesNbValues[i];
        if (aNbValues == 0) {
            continue;
        }

        RWCae::MeshPropertyEntityInfo anInfo(i);
        theResProp->SetValue(anInfo, theResProp->Value(anInfo) / aNbValues, false);
    }
}

// =======================================================================
// function : genInputNodalPropT
// =======================================================================
template<class Elem_T>
void MeshGeometryIso::genInputNodalPropT(const RWCae::MeshAdaptor<Elem_T>&   theAdaptor,
                                         const Handle(MeshScalarPropertyBase)& thePropIn,
                                         const Handle(MeshScalarPropertyBase)& thePropOut,
                                         NCollection_Array1<int>*              theNodesNbValues) const
{
    RWCae::MeshEntityId aNbElems = theAdaptor.NbElements();

    for (RWCae::MeshEntityId anElemIdx = 0; anElemIdx < aNbElems; anElemIdx++) {
        const Elem_T& anElem = theAdaptor.GetElement(anElemIdx);

        for (RWCae::MeshElemEntityId aNodeIndex = 0; aNodeIndex < anElem.NbNodes(); aNodeIndex++) {
            double aValueIn = thePropIn->Value(MeshPropertyEntityInfo(anElem.GlobalIndex(), anElem[aNodeIndex]));
            if (RWCae::IsNaN(aValueIn)) {
                continue;
            }
            double aValueOut = thePropOut->Value(MeshPropertyEntityInfo(anElem[aNodeIndex]));
            thePropOut->SetValue(MeshPropertyEntityInfo(anElem[aNodeIndex]),
                                 !RWCae::IsNaN(aValueOut) ? aValueOut + aValueIn : aValueIn,
                                 false);
            ++theNodesNbValues->ChangeValue(anElem[aNodeIndex]);
        }
    }
}

// =======================================================================
// function : genInputNodalProp
// =======================================================================
void MeshGeometryIso::genInputNodalPropElem(const Handle(MeshGeometry)&           theMeshGeom,
                                            const Handle(MeshScalarPropertyBase)& theProp,
                                            const Handle(MeshScalarProperty)&     theResProp,
                                            const RWCae::MeshEntityId           theElemIdx,
                                            RWCae::MeshData::EntityMap&         theNodesMap) const
{
    theNodesMap.Clear();

    RWCae::MeshData::IndexedElement anIndElem = theMeshGeom->GetMeshData()->IndexedElements()[theElemIdx];

    auto compute = [this, &anIndElem, &theProp, &theResProp, &theMeshGeom, &theNodesMap](const auto& theAdaptor) {
        const auto& anElem = theAdaptor.GetElement(anIndElem.Index);

        RWCae::MeshElemEntityId aNbNodes = anElem.NbNodes();

        theResProp->Init(aNbNodes, theProp->ElemType());

        RWCae::MeshEntityId aNbAddedNodes = 0;
        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < aNbNodes; aNodeIter++) {
            calcNodeAverageValue(theMeshGeom, theProp, theResProp, anElem[aNodeIter], theNodesMap, aNbAddedNodes);
        }

        return true;
    };
    RWCae::MeshAdaptorsIteratorAllElems::Switch(anIndElem.Type, compute, theMeshGeom->GetMeshData());
}

// =======================================================================
// function : genInputNodalProp
// =======================================================================
void MeshGeometryIso::genInputNodalPropNodes(const Handle(MeshGeometry)&              theMeshGeom,
                                             const Handle(MeshScalarPropertyBase)&    theProp,
                                             const Handle(MeshScalarProperty)&        theResProp,
                                             const std::vector<MeshInterpolatedNode>& theNodes,
                                             RWCae::MeshData::EntityMap&            theNodesMap) const
{
    theNodesMap.Clear();

    RWCae::MeshElemEntityId aNbNodes = 0;
    for (const MeshInterpolatedNode& anIntNode : theNodes) {
        aNbNodes += static_cast<RWCae::MeshElemEntityId>(anIntNode.BaseNodes().size());
    }

    theResProp->Init(aNbNodes, theProp->ElemType());

    RWCae::MeshEntityId aNbAddedNodes = 0;
    for (const MeshInterpolatedNode& anIntNode : theNodes) {
        for (const std::pair<RWCae::MeshEntityId, double>& aNode : anIntNode.BaseNodes()) {
            calcNodeAverageValue(theMeshGeom, theProp, theResProp, aNode.first, theNodesMap, aNbAddedNodes);
        }
    }
}

// =======================================================================
// function : calcNodeAverageValue
// =======================================================================
void MeshGeometryIso::calcNodeAverageValue(const Handle(MeshGeometry)&           theMeshGeom,
                                           const Handle(MeshScalarPropertyBase)& theProp,
                                           const Handle(MeshScalarProperty)&     theResProp,
                                           RWCae::MeshEntityId                 theNodeIdx,
                                           RWCae::MeshData::EntityMap&         theNodesMap,
                                           RWCae::MeshEntityId&                theNbAddedNodes) const
{
    RWCae::MeshElemEntityId    aNbAdjElems      = 0;
    RWCae::MeshElemEntityId    aNbAddedAdjElems = 0;
    const RWCae::MeshEntityId* anAdjElems       = theMeshGeom->GetMeshData()->NodeAdjElems(theNodeIdx, aNbAdjElems);

    for (RWCae::MeshElemEntityId anAdjElemIter = 0; anAdjElemIter < aNbAdjElems; anAdjElemIter++) {
        RWCae::MeshElemType aType = theMeshGeom->GetMeshData()->IndexedElements()[anAdjElems[anAdjElemIter]].Type;
        if ((int)aType < RWCae::MeshElem3dLowerType || (int)aType > RWCae::MeshElem3dUpperType) {
            continue;
        }
        double aValueIn = theProp->Value(MeshPropertyEntityInfo(anAdjElems[anAdjElemIter], theNodeIdx));
        if (RWCae::IsNaN(aValueIn)) {
            continue;
        }
        double aValueOut = theResProp->Value(MeshPropertyEntityInfo(theNbAddedNodes));
        aValueOut        = !RWCae::IsNaN(aValueOut) ? aValueOut + aValueIn : aValueIn;
        theResProp->SetValue(MeshPropertyEntityInfo(theNbAddedNodes), aValueOut, false);
        aNbAddedAdjElems++;
    }

    if (aNbAddedAdjElems > 0) {
        double aValueOut = theResProp->Value(MeshPropertyEntityInfo(theNbAddedNodes)) / aNbAddedAdjElems;
        theResProp->SetValue(MeshPropertyEntityInfo(theNbAddedNodes), aValueOut, false);
    }

    theNodesMap.Bind(theNodeIdx, theNbAddedNodes);
    theNbAddedNodes++;
}

// =======================================================================
// function : ToJson
// =======================================================================
void MeshGeometryIso::ToJson(RWBase::JsonHelper::Writer& theJson)
{
    theJson.Key(NAME_PROP);
    theJson.String(myProperty.IsDefined() ? myProperty.Property->Id().ToCString() : "");
    theJson.Key(NAME_COMP);
    theJson.Int(myProperty.ComponentIndex);
    theJson.Key(NAME_VALUES);
    theJson.StartArray();
    for (double aValue : myIsoValues) {
        theJson.Double(aValue);
    }
    theJson.EndArray();
}

// =======================================================================
// function : FromJson
// =======================================================================
bool MeshGeometryIso::FromJson(const RWBase::JsonHelper::Value& theJson,
                               const Handle(RWCae::MeshData)&   theMeshData,
                               int                                theTime,
                               bool&                              theIsChanged)
{
    const RWBase::JsonHelper::Value* aIsoPropIdField   = RWBase::JsonHelper::FindStringMember(theJson, NAME_PROP);
    const RWBase::JsonHelper::Value* aIsoPropCompField = RWBase::JsonHelper::FindIntMember(theJson, NAME_COMP);
    const RWBase::JsonHelper::Value* aIsoValuesField   = RWBase::JsonHelper::FindArrayMember(theJson, NAME_VALUES);
    if (aIsoPropIdField == nullptr || aIsoPropCompField == nullptr || aIsoValuesField == nullptr) {
        return false;
    }

    RWCae::MeshPropertyComponent aCurComp(theMeshData->GetPropertyById(aIsoPropIdField->GetString(), theTime),
                                            aIsoPropCompField->GetInt());
    if (!RWCae::MeshPropertyComponent::IsEqual(aCurComp, myProperty)) {
        SetProperty(aCurComp);
        RWCae::MeshTools::LoadMeshProperty(theMeshData, aCurComp.Property);
        theIsChanged = true;
    }

    std::vector<double> aCurValues = {};
    bool                isChanged  = false;
    for (rapidjson::Value::ConstValueIterator aValuesIter = aIsoValuesField->Begin();
         aValuesIter != aIsoValuesField->End();
         ++aValuesIter) {
        double aValue = aValuesIter->GetDouble();
        aCurValues.push_back(aValue);
        isChanged = isChanged || aCurValues.size() > myIsoValues.size()
                 || !::IsEqual(aValue, myIsoValues[aCurValues.size() - 1]);
    }

    if (isChanged || aCurValues.size() != myIsoValues.size()) {
        SetIsoValues(aCurValues);
        theIsChanged = true;
    }

    return true;
}

} // namespace           
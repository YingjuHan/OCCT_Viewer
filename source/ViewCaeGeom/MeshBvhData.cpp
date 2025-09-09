#include <ViewCaeGeom/MeshBvhData.h>

#include <ViewCaeGeom/MeshGeometry.h>

#include <RWCae/MeshAdaptorsIterator.h>

#include <BVH_LinearBuilder.hxx>

namespace ViewCaeGeom {

// =======================================================================
// function : MeshBvhData
// =======================================================================
MeshBvhData::MeshBvhData()
{
    //
}

// =======================================================================
// function : BvhBoxSet
// =======================================================================
const Handle(MeshBvhData::MeshBvhBoxSet)& MeshBvhData::BvhBoxSet(const Handle(MeshGeometry)& theMeshGeom)
{
    Handle(MeshBvhData::MeshBvhBoxSet)* aBoxSet = myBvhBoxSets.ChangeSeek(theMeshGeom);
    if (aBoxSet == nullptr) {
        Handle(MeshBvhData::MeshBvhBoxSet) aBoxSetToAdd;
        if (getBvhBoxSet(theMeshGeom, aBoxSetToAdd)) {
            aBoxSet = myBvhBoxSets.Bound(theMeshGeom, nullptr);

            myBvhBoxSets.ChangeFind(theMeshGeom) = aBoxSetToAdd;
        } else {
            aBoxSet = myBvhBoxSets.Bound(theMeshGeom, nullptr);
        }
    }

    if (aBoxSet->IsNull()) {
        fillBvhBoxSet(theMeshGeom, *aBoxSet);
    }
    return *aBoxSet;
}

// =======================================================================
// function : RemoveBvhBoxSet
// =======================================================================
void MeshBvhData::RemoveBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom)
{
    myBvhBoxSets.UnBind(theMeshGeom);
}

// =======================================================================
// function : fillBvhBoxSet
// =======================================================================
void MeshBvhData::fillBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom, Handle(MeshBvhBoxSet)& theBvhBoxSet)
{
    static constexpr int BVH_DIM = 3;

    static constexpr RWCae::MeshEntityId MIN_NB_SUBSETS_ELEMS = 10000u;

    theBvhBoxSet = new MeshBvhBoxSet(new BVH_LinearBuilder<Standard_ShortReal, BVH_DIM>());

    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();
    if (aMeshData.IsNull()) {
        return;
    }

    RWCae::MeshEntityId aNbElems = aMeshData->NbIndexedElems();

    int                   aNbThreads     = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
    RWCae::MeshEntityId aSubSetNbElems = std::max(aNbElems / aNbThreads, MIN_NB_SUBSETS_ELEMS);
    int                   aNbSubSets     = Max(Min(aNbElems / aSubSetNbElems, aNbThreads), 1);
    theBvhBoxSet->Init(aNbSubSets, aSubSetNbElems);

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++) {
        const RWCae::MeshData::IndexedElement& anIndElem = aMeshData->IndexedElements()[anElemIter];

        const RWCae::MeshElements& anElems = aMeshData->Elements(anIndElem.Type);

        RWCae::MeshElemEntityOffset aNodesStart = 0;
        RWCae::MeshElemEntityOffset aNodesEnd   = 0;
        anElems.GetElemNodesRange(anIndElem.Index, &aNodesStart, &aNodesEnd);

        BVH_Box<float, BVH_DIM> aBox;
        for (RWCae::MeshElemEntityOffset i = aNodesStart; i < aNodesEnd; i++) {
            RWCae::MeshEntityId aNodeIdx = anElems.Nodes()[static_cast<int>(i)];
            Graphic3d_Vec3        aNode    = aMeshData->Nodes()[aNodeIdx];
            theMeshGeom->ModifyNode(aNodeIdx, aNode);
            aBox.Add(BVH_Vec3f(aNode.x(), aNode.y(), aNode.z()));
        }
        theBvhBoxSet->AddElement(Min(anElemIter / aSubSetNbElems, aNbSubSets - 1), anElemIter, aBox);
    }

    theBvhBoxSet->Update();
}

// =======================================================================
// function : getBvhBoxSet
// =======================================================================
bool MeshBvhData::getBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom, Handle(MeshBvhBoxSet)& theBvhBoxSet)
{
    auto hasChildModifier = [&](const Handle(MeshGeometry)& theGeom) -> bool {
        for (MeshGeometry* aChild : MeshGeomChildren(theGeom->GetChild().get())) {
            if (aChild->IsModifier()) {
                return true;
            }
        }
        return false;
    };

    if (hasChildModifier(theMeshGeom)) {
        return false;
    }

    for (NCollection_DataMap<Handle(MeshGeometry), Handle(MeshBvhBoxSet)>::Iterator anIter(myBvhBoxSets); anIter.More();
         anIter.Next()) {
        if (theMeshGeom->GetMeshData() != anIter.Key()->GetMeshData()) {
            continue;
        }

        if (!hasChildModifier(anIter.Key())) {
            theBvhBoxSet = anIter.ChangeValue();
            return true;
        }
    }

    return false;
}

// =======================================================================
// function : BvhValueSet
// =======================================================================
const Handle(MeshBvhData::MeshBvhValueSet)& MeshBvhData::BvhValueSet(
    const Handle(MeshGeometry)&                    theMeshGeom,
    const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    ValueData                aData     = {theMeshGeom, theProp};
    Handle(MeshBvhValueSet)* aValueSet = myBvhValueSets.ChangeSeek(aData);
    if (aValueSet == nullptr) {
        Handle(MeshBvhData::MeshBvhValueSet) aValueSetToAdd;
        if (getBvhValueSet(theMeshGeom, theProp, aValueSetToAdd)) {
            aValueSet = myBvhValueSets.Bound(aData, nullptr);

            myBvhValueSets.ChangeFind(aData) = aValueSetToAdd;
        } else {
            aValueSet = myBvhValueSets.Bound(aData, nullptr);
        }
    }

    if (aValueSet->IsNull()) {
        fillBvhValueSet(theMeshGeom, theProp, *aValueSet);
    }
    return *aValueSet;
}

// =======================================================================
// function : RemoveBvhValueSet
// =======================================================================
void MeshBvhData::RemoveBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                    const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    ValueData aData = {theMeshGeom, theProp};
    myBvhValueSets.UnBind(aData);
}

// =======================================================================
// function : fillBvhValueSet
// =======================================================================
void MeshBvhData::fillBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                  const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                  Handle(MeshBvhValueSet)&                       theBvhValueSet)
{
    static constexpr int BVH_DIM = 1;

    static constexpr RWCae::MeshEntityId MIN_NB_SUBSETS_ELEMS = 10000u;

    theBvhValueSet = new MeshBvhValueSet();
    theBvhValueSet->SetBuilder(new BVH_LinearBuilder<double, BVH_DIM>());

    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();
    if (aMeshData.IsNull()) {
        return;
    }

    if (theProp.IsNull()) {
        return;
    }

    RWCae::MeshEntityId aNbElems = aMeshData->NbIndexedElems();

    int                   aNbThreads     = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
    RWCae::MeshEntityId aSubSetNbElems = std::max(aNbElems / aNbThreads, MIN_NB_SUBSETS_ELEMS);
    int                   aNbSubSets     = Max(Min(aNbElems / aSubSetNbElems, aNbThreads), 1);
    theBvhValueSet->Init(aNbSubSets, aSubSetNbElems);

    for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbElems; anElemIter++) {
        Bnd_Range aRange;
        fillBvhValueElementRange(aMeshData, anElemIter, theProp, aRange);

        if (!aRange.IsVoid()) {
            double aMin = 0.0;
            double aMax = 0.0;
            aRange.GetBounds(aMin, aMax);
            theBvhValueSet->AddElement(Min(anElemIter / aSubSetNbElems, aNbSubSets - 1),
                                       anElemIter,
                                       BVH_Box<Standard_Real, BVH_DIM>(aMin, aMax));
        }
    }

    theBvhValueSet->Update();
}

// =======================================================================
// function : fillBvhValueElementRange
// =======================================================================
void MeshBvhData::fillBvhValueElementRange(const Handle(RWCae::MeshData)&               theMeshData,
                                           RWCae::MeshEntityId                          theElemIdx,
                                           const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                           Bnd_Range&                                     theRange)
{
    if (theProp->IsElemental() && theProp->BaseType() != RWCae::MeshScalarPropertyBaseType::NodeVec) {
        theRange = getElementalValueRange(theElemIdx, theProp);
    } else {
        const RWCae::MeshData::IndexedElement& anIndElem = theMeshData->IndexedElements()[theElemIdx];
        const RWCae::MeshElements&             anElems   = theMeshData->Elements(anIndElem.Type);

        RWCae::MeshElemEntityOffset aNodesStart = 0;
        RWCae::MeshElemEntityOffset aNodesEnd   = 0;
        anElems.GetElemNodesRange(anIndElem.Index, &aNodesStart, &aNodesEnd);

        if (theProp->IsNodal()) {
            theRange = getNodalValueRange(anElems, aNodesStart, aNodesEnd, theProp);
        } else {
            theRange = getNodeVecValueRange(anElems, theElemIdx, aNodesStart, aNodesEnd, theProp);
        }
    }
}

// =======================================================================
// function : getNodeVecValueRange
// =======================================================================
Bnd_Range MeshBvhData::getNodeVecValueRange(const RWCae::MeshElements&                   theElems,
                                            RWCae::MeshEntityId                          theElemIdx,
                                            RWCae::MeshElemEntityOffset                  theNodeBegin,
                                            RWCae::MeshElemEntityOffset                  theNodeEnd,
                                            const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    Bnd_Range aRange;
    for (RWCae::MeshElemEntityOffset aNodeIter = theNodeBegin; aNodeIter < theNodeEnd; aNodeIter++) {
        RWCae::MeshEntityId aNodeIdx = theElems.Nodes()[static_cast<int>(aNodeIter)];
        double                aValue   = theProp->Value(RWCae::MeshPropertyEntityInfo(theElemIdx, aNodeIdx));
        if (RWCae::IsNaN(aValue)) {
            continue;
        }
        aRange.Add(aValue);
    }
    return aRange;
}

// =======================================================================
// function : getElementalValueRange
// =======================================================================
Bnd_Range MeshBvhData::getElementalValueRange(RWCae::MeshEntityId                          theElemIdx,
                                              const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    Bnd_Range aRange;
    double    aValue = theProp->Value(RWCae::MeshPropertyEntityInfo(theElemIdx));
    if (!RWCae::IsNaN(aValue)) {
        aRange.Add(aValue);
    }
    return aRange;
}

// =======================================================================
// function : getNodalValueRange
// =======================================================================
Bnd_Range MeshBvhData::getNodalValueRange(const RWCae::MeshElements&                   theElems,
                                          RWCae::MeshElemEntityOffset                  theNodeBegin,
                                          RWCae::MeshElemEntityOffset                  theNodeEnd,
                                          const Handle(RWCae::MeshScalarPropertyBase)& theProp)
{
    Bnd_Range aRange;
    for (RWCae::MeshElemEntityOffset aNodeIter = theNodeBegin; aNodeIter < theNodeEnd; aNodeIter++) {
        RWCae::MeshEntityId aNodeIdx = theElems.Nodes()[static_cast<int>(aNodeIter)];
        double                aValue   = theProp->Value(RWCae::MeshPropertyEntityInfo(aNodeIdx));
        if (RWCae::IsNaN(aValue)) {
            continue;
        }
        aRange.Add(aValue);
    }
    return aRange;
}

// =======================================================================
// function : getBvhValueSet
// =======================================================================
bool MeshBvhData::getBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                 const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                 Handle(MeshBvhValueSet)&                       theBvhValueSet)
{
    for (NCollection_DataMap<ValueData, Handle(MeshBvhValueSet), ValueData>::Iterator anIter(myBvhValueSets);
         anIter.More();
         anIter.Next()) {
        if (theMeshGeom->GetMeshData() == anIter.Key().MeshGeom->GetMeshData() && theProp == anIter.Key().Prop) {
            theBvhValueSet = anIter.ChangeValue();
            return true;
        }
    }

    return false;
}

} // namespace ViewCaeGeom
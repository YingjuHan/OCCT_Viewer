
#include <ViewCae/MeshSubEntity.h>

#include <RWCae/MeshAdaptorsIterator.h>

namespace ViewCae {

// ================================================================
// Function : HashCode
// ================================================================
int MeshSubEntity::HashCode(const MeshSubEntity& theKey, const int theUpperBound)
{
    unsigned int aHashCode = 0;
    aHashCode ^= ::HashCode((int)theKey.myEntType, theUpperBound);
    if (theKey.myEntType != MeshSubEntityType::Presentation) {
        aHashCode ^= ::HashCode(theKey.myRefMeshPrs, theUpperBound);
        aHashCode ^= ::HashCode(theKey.myRefMeshGeom, theUpperBound);
        aHashCode ^= ::HashCode(theKey.myIndex, theUpperBound);
        aHashCode ^= ::HashCode((int)theKey.myElemType, theUpperBound);
        aHashCode ^= ::HashCode(theKey.myIsElemSurf, theUpperBound);
    }
    return (static_cast<int>(aHashCode & 0x7fffffff) % theUpperBound) + 1;
}

// ================================================================
// Function : IsEqual
// ================================================================
bool MeshSubEntity::IsEqual(const MeshSubEntity& theKey1, const MeshSubEntity& theKey2)
{
    if (theKey1.myRefMeshPrs != theKey2.myRefMeshPrs || theKey1.myEntType != theKey2.myEntType) {
        return false;
    }
    if (theKey1.myEntType == MeshSubEntityType::Presentation) {
        return true;
    }

    return theKey1.myRefMeshGeom == theKey2.myRefMeshGeom && theKey1.myIndex == theKey2.myIndex
        && theKey1.myElemType == theKey2.myElemType && theKey1.myIsElemSurf == theKey2.myIsElemSurf;
}

// ================================================================
// Function : addElement
// ================================================================
template<class Elem_T>
void MeshSubEntity::addElement(const Handle(ViewCaeGeom::MeshGeometry)& theMeshGeom, const Elem_T& theElem)
{
    const Handle(RWCae::MeshData)& aMeshData = theMeshGeom->GetMeshData();

    std::vector<RWCae::MeshEntityId> aNodes(theElem.NbNodes());
    for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theElem.NbNodes(); aNodeIter++) {
        Graphic3d_Vec3 aNode = aMeshData->Nodes()[theElem[aNodeIter]];
        theMeshGeom->ModifyNode(theElem[aNodeIter], aNode);
        aNodes[aNodeIter] = myRefMeshData->AddNodePosition(aNode);
    }

    Elem_T aNewElem = theElem;
    aNewElem.SetNodes(aNodes.data(), static_cast<RWCae::MeshElemEntityId>(aNodes.size()), false);
    myRefMeshData->AddElement(aNewElem);
}

// ================================================================
// Function : addOriginElem
// ================================================================
void MeshSubEntity::addOriginElem(const Handle(MeshPresentationBase)&       thePrs,
                                  const Handle(ViewCaeGeom::MeshGeometry)& theGeom)
{
    const Handle(ViewCaeGeom::MeshGeometry)& aPrsGeom = thePrs->GetMeshGeometry();

    RWCae::MeshEntityId anIndex;
    if (!theGeom->OriginElement(ViewCaeGeom::MeshGeomElem(myIndex, myElemType, myIsElemSurf), anIndex)) {
        return;
    }

    const Handle(RWCae::MeshData)& aMeshData = aPrsGeom->GetMeshData();

    RWCae::MeshData::IndexedElement anIndElem = aMeshData->IndexedElements()[anIndex];

    auto compute = [this, &aPrsGeom, &anIndElem](const auto& theAdaptor) {
        addElement(aPrsGeom, theAdaptor.GetElement(anIndElem.Index));
        return true;
    };

    RWCae::MeshAdaptorsIteratorAllElems::Switch(anIndElem.Type, compute, aMeshData);

    myIndex      = anIndex;
    myElemType   = anIndElem.Type;
    myIsElemSurf = false;

    addIdProp();

    myRefMeshGeom = aPrsGeom->Id();
}

// ================================================================
// Function : getPntOnNotDefElem
// ================================================================
template<class Elem_T>
void MeshSubEntity::getPntOnNotDefElem(const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                       const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                                       const Elem_T&                             theElem,
                                       const Graphic3d_Vec3&                     thePnt,
                                       Graphic3d_Vec3&                           theIniPnt)
{
    std::vector<RWCae::MeshEntityId> anElemNodesIds;
    theAdaptor.ElementNodes(theElem, anElemNodesIds);

    std::vector<Graphic3d_Vec3> anElemNodes(anElemNodesIds.size());
    std::vector<Graphic3d_Vec3> aDefElemNodes(anElemNodesIds.size());

    for (size_t i = 0; i < anElemNodesIds.size(); i++) {
        theAdaptor.NodePosition(anElemNodes[i], anElemNodesIds[i]);
        aDefElemNodes[i] = anElemNodes[i];
        theGeom->ModifyNode(anElemNodesIds[i], aDefElemNodes[i]);
    }

    RWCae::MeshElemEntityId aNbFaces = theAdaptor.NbFaces(theElem);
    for (RWCae::MeshElemEntityId aFaceIter = 0; aFaceIter < aNbFaces; aFaceIter++) {
        for (typename RWCae::MeshAdaptor<Elem_T>::TrianglesIterator aTrgIter(theAdaptor, theElem, aFaceIter);
             aTrgIter.More();
             aTrgIter.Next()) {
            const Graphic3d_Vec3ub& aTrg = aTrgIter.TriangleIndices();

            Graphic3d_Vec3 aDir  = thePnt - aDefElemNodes[aTrg.x()];
            Graphic3d_Vec3 aDir1 = aDefElemNodes[aTrg.y()] - aDefElemNodes[aTrg.x()];
            Graphic3d_Vec3 aDir2 = aDefElemNodes[aTrg.z()] - aDefElemNodes[aTrg.x()];

            float aD00   = aDir1.Dot(aDir1);
            float aD01   = aDir1.Dot(aDir2);
            float aD11   = aDir2.Dot(aDir2);
            float aD20   = aDir.Dot(aDir1);
            float aD21   = aDir.Dot(aDir2);
            float aDenom = aD00 * aD11 - aD01 * aD01;

            if (aDenom < ShortRealSmall()) {
                continue;
            }

            float aCoord1 = (aD11 * aD20 - aD01 * aD21) / aDenom;
            float aCoord2 = (aD00 * aD21 - aD01 * aD20) / aDenom;
            if (aCoord1 < -1e-3f || aCoord2 < -1e-3f || aCoord1 + aCoord2 > 1.0f + 1e-3f) {
                continue;
            }

            Graphic3d_Vec3 aDisp1 = aDefElemNodes[aTrg.x()] - anElemNodes[aTrg.x()];
            Graphic3d_Vec3 aDisp2 = aDefElemNodes[aTrg.y()] - anElemNodes[aTrg.y()];
            Graphic3d_Vec3 aDisp3 = aDefElemNodes[aTrg.z()] - anElemNodes[aTrg.z()];
            theIniPnt             = thePnt - aDisp1 - (aDisp2 - aDisp1) * aCoord1 - (aDisp3 - aDisp1) * aCoord2;
            return;
        }
    }

    theIniPnt = anElemNodes[0];
}

// ================================================================
// Function : computeValueInPoint
// ================================================================
template<class Elem_T>
bool MeshSubEntity::computeValueInPoint(const Elem_T&                                               theElem,
                                        const std::vector<float>&                                   theWeights,
                                        const std::vector<Handle(RWCae::MeshScalarPropertyBase)>& theComps,
                                        std::vector<double>&                                        theValues)
{
    bool allNan = true;

    for (size_t aCompIter = 0; aCompIter < theComps.size(); aCompIter++) {
        if (theComps[aCompIter].IsNull()) {
            theValues[aCompIter] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }

        if (theComps[aCompIter]->IsElemental()
            && theComps[aCompIter]->BaseType() != RWCae::MeshScalarPropertyBaseType::NodeVec) {
            theValues[aCompIter] = theComps[aCompIter]->Value(RWCae::MeshPropertyEntityInfo(0));

            allNan = allNan && RWCae::IsNaN(theValues[aCompIter]);
            continue;
        }

        for (RWCae::MeshElemEntityId aNodeIter = 0; aNodeIter < theElem.NbNodes(); aNodeIter++) {
            RWCae::MeshPropertyEntityInfo anInfo(theComps[aCompIter]->IsElemental() ? 0 : aNodeIter,
                                                   theElem[aNodeIter]);
            double aValue = theComps[aCompIter]->Value(anInfo);
            if (RWCae::IsNaN(aValue)) {
                theValues[aCompIter] = std::numeric_limits<double>::quiet_NaN();
                break;
            }

            theValues[aCompIter] += aValue * theWeights[aNodeIter];
        }

        allNan = allNan && RWCae::IsNaN(theValues[aCompIter]);
    }

    return !allNan;
}

// ================================================================
// Function : computeElemPntValues
// ================================================================
template<class Elem_T>
void MeshSubEntity::computeElemPntValues(const Handle(MeshPresentationBase)&       thePrs,
                                         const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                         const RWCae::MeshAdaptor<Elem_T>&       theAdaptor,
                                         RWCae::MeshEntityId                     theIndex,
                                         const Graphic3d_Vec3&                     thePnt)
{
    const Elem_T& anElem = theAdaptor.GetElement(theIndex);

    Graphic3d_Vec3 anIniPnt;
    getPntOnNotDefElem(theGeom, theAdaptor, anElem, thePnt, anIniPnt);

    std::vector<float> aWeights(anElem.NbNodes());

    if (!theAdaptor.ProbePoint(anElem, anIniPnt, aWeights.data())) {
        return;
    }

    bool isGenGeom = theGeom->Type() == ViewCaeGeom::MeshGeometryType::Gen;

    std::vector<RWCae::MeshEntityId> aNodes;

    theAdaptor.ElementNodes(anElem, aNodes);

    ViewCaeGeom::MeshGeomElem aGeomElem(theIndex, theAdaptor.GetElementType(), theAdaptor.IsSurface());

    int anActiveTime = thePrs->ActiveTimeStep();
    for (const Handle(RWCae::MeshNVecProperty)& aProp : thePrs->GetMeshGeometry()->GetMeshData()->Properties()) {
        if (aProp.IsNull() || (!aProp->IsLoaded() && !isGenGeom)
            || (anActiveTime != -1 && aProp->TimeStep() != anActiveTime)) {
            continue;
        }

        std::vector<Handle(RWCae::MeshScalarPropertyBase)> aComps(aProp->NbRawComponents());

        if (theGeom->Type() == ViewCaeGeom::MeshGeometryType::Gen) {
            for (int aCompIter = 0; aCompIter < aProp->NbRawComponents(); aCompIter++) {
                RWCae::MeshPropertyComponent aGeomPropComp =
                    theGeom->CalculatePropertyComponent(aProp, aCompIter, &aGeomElem, &aNodes);
                aComps[aCompIter] = aGeomPropComp.GetScalarComp();
            }
        } else {
            Handle(RWCae::MeshNVecProperty) aGeomProp = theGeom->CalculateProperty(aProp, &aGeomElem, &aNodes);
            for (int aCompIter = 0; !aGeomProp.IsNull() && aCompIter < aGeomProp->NbRawComponents(); aCompIter++) {
                aComps[aCompIter] = aGeomProp->GetRawComponent(aCompIter);
            }
        }

        std::vector<double> aValues(aProp->NbRawComponents(), 0.0);

        if (!computeValueInPoint(anElem, aWeights, aComps, aValues)) {
            continue;
        }

        addNodalProp(aProp, aValues);
    }
}

// ================================================================
// Function : SetPresentationViaElem
// ================================================================
void MeshSubEntity::SetPresentationViaElem(const Handle(MeshPresentationBase)&       thePrs,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                           const Graphic3d_Vec3&                     thePos,
                                           ViewCaeGeom::MeshGeomElem                theElem)
{
    myEntType = MeshSubEntityType::Presentation;
    setElem(thePrs, theGeom, thePos, theElem);
}

// ================================================================
// Function : SetElem
// ================================================================
void MeshSubEntity::SetElem(const Handle(MeshPresentationBase)&       thePrs,
                            const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                            const Graphic3d_Vec3&                     thePos,
                            ViewCaeGeom::MeshGeomElem                theElem)
{
    myEntType = MeshSubEntityType::Element;
    setElem(thePrs, theGeom, thePos, theElem);
}

// ================================================================
// Function : setElem
// ================================================================
void MeshSubEntity::setElem(const Handle(MeshPresentationBase)&       thePrs,
                            const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                            const Graphic3d_Vec3&                     thePos,
                            ViewCaeGeom::MeshGeomElem                theElem)
{
    Handle(ViewCaeGeom::MeshGeometry) aGeom = theGeom;

    myRefMeshPrs  = thePrs->Id();
    myRefMeshData = new RWCae::MeshData();

    myIndex      = theElem.Index;
    myElemType   = theElem.Type;
    myIsElemSurf = theElem.IsSurface;

    myRefMeshData->AddNodePosition(thePos);

    if (aGeom->Type() == ViewCaeGeom::MeshGeometryType::Gen && !IsPresentation()) {
        const std::pair<ViewCaeGeom::MeshGeometry*, ViewCaeGeom::MeshGeomElem>* aParentElem =
            aGeom->GetElementsMap().Seek(theElem);
        if (aParentElem != nullptr && aParentElem->first != nullptr) {
            aGeom        = aParentElem->first;
            myIndex      = aParentElem->second.Index;
            myElemType   = aParentElem->second.Type;
            myIsElemSurf = aParentElem->second.IsSurface;
        }
    }

    myRefMeshGeom = aGeom->Id();

    auto compute = [this, &thePrs, &aGeom, &thePos](const auto& theAdaptor) {
        computeElemPntValues(thePrs, aGeom, theAdaptor, this->myIndex, thePos);
        return true;
    };

    RWCae::MeshAdaptorsIteratorAllElems::Switch(myElemType, compute, aGeom->GetMeshData(), myIsElemSurf);

    if (aGeom->Type() != ViewCaeGeom::MeshGeometryType::Gen && !IsPresentation()) {
        addOriginElem(thePrs, aGeom);
    }
}

// ================================================================
// Function : addOriginNodes
// ================================================================
void MeshSubEntity::addOriginNodes(const Handle(MeshPresentationBase)&       thePrs,
                                   const Handle(ViewCaeGeom::MeshGeometry)& theGeom)
{
    const Handle(ViewCaeGeom::MeshGeometry)& aPrsGeom = thePrs->GetMeshGeometry();

    ViewCaeGeom::MeshInterpolatedNode aNodes;
    if (!theGeom->OriginNode(myIndex, aNodes)) {
        return;
    }

    const Handle(RWCae::MeshData)& aMeshData = aPrsGeom->GetMeshData();

    for (const std::pair<RWCae::MeshEntityId, double>& aNode : aNodes.BaseNodes()) {
        Graphic3d_Vec3 aNodeToAdd = aMeshData->Nodes()[aNode.first];
        aPrsGeom->ModifyNode(aNode.first, aNodeToAdd);
        myRefMeshData->AddNodePosition(aNodeToAdd);

        myIndex = aNode.first;
    }

    addIdProp();

    myRefMeshGeom = aPrsGeom->Id();
}

// ================================================================
// Function : computeNodeValues
// ================================================================
void MeshSubEntity::computeNodeValues(const Handle(MeshPresentationBase)&       thePrs,
                                      const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                      RWCae::MeshEntityId                     theIndex)
{
    std::vector<RWCae::MeshEntityId> aNodes(1, theIndex);

    for (const Handle(RWCae::MeshNVecProperty)& aProp : thePrs->GetMeshGeometry()->GetMeshData()->Properties()) {
        if (!aProp->IsLoaded()) {
            continue;
        }

        std::vector<Handle(RWCae::MeshScalarPropertyBase)> aComps(aProp->NbRawComponents());

        if (theGeom->Type() == ViewCaeGeom::MeshGeometryType::Gen) {
            for (int aCompIter = 0; aCompIter < aProp->NbRawComponents(); aCompIter++) {
                RWCae::MeshPropertyComponent aGeomPropComp =
                    theGeom->CalculatePropertyComponent(aProp, aCompIter, nullptr, &aNodes);
                aComps[aCompIter] = aGeomPropComp.GetScalarComp();
            }
        } else {
            Handle(RWCae::MeshNVecProperty) aGeomProp = theGeom->CalculateProperty(aProp, nullptr, &aNodes);
            for (int aCompIter = 0; !aGeomProp.IsNull() && aCompIter < aGeomProp->NbRawComponents(); aCompIter++) {
                aComps[aCompIter] = aGeomProp->GetRawComponent(aCompIter);
            }
        }

        std::vector<double> aValues(aProp->NbRawComponents(), std::numeric_limits<double>::quiet_NaN());

        bool allNan = true;
        for (size_t aCompIter = 0; aCompIter < aComps.size(); aCompIter++) {
            if (aComps[aCompIter].IsNull() || !aComps[aCompIter]->IsNodal()) {
                continue;
            }
            aValues[aCompIter] = aComps[aCompIter]->Value(RWCae::MeshPropertyEntityInfo(0));
            if (!RWCae::IsNaN(aValues[aCompIter])) {
                allNan = false;
            }
        }

        if (allNan) {
            continue;
        }

        addNodalProp(aProp, aValues);
    }
}

// ================================================================
// Function : SetPresentationViaNode
// ================================================================
void MeshSubEntity::SetPresentationViaNode(const Handle(MeshPresentationBase)&       thePrs,
                                           const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                                           const Graphic3d_Vec3&                     thePos,
                                           int                                       theIndex)
{
    myEntType = MeshSubEntityType::Presentation;
    setNode(thePrs, theGeom, thePos, theIndex);
}

// ================================================================
// Function : SetNode
// ================================================================
void MeshSubEntity::SetNode(const Handle(MeshPresentationBase)&       thePrs,
                            const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                            const Graphic3d_Vec3&                     thePos,
                            int                                       theIndex)
{
    myEntType = MeshSubEntityType::Node;
    setNode(thePrs, theGeom, thePos, theIndex);
}

// ================================================================
// Function : setNode
// ================================================================
void MeshSubEntity::setNode(const Handle(MeshPresentationBase)&       thePrs,
                            const Handle(ViewCaeGeom::MeshGeometry)& theGeom,
                            const Graphic3d_Vec3&                     thePos,
                            int                                       theIndex)
{
    Handle(ViewCaeGeom::MeshGeometry) aGeom = theGeom;

    myRefMeshPrs  = thePrs->Id();
    myRefMeshData = new RWCae::MeshData();

    myIndex = theIndex;

    myRefMeshData->AddNodePosition(thePos);

    if (aGeom->Type() == ViewCaeGeom::MeshGeometryType::Gen && !IsPresentation()) {
        const std::pair<ViewCaeGeom::MeshGeometry*, ViewCaeGeom::MeshInterpolatedNode>* aParentNode =
            aGeom->GetNodesMap().Seek(myIndex);
        if (aParentNode != nullptr && aParentNode->first != nullptr && aParentNode->second.BaseNodes().size() == 1) {
            aGeom   = aParentNode->first;
            myIndex = aParentNode->second.BaseNodes()[0].first;
        }
    }

    myRefMeshGeom = aGeom->Id();

    computeNodeValues(thePrs, aGeom, myIndex);

    if (aGeom->Type() != ViewCaeGeom::MeshGeometryType::Gen && !IsPresentation()) {
        addOriginNodes(thePrs, aGeom);
    }
}

// ================================================================
// Function : SetRefMeshData
// ================================================================
bool MeshSubEntity::SetRefMeshData(const Handle(RWCae::MeshData)& theMeshData)
{
    if (theMeshData.IsNull() || theMeshData->NbNodes() <= 1) {
        return false;
    }

    if (myEntType == MeshSubEntityType::Element) {
        if (theMeshData->NbIndexedElems() == 0) {
            return false;
        }

        RWCae::MeshData::IndexedElement anIndElem = theMeshData->IndexedElements()[0];

        myIndex      = anIndElem.Index;
        myElemType   = anIndElem.Type;
        myIsElemSurf = false;
    } else {
        myIndex = 1;
    }

    myRefMeshData = theMeshData;

    parseIdProp();

    return IsDefined();
}

// ================================================================
// Function : addNodalProp
// ================================================================
void MeshSubEntity::addNodalProp(const Handle(RWCae::MeshNVecProperty)& theProp, const std::vector<double>& theValues)
{
    Handle(RWCae::MeshNVecProperty) aPropToAdd =
        new RWCae::MeshNVecProperty(theProp->Name(),
                                      theProp->NbComponents(),
                                      RWCae::MeshPropertyBase::MappingType::Nodal,
                                      theProp->Type());
    aPropToAdd->SetId(theProp->Id());
    aPropToAdd->SetDescription(theProp->Description());
    aPropToAdd->Init<RWCae::MeshScalarProperty>();

    for (int aCompIter = 0; aCompIter < aPropToAdd->NbRawComponents(); aCompIter++) {
        const Handle(RWCae::MeshScalarPropertyBase)& aBaseComp = theProp->GetRawComponent(aCompIter);
        const Handle(RWCae::MeshScalarPropertyBase)& aComp     = aPropToAdd->GetRawComponent(aCompIter);
        aComp->SetId(aBaseComp->Id());
        aComp->SetName(aBaseComp->Name());
        aComp->SetDescription(aBaseComp->Description());
        aComp->SetDisplayUnits(aBaseComp->DisplayUnits());

        Handle(RWCae::MeshScalarProperty)::DownCast(aComp)->Init(1);
        aComp->SetValue(RWCae::MeshPropertyEntityInfo(0), theValues[aCompIter]);
    }

    aPropToAdd->UpdateComponents(false);

    myRefMeshData->ChangeProperties().Append(aPropToAdd);
}

// ================================================================
// Function : addIdProp
// ================================================================
void MeshSubEntity::addIdProp()
{
    Handle(RWCae::MeshNVecProperty) aProp =
        new RWCae::MeshNVecProperty("", 1, RWCae::MeshPropertyBase::MappingType::Nodal);
    aProp->SetId("entity_id");
    aProp->Init<RWCae::MeshScalarProperty>();

    Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
        aProp->GetRawComponent(0));
    aComp->Init(1);
    aComp->SetValue(RWCae::MeshPropertyEntityInfo(0), (double)myIndex);

    aProp->UpdateComponents(false);

    myRefMeshData->ChangeProperties().Append(aProp);
}

// ================================================================
// Function : parseIdProp
// ================================================================
void MeshSubEntity::parseIdProp()
{
    const Handle(RWCae::MeshNVecProperty)& aProp = myRefMeshData->GetPropertyById("entity_id");
    if (aProp.IsNull()) {
        return;
    }

    myIndex = (int)aProp->GetRawComponent(0)->Value(RWCae::MeshPropertyEntityInfo(0));
}

} // namespace
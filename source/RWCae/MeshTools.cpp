#include <RWCae/MeshTools.h>

#include <RWCae/MeshBinReader.h>
#include <RWCae/MeshBinWriter.h>
#include <RWCae/MeshNodeVecProperty.h>

#include <Units.hxx>

#include <algorithm>

namespace {
//! Hasher for mesh property.
struct MeshPropertyHasher {
    //! Hash value, for Map interface.
    static int HashCode(const Handle(RWCae::MeshNVecProperty)& theProp, const int theUpper)
    {
        unsigned int aHashCode = 0;

        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp->Name(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp->Description(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode((int)theProp->MapType(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode((int)theProp->Type(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp->NbComponents(), theUpper);
        aHashCode = aHashCode ^ (unsigned int)::HashCode(theProp->TimeStep(), theUpper);
        return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
    }

    //! Matching two instances, for Map interface.
    static bool IsEqual(const Handle(RWCae::MeshNVecProperty)& theProp1,
                        const Handle(RWCae::MeshNVecProperty)& theProp2)
    {
        return theProp1->Name() == theProp2->Name() && theProp1->Description() == theProp2->Description()
            && theProp1->MapType() == theProp2->MapType() && theProp1->Type() == theProp2->Type()
            && theProp1->NbComponents() == theProp2->NbComponents()
            && theProp1->ComponentsType() == theProp2->ComponentsType() && theProp1->TimeStep() == theProp2->TimeStep();
    }
};

//! Compute total number of nodes and elements.
void ComputeMeshDataInfo(const RWCae::MeshTools::MeshDataList& theMeshDataList,
                         RWCae::MeshEntityId&                  theNbNodes,
                         RWCae::MeshEntityId&                  theNbElems)
{
    for (const RWCae::MeshTools::MeshDataBlockPair& aMeshDataBlock : theMeshDataList) {
        const Handle(RWCae::MeshData)& aMeshData = aMeshDataBlock.first;
        if (aMeshData.IsNull() || (aMeshData->NbNodes() == 0 && aMeshData->NbIndexedElems() == 0)) {
            continue;
        }

        theNbNodes += aMeshData->NbNodes();
        theNbElems += aMeshData->NbIndexedElems();
    }
}

using CommonProperties =
    NCollection_DataMap<Handle(RWCae::MeshNVecProperty), Handle(RWCae::MeshNVecProperty), MeshPropertyHasher>;

//! Compute and store properties with the same name and type.
void ComputeCommonProperties(const RWCae::MeshTools::MeshDataList& theMeshDataList,
                             const Handle(RWCae::MeshData)&        theMeshData,
                             int                                     theNbNodes,
                             int                                     theNbElems,
                             CommonProperties&                       theProps)
{
    for (const RWCae::MeshTools::MeshDataBlockPair& aMeshDataBlock : theMeshDataList) {
        const Handle(RWCae::MeshData)& aMeshData = aMeshDataBlock.first;
        if (aMeshData.IsNull() || (aMeshData->NbNodes() == 0 && aMeshData->NbIndexedElems() == 0)) {
            continue;
        }

        for (const Handle(RWCae::MeshNVecProperty)& aProp : aMeshData->Properties()) {
            if (theProps.IsBound(aProp)) {
                continue;
            }

            Handle(RWCae::MeshNVecProperty) aPropToStore = aProp->CopyRaw(aProp->IsNodal() ? theNbNodes : theNbElems);
            if (!aPropToStore.IsNull()) {
                aPropToStore->SetId(aProp->Id());
                theMeshData->ChangeProperties().Append(aPropToStore);
                theProps.Bind(aProp, aPropToStore);
            }
        }
    }

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theMeshData->Properties()) {
        aProp->UpdateComponents(false);
    }
}

//! Append elements of mesh data to elements of other mesh data.
template<class Elem_T>
void AddMeshElements(const NCollection_Vector<Elem_T>& theSrcElements,
                     NCollection_Vector<Elem_T>&       theDstElements,
                     Elem_T                            theOffset)
{
    const int aNbElemsToAdd = theSrcElements.Size();
    if (aNbElemsToAdd == 0) {
        return;
    }

    const int aPrevNbElems = theDstElements.Size();

    theDstElements.SetValue(aPrevNbElems + aNbElemsToAdd - 1, Elem_T()); // resize vector
    for (int i = 0; i < aNbElemsToAdd; ++i) {
        theDstElements.SetValue(aPrevNbElems + i, theSrcElements.Value(i) + theOffset);
    }
}

//! Append elements of mesh data to elements of other mesh data.
void AddMeshElements(const Handle(RWCae::MeshData)& theSrcMeshData,
                     const Handle(RWCae::MeshData)& theDstMeshData,
                     RWCae::MeshEntityId            theNodesOffset)
{
    RWCae::MeshEntityId aNbPrevElems  = theDstMeshData->NbIndexedElems();
    RWCae::MeshEntityId aNbElemsToAdd = theSrcMeshData->NbIndexedElems();
    if (aNbElemsToAdd == 0) { return; }

    theDstMeshData->ChangeIndexedElements().SetValue(aNbPrevElems + aNbElemsToAdd - 1,
                                                     RWCae::MeshData::IndexedElement());

    for (int anElemType = 0; anElemType < RWCae::MeshElemNbTypes; anElemType++) {
        RWCae::MeshElemType aType = (RWCae::MeshElemType)anElemType;

        const RWCae::MeshElements& aSrcElements = theSrcMeshData->Elements(aType);
        RWCae::MeshElements&       aDstElements = theDstMeshData->ChangeElements(aType);

        RWCae::MeshEntityId aNbPrevTypeElems  = aDstElements.NbElements();
        RWCae::MeshEntityId aNbTypeElemsToAdd = aSrcElements.NbElements();

        AddMeshElements(aSrcElements.NodesOffsets(),
                        aDstElements.NodesOffsets(),
                        !aDstElements.NodesOffsets().IsEmpty() ? aDstElements.NodesOffsets().Last() : 0);
        AddMeshElements(aSrcElements.Nodes(), aDstElements.Nodes(), theNodesOffset);
        AddMeshElements(aSrcElements.FacesOffsets(),
                        aDstElements.FacesOffsets(),
                        !aDstElements.FacesOffsets().IsEmpty() ? aDstElements.FacesOffsets().Last() : 0);
        AddMeshElements(aSrcElements.Faces(), aDstElements.Faces(), (RWCae::MeshElemEntityId)0);
        AddMeshElements(aSrcElements.GlobalIndices(), aDstElements.GlobalIndices(), (int)aNbPrevElems);

        for (RWCae::MeshEntityId anElemIter = 0; anElemIter < aNbTypeElemsToAdd; anElemIter++) {
            theDstMeshData->ChangeIndexedElements().SetValue(
                aNbPrevElems + aSrcElements.GlobalIndex(anElemIter),
                RWCae::MeshData::IndexedElement(aType, aNbPrevTypeElems + anElemIter));
        }
    }

    for (auto cell : theSrcMeshData->GenericCells()) {
        for (auto& node : cell.Nodes) { node += theNodesOffset; }
        for (auto& face : cell.Faces) {
            for (auto& node : face) { node += theNodesOffset; }
        }
        theDstMeshData->ChangeGenericCells().push_back(std::move(cell));
    }
    theDstMeshData->SetToUpdateReconstructedSurface();
}

//! Append mesh properties to the existing mesh data.
void AddMeshProperties(const Handle(RWCae::MeshData)& theMeshData,
                       CommonProperties&                thePropsMap,
                       int                              theNodesOffset,
                       int                              theElemsOffset)
{
    auto processComponent = [theNodesOffset,
                             theElemsOffset](const Handle(RWCae::MeshNVecProperty)&       theProp,
                                             const Handle(RWCae::MeshScalarPropertyBase)& theComp,
                                             const Handle(RWCae::MeshNodeVecProperty)&    theNodeVecComp,
                                             const Handle(RWCae::MeshScalarPropertyBase)& theCompToFill) {
        Bnd_Range aRange = theCompToFill->ValueRange();
        aRange.Add(theComp->ValueRange());
        theCompToFill->SetValueRange(aRange);

        for (int aValueIter = 0; aValueIter < theComp->NbValues(); aValueIter++) {
            std::vector<int> anIndices;
            if (!theNodeVecComp.IsNull()) {
                theNodeVecComp->SubIndices(aValueIter, anIndices);
            } else {
                anIndices.push_back(-1);
            }

            for (const int& aNodeIndex : anIndices) {
                RWCae::MeshPropertyEntityInfo anInfoToRead(aValueIter, aNodeIndex);
                RWCae::MeshPropertyEntityInfo anInfoToStore(aValueIter
                                                                  + (theProp->IsNodal() ? theNodesOffset
                                                                                        : theElemsOffset),
                                                              aNodeIndex != -1 ? aNodeIndex + theNodesOffset : -1);
                theCompToFill->SetRawValue(anInfoToStore, theComp->RawValue(anInfoToRead), false);
            }
        }
    };

    for (const Handle(RWCae::MeshNVecProperty)& aProp : theMeshData->Properties()) {
        Handle(RWCae::MeshNVecProperty)* aPropToFill = thePropsMap.ChangeSeek(aProp);
        if (aPropToFill == nullptr) {
            continue;
        }

        for (int aCompIter = 0; aCompIter < aProp->NbRawComponents(); aCompIter++) {
            const Handle(RWCae::MeshScalarPropertyBase)& aComp = aProp->GetRawComponent(aCompIter);
            Handle(RWCae::MeshNodeVecProperty) aNodeVecComp = Handle(RWCae::MeshNodeVecProperty)::DownCast(aComp);

            const Handle(RWCae::MeshScalarPropertyBase)& aCompToFill = (*aPropToFill)->GetRawComponent(aCompIter);

            processComponent(aProp, aComp, aNodeVecComp, aCompToFill);
        }
    }
}

}

namespace RWCae {
// ================================================================
// Function : fillPropertiesRange
// ================================================================
void MeshTools::fillPropertiesRange(MeshNVecProperties& theProperties, PropertyRanges& theRanges)
{
    std::stable_sort(theProperties.begin(), theProperties.end(), MeshNVecProperty::SortByTimestampLesser());
    for (const Handle(MeshNVecProperty)& aProp : theProperties) {
        const int aNbComponents = aProp->NbRawComponents();

        NCollection_Array1<Bnd_Range>* aMinMaxVec = theRanges.ChangeSeek(aProp->Id());
        if (aMinMaxVec == nullptr) {
            aMinMaxVec = theRanges.Bound(aProp->Id(), NCollection_Array1<Bnd_Range>(0, aNbComponents - 1));
        }
        if (aMinMaxVec->Size() != aNbComponents) {
            Message::SendFail("Error: several properties with same IDs but different number of components");
        }
        for (int i = 0; i < aNbComponents; ++i) {
            aMinMaxVec->ChangeValue(i).Add(aProp->GetRawComponent(i)->ValueRange());
        }
    }
}

// ================================================================
// Function : restorePropertiesRange
// ================================================================
void MeshTools::restorePropertiesRange(const MeshNVecProperties& theProperties, const PropertyRanges& theRanges)
{
    for (const Handle(MeshNVecProperty)& aProp : theProperties) {
        const int aNbComponents = aProp->NbRawComponents();

        const NCollection_Array1<Bnd_Range>* aMinMaxVec = theRanges.Seek(aProp->Id());
        if (aMinMaxVec == nullptr) {
            continue;
        }
        if (aMinMaxVec->Size() != aNbComponents) {
            continue;
        }
        for (int i = 0; i < aNbComponents; i++) {
            aProp->GetRawComponent(i)->SetValueRange(aMinMaxVec->Value(i));
        }
    }
}

// ================================================================
// Function : MergeMeshData
// ================================================================
Handle(MeshData) MeshTools::MergeMeshData(const MeshDataList& theMeshDataList, const bool theToWriteProps)
{
    Handle(MeshData) aRes = new MeshData();

    MeshEntityId aNbNodes    = 0;
    MeshEntityId aNbElements = 0;
    ComputeMeshDataInfo(theMeshDataList, aNbNodes, aNbElements);

    CommonProperties aPropsMap;
    if (theToWriteProps) {
        ComputeCommonProperties(theMeshDataList, aRes, aNbNodes, aNbElements, aPropsMap);
    }

    Handle(MeshDataBlock) aBlocks = new MeshDataBlock("Root");

    for (const RWCae::MeshTools::MeshDataBlockPair& aMeshDataBlock : theMeshDataList) {
        const Handle(RWCae::MeshData)& aMeshData = aMeshDataBlock.first;
        if (aMeshData.IsNull() || (aMeshData->NbNodes() == 0 && aMeshData->NbIndexedElems() == 0)) {
            continue;
        }

        Handle(MeshDataBlock) aBlockNode = aBlocks;
        for (const TCollection_AsciiString& aBlockName : aMeshDataBlock.second) {
            aBlockNode = aBlockNode->AppendChild(aBlockName);
        }

        MeshEntityId aNodesOffset  = aRes->NbNodes();
        MeshEntityId anElemsOffset = aRes->NbIndexedElems();

        for (const Graphic3d_Vec3& aNode : aMeshData->Nodes()) {
            aRes->AddNodePosition(aNode);
        }

        AddMeshElements(aMeshData, aRes, aNodesOffset);

        MeshDataRange aRange(aNodesOffset, aRes->NbNodes() - 1, anElemsOffset, aRes->NbIndexedElems() - 1);
        aBlockNode->Range().AppendRange(aRange);

        if (theToWriteProps) {
            AddMeshProperties(aMeshData, aPropsMap, aNodesOffset, anElemsOffset);
        }
    }

    aRes->SetBlocks(aBlocks);

    return aRes;
}

// ================================================================
// Function : ComputeVectorTrsfRatio
// ================================================================
double MeshTools::ComputeVectorTrsfRatio(const Handle(MeshData)&         theMeshData,
                                         const Handle(MeshNVecProperty)& theVecProperty)
{
    double         aTrsfRatio = 1.0;
    const Bnd_Box& aBounds    = theMeshData->CachedMinMax();
    if (aBounds.IsVoid()) {
        return aTrsfRatio;
    }
    gp_Pnt aMin(aBounds.CornerMin());
    gp_Pnt aMax(aBounds.CornerMax());
    gp_XYZ aDims   = aMax.Coord() - aMin.Coord();
    double aMaxDim = ::Max(aDims.X(), ::Max(aDims.Y(), aDims.Z()));

    // theVecProperty always holds the pre-computed magnitude as component with index 0.
    // The magnitude is guaranteed to be non-negative.
    const Handle(MeshScalarPropertyBase)& aComponent  = theVecProperty->GetRawComponent(0);
    const Bnd_Range&                      aValueRange = aComponent->ValueRange();
    Graphic3d_Vec2d                       aRangeVec;
    aValueRange.GetBounds(aRangeVec[0], aRangeVec[1]);

    aTrsfRatio = (aMaxDim > 0.0 && aRangeVec[1] > 0.0) ? aMaxDim / aRangeVec[1] : 1.0;
    return aTrsfRatio;
}

// ================================================================
// Function : LoadMeshProperty
// ================================================================
bool MeshTools::LoadMeshProperty(const Handle(MeshData)& theMeshData, const Handle(MeshNVecProperty)& theProp)
{
    if (theMeshData.IsNull() || theProp.IsNull()) {
        return false;
    }

    if (!theMeshData->HasProperty(theProp)) {
        return false;
    }

    if (theProp->IsLoaded()) {
        return true;
    }

    if (!theProp->CanBeLoaded()) {
        return false;
    }

    MeshBinReader aReader(theMeshData);
    aReader.SetOptions((int)MeshDataImportExportOptions::SKIP_GEOMETRY
                       | (int)MeshDataImportExportOptions::SAVE_PROPERTY_REF_FILE
                       | (int)MeshDataImportExportOptions::PARALLEL_PROCESSING);

    int aTimeStep = theProp->TimeStep();

    Handle(MeshBinIOBase::SharedMapOfAsciiString) aProps = new MeshBinIOBase::SharedMapOfAsciiString();
    aProps->Bind(theProp->Id(), MeshBinIOBase::TimeRange(aTimeStep, aTimeStep));
    aReader.SetPropsToProcess(aProps);

    if (!aReader.Perform(theProp->RefFilePath())) {
        return false;
    }

    return theProp->IsLoaded();
}

// ================================================================
// Function : DumpMeshProperty
// ================================================================
bool MeshTools::DumpMeshProperty(const Handle(MeshData)&         theMeshData,
                                 const Handle(MeshNVecProperty)& theProp,
                                 const TCollection_AsciiString&  theFile)
{
    if (theMeshData.IsNull() || theProp.IsNull()) {
        return false;
    }

    if (!theMeshData->HasProperty(theProp)) {
        return false;
    }

    if (!theProp->IsLoaded()) {
        return false;
    }

    RWCae::MeshBinWriter aWriter(theMeshData);

    aWriter.SetOptions((int)MeshDataImportExportOptions::SKIP_GEOMETRY);

    int aTimeStep = theProp->TimeStep();

    Handle(MeshBinIOBase::SharedMapOfAsciiString) aProps = new MeshBinIOBase::SharedMapOfAsciiString();
    aProps->Bind(theProp->Id(), MeshBinIOBase::TimeRange(aTimeStep, aTimeStep));
    aWriter.SetPropsToProcess(aProps);

    if (!aWriter.Perform(theFile)) {
        return false;
    }

    theProp->SetRefFilePath(theFile);
    for (int i = 0; i < theProp->NbRawComponents(); i++) {
        theProp->GetRawComponent(i)->SetRefFilePath(theFile);
    }

    return true;
}

// ================================================================
// Function : ConvertMeshProperty
// ================================================================
void MeshTools::ConvertMeshProperty(const Handle(MeshNVecProperty)& theProp,
                                    const MeshMappingFieldType      thePropType,
                                    const std::vector<std::string>& theSourceUnits)
{
    TCollection_AsciiString aCompUnits;
    TCollection_AsciiString anAuxUnits;

    const int aNbUnits = static_cast<int>(theSourceUnits.size());

    // Missing units information means nothing to convert
    if (aNbUnits < theProp->NbComponents()) {
        return;
    }

    if (thePropType == RWCae::MeshMappingFieldType::Vector4) {
        // VECTOR4 is a bit strange field with unitless components and "length" magnitude
        anAuxUnits = theSourceUnits[aNbUnits - 1].c_str();
    } else {
        // All component units are supposed to be identical, hence we can take the first item in an array
        aCompUnits = theSourceUnits[0].c_str();
        anAuxUnits = aCompUnits;
    }

    const int aNbAuxComponents = theProp->NbRawComponents() - theProp->NbComponents();
    for (int aCompIter = 0; aCompIter < theProp->NbRawComponents(); aCompIter++) {
        const Handle(RWCae::MeshScalarPropertyBase)& aComp = theProp->GetRawComponent(aCompIter);
        Handle(RWCae::MeshNodeVecProperty) aNodeVecComp    = Handle(RWCae::MeshNodeVecProperty)::DownCast(aComp);
        convertComponent(aComp, aNodeVecComp, aCompIter < aNbAuxComponents ? anAuxUnits : aCompUnits);
    }
}

// ================================================================
// Function : UpdateMeshPropertyUnits
// ================================================================
void MeshTools::UpdateMeshPropertyUnits(const Handle(MeshNVecProperty)& theProp,
                                        const MeshMappingFieldType      thePropType,
                                        const std::vector<std::string>& theComponentUnits)
{
    TCollection_AsciiString aCompUnits;
    TCollection_AsciiString anAuxUnits;

    const int aNbUnits = static_cast<int>(theComponentUnits.size());

    // Missing units information in a mapping file means the property is unitless
    if (aNbUnits < theProp->NbComponents()) {
        return;
    }

    if (thePropType == RWCae::MeshMappingFieldType::Vector4) {
        // VECTOR4 is a bit strange field with unitless components and "length" magnitude
        anAuxUnits = theComponentUnits[aNbUnits - 1].c_str();
    } else {
        // All component units are supposed to be identical, hence we can take the first item in an array
        aCompUnits = theComponentUnits[0].c_str();
        anAuxUnits = aCompUnits;
    }

    // Set display units for auxiliary (auto-generated) property components, such as vector's magnitude,
    // when aCompIter < aNbAuxComponents, else set display units for imported property components
    const int aNbAuxComponents = theProp->NbRawComponents() - theProp->NbComponents();
    for (int aCompIter = 0; aCompIter < theProp->NbRawComponents(); aCompIter++) {
        const Handle(RWCae::MeshScalarPropertyBase)& aComp = theProp->GetRawComponent(aCompIter);
        aComp->SetDisplayUnits(aCompIter < aNbAuxComponents ? anAuxUnits : aCompUnits);
    }
}

// ================================================================
// Function : convertComponent
// ================================================================
void MeshTools::convertComponent(const Handle(MeshScalarPropertyBase)& theComp,
                                 const Handle(MeshNodeVecProperty)&    theNodeVecComp,
                                 const TCollection_AsciiString&        theUnits)
{
    auto convertValue = [](double theVal, const TCollection_AsciiString& theFromUnit) {
        try {
            return Units::ToSI(theVal, theFromUnit.ToCString());
        } catch (const Standard_Failure& theErr) {
            Message::SendFail() << "Units::ToSI(" << theVal << ", " << theFromUnit << ") - conversion FAILED:\n"
                                << theErr;
            return theVal;
        }
    };

    if (theUnits.IsEmpty()) {
        return;
    }

    // Convert value range
    Graphic3d_Vec2d aRangeVec;
    theComp->ValueRange().GetBounds(aRangeVec[0], aRangeVec[1]);
    aRangeVec[0] = convertValue(aRangeVec[0], theUnits);
    aRangeVec[1] = convertValue(aRangeVec[1], theUnits);
    theComp->SetValueRange(Bnd_Range(aRangeVec[0], aRangeVec[1]));

    // Convert values without changing the range
    for (int aValueIter = 0; aValueIter < theComp->NbValues(); aValueIter++) {
        std::vector<int> anIndices;
        if (!theNodeVecComp.IsNull()) {
            theNodeVecComp->SubIndices(aValueIter, anIndices);
        } else {
            anIndices.push_back(-1);
        }

        for (const int& aNodeIndex : anIndices) {
            RWCae::MeshPropertyEntityInfo anInfo(aValueIter, aNodeIndex);
            if (theComp->HasValue(anInfo)) {
                double aValue = convertValue(theComp->Value(anInfo), theUnits);
                theComp->SetValue(anInfo, aValue, false);
            }
        }
    }
}

// ================================================================
// Function : PointElementChecker
// ================================================================
template<class Element_T>
MeshTools::PointElementChecker<Element_T>::PointElementChecker(const MeshAdaptor<Element_T>*            theAdaptor,
                                                               const Element_T&                         theElem,
                                                               const Graphic3d_Vec3&                    thePnt,
                                                               float*                                   theWeights,
                                                               const Handle(NCollection_BaseAllocator)& theAlloc)
    : myAlloc(!theAlloc.IsNull() ? theAlloc : NCollection_BaseAllocator::CommonBaseAllocator()),
      myAdaptor(theAdaptor),
      myElem(&theElem),
      myPnt(thePnt),
      myWeights(theWeights),
      myNodePntVec(NCollection_StdAllocator<Graphic3d_Vec3>(myAlloc)),
      myNodePntVecLength(NCollection_StdAllocator<float>(myAlloc)),
      myCurAlpha1Angles(NCollection_StdAllocator<float>(myAlloc)),
      myCurAlpha2Angles(NCollection_StdAllocator<float>(myAlloc)),
      myCurThetaAngles(NCollection_StdAllocator<float>(myAlloc))
{
    myNbNodes = myElem->NbNodes();
    myNbFaces = myAdaptor->NbFaces(*myElem);

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myWeights[aNodeIter] = 0.0f;
    }
}

// ================================================================
// Function : Perform
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::Perform()
{
    if (!CheckIsInside()) {
        return false;
    }

    if (myIsPlanar && Element_T::Type != MeshElemType::Polyhedron) {
        return PerformPlanar();
    }

    if (!ComputeNodePntVec()) {
        return true;
    }

    for (myCurFace = 0; myCurFace < myNbFaces; myCurFace++) {
        myCurNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, myCurFace);

        ComputeFaceVector();

        if (!ComputeFaceAngles()) {
            continue;
        }

        float aFaceSum = 0.0f;
        for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < myCurNbFaceNodes; aFaceNodeIter++) {
            aFaceSum += (std::tan(myCurAlpha2Angles[(aFaceNodeIter + myCurNbFaceNodes - 1) % myCurNbFaceNodes] * 0.5f)
                         + std::tan(myCurAlpha2Angles[aFaceNodeIter] * 0.5f))
                      / std::tan(myCurThetaAngles[aFaceNodeIter]);
        }

        if (Abs(aFaceSum) < 1e-4f || myNbFaces == 1) {
            if (!ComputePlanePointWeights()) {
                continue;
            }
        }

        for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < myCurNbFaceNodes; aFaceNodeIter++) {
            MeshElemEntityId aNodeIndex = myAdaptor->FaceNodeIndex(*myElem, myCurFace, aFaceNodeIter);
            float            aWeight    = myCurFaceVecLength / myNodePntVecLength[aNodeIndex]
                          * (std::tan(myCurAlpha2Angles[(aFaceNodeIter + myCurNbFaceNodes - 1) % myCurNbFaceNodes]
                                      * 0.5f)
                             + std::tan(myCurAlpha2Angles[aFaceNodeIter] * 0.5f))
                          / std::sin(myCurThetaAngles[aFaceNodeIter]) / aFaceSum;
            myWeights[aNodeIndex] += aWeight;
            myWeightsSum += aWeight;
        }
    }

    if (Abs(myWeightsSum) < ShortRealEpsilon()) {
        return false;
    }

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myWeights[aNodeIter] /= myWeightsSum;
        myWeights[aNodeIter] = Max(0.0f, Min(1.0f, myWeights[aNodeIter]));
    }
    return true;
}

// ================================================================
// Function : CheckIsInside
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::CheckIsInside()
{
    if (myNbFaces == 1) {
        return CheckIsInside2d();
    }
    myCenter = myAdaptor->ElementCenter(*myElem);
    if ((myPnt - myCenter).Modulus() < ShortRealEpsilon()) {
        myIsPlanar = false;
        return true;
    }

    bool isInside = false;

    Graphic3d_Vec3 aNode1;
    Graphic3d_Vec3 aNode2;
    Graphic3d_Vec3 aNode3;

    bool isTetra = Element_T::Type == MeshElemType::Tetrahedron;

    bool toContinue = !isInside || (isInside && myIsPlanar && !isTetra);

    for (MeshElemEntityId aFaceIter = 0; aFaceIter < myNbFaces && toContinue; aFaceIter++) {
        MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, aFaceIter);
        if (aNbFaceNodes == m_n3) {
            Graphic3d_Vec3 aNorm(0.0f);
            myAdaptor->NodePosition(aNode1, myAdaptor->FaceNode(*myElem, aFaceIter, m_n0));
            myAdaptor->NodePosition(aNode2, myAdaptor->FaceNode(*myElem, aFaceIter, m_n1));
            myAdaptor->NodePosition(aNode3, myAdaptor->FaceNode(*myElem, aFaceIter, m_n2));
            isInside = isInside || CheckLineTrgIntersection(myCenter, myPnt, aNode1, aNode2, aNode3, aNorm);
        } else {
            Graphic3d_Vec3 aFaceCenter(0.0f);
            for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < aNbFaceNodes; aFaceNodeIter++) {
                myAdaptor->NodePosition(aNode1, myAdaptor->FaceNode(*myElem, aFaceIter, aFaceNodeIter));
                aFaceCenter += aNode1;
            }
            aFaceCenter /= aNbFaceNodes;

            Graphic3d_Vec3 aFaceNorm(0.0f);
            float          aNormSumLength = 0.0f;
            for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < aNbFaceNodes && toContinue; aFaceNodeIter++) {
                Graphic3d_Vec3 aNorm(0.0f);
                myAdaptor->NodePosition(aNode1, myAdaptor->FaceNode(*myElem, aFaceIter, aFaceNodeIter));
                myAdaptor->NodePosition(aNode2,
                                        myAdaptor->FaceNode(*myElem, aFaceIter, (aFaceNodeIter + 1) % aNbFaceNodes));
                isInside = isInside || CheckLineTrgIntersection(myCenter, myPnt, aFaceCenter, aNode1, aNode2, aNorm);
                aFaceNorm += aNorm;
                aNormSumLength += aNorm.Modulus();

                toContinue = !isInside || (isInside && myIsPlanar && !isTetra);
            }

            myIsPlanar = myIsPlanar && Abs(aFaceNorm.Modulus() - aNormSumLength) < ShortRealEpsilon();
        }

        toContinue = !isInside || (isInside && myIsPlanar && !isTetra);
    }

    return isInside;
}

// ================================================================
// Function : CheckIsInside2d
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::CheckIsInside2d()
{
    myIsPlanar = false;

    Graphic3d_Vec3 aNode1;
    Graphic3d_Vec3 aNode2;
    Graphic3d_Vec3 aNode3;

    for (typename RWCae::MeshAdaptor<Element_T>::TrianglesIterator aTrgIter(*myAdaptor, *myElem, 0); aTrgIter.More();
         aTrgIter.Next()) {
        const Graphic3d_Vec3u& aTrg = aTrgIter.Triangle();

        myAdaptor->NodePosition(aNode1, aTrg.x());
        myAdaptor->NodePosition(aNode2, aTrg.y());
        myAdaptor->NodePosition(aNode3, aTrg.z());

        if (CheckPntTrgIntersection(myPnt - aNode1, aNode2 - aNode1, aNode3 - aNode1)) {
            return true;
        }
    }

    return false;
}

// ================================================================
// Function : CheckFaceNode
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::CheckFaceNode(MeshEntityId     theNodeId,
                                                              MeshElemEntityId theFace,
                                                              MeshElemEntityId theNbFaceNodes)
{
    for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < theNbFaceNodes; aFaceNodeIter++) {
        if (myAdaptor->FaceNode(*myElem, theFace, aFaceNodeIter) == theNodeId) {
            return true;
        }
    }

    return false;
}

// ================================================================
// Function : PerformPlanar
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::PerformPlanar()
{
    Graphic3d_Vec3 aNodePnt;
    Graphic3d_Vec3 aNode1;
    Graphic3d_Vec3 aNode2;
    Graphic3d_Vec3 aNode3;

    float aSum = 0.0f;

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myAdaptor->NodePosition(aNodePnt, (*myElem)[aNodeIter]);

        Graphic3d_Vec3 aPrevNorm;
        Graphic3d_Vec3 aPrevDir;
        bool           aPrevComputed = false;

        for (MeshElemEntityId aFaceIter = 0; aFaceIter < myNbFaces; aFaceIter++) {
            MeshElemEntityId aNbFaceNodes = myAdaptor->NbFaceNodes(*myElem, aFaceIter);
            if (CheckFaceNode((*myElem)[aNodeIter], aFaceIter, aNbFaceNodes)) {
                continue;
            }

            myAdaptor->NodePosition(aNode1, myAdaptor->FaceNode(*myElem, aFaceIter, m_n0));
            myAdaptor->NodePosition(aNode2, myAdaptor->FaceNode(*myElem, aFaceIter, m_n1));
            myAdaptor->NodePosition(aNode3, myAdaptor->FaceNode(*myElem, aFaceIter, m_n2));

            Graphic3d_Vec3 aNorm       = Graphic3d_Vec3::Cross(aNode2 - aNode1, aNode3 - aNode1);
            float          aNormLength = aNorm.Modulus();
            if (aNormLength < ShortRealEpsilon()) {
                continue;
            }
            aNorm /= aNormLength;

            float aWeight = InversedNormPointPlaneDistance(aNodePnt, myPnt, aNode1, aNorm);

            Graphic3d_Vec3 aDir = (aNode1 + aNode2 + aNode3) / m_n3 - aNodePnt;

            if (aPrevComputed) {
                Graphic3d_Vec3 aCross1 = Graphic3d_Vec3::Cross(aDir, aPrevDir);
                Graphic3d_Vec3 aCross2 = Graphic3d_Vec3::Cross(aNorm, aPrevNorm);
                myWeights[aNodeIter]   = aCross1.Dot(aCross2) < 0.0f ? Max(myWeights[aNodeIter], aWeight)
                                                                     : Min(myWeights[aNodeIter], aWeight);
            } else {
                myWeights[aNodeIter] = aWeight;
            }

            aPrevNorm     = aNorm;
            aPrevDir      = aDir;
            aPrevComputed = true;
        }

        myWeights[aNodeIter] = Max(0.0f, Min(myWeights[aNodeIter], 1.0f));
        aSum += myWeights[aNodeIter];
    }

    if (aSum < ShortRealEpsilon()) {
        return false;
    }

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myWeights[aNodeIter] /= aSum;
    }

    return true;
}

// ================================================================
// Function : ComputeNodePntVec
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::ComputeNodePntVec()
{
    myNodePntVec.resize(myNbNodes);
    myNodePntVecLength.resize(myNbNodes);

    Graphic3d_Vec3 aNode;
    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myAdaptor->NodePosition(aNode, (*myElem)[aNodeIter]);
        myNodePntVec[aNodeIter]       = aNode - myPnt;
        myNodePntVecLength[aNodeIter] = myNodePntVec[aNodeIter].Modulus();
        if (myNodePntVecLength[aNodeIter] < ShortRealEpsilon()) {
            myWeights[aNodeIter] = 1.0f;
            return false;
        }
        myNodePntVec[aNodeIter] /= myNodePntVecLength[aNodeIter];
    }
    return true;
}

// ================================================================
// Function : ComputeFaceVector
// ================================================================
template<class Element_T>
void MeshTools::PointElementChecker<Element_T>::ComputeFaceVector()
{
    myCurAlpha1Angles.resize(myCurNbFaceNodes);

    Graphic3d_Vec3 aNodePntVecAcc(0.0f);

    for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < myCurNbFaceNodes; aFaceNodeIter++) {
        MeshElemEntityId aNodeIndex1 = myAdaptor->FaceNodeIndex(*myElem, myCurFace, aFaceNodeIter);
        MeshElemEntityId aNodeIndex2 = myAdaptor->FaceNodeIndex(*myElem,
                                                                myCurFace,
                                                                (aFaceNodeIter + 1) % myCurNbFaceNodes);
        myCurAlpha1Angles[aFaceNodeIter] =
            std::asin(Min((myNodePntVec[aNodeIndex2] - myNodePntVec[aNodeIndex1]).Modulus() * 0.5f, 1.0f)) * 2.0f;

        Graphic3d_Vec3 aCross  = Graphic3d_Vec3::Cross(myNodePntVec[aNodeIndex2], myNodePntVec[aNodeIndex1]);
        float          aLength = aCross.Modulus();
        if (aLength > ShortRealEpsilon()) {
            myCurFaceVec += aCross / aLength * myCurAlpha1Angles[aFaceNodeIter] * 0.5f;
        }

        aNodePntVecAcc += myNodePntVec[aNodeIndex1];
    }

    myCurFaceVecLength = myCurFaceVec.Modulus();
    if (myCurFaceVecLength > ShortRealEpsilon()) {
        myCurFaceVec /= myCurFaceVecLength;
    }

    myCurFaceCoeff = 1.0f;
    if (myCurFaceVec.Dot(aNodePntVecAcc) < 0.0f) {
        myCurFaceVec *= -1.0f;
        myCurFaceCoeff = -1.0f;
    }
}

// ================================================================
// Function : ComputeFaceAngles
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::ComputeFaceAngles()
{
    myCurAlpha2Angles.resize(myCurNbFaceNodes);
    myCurThetaAngles.resize(myCurNbFaceNodes);

    for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < myCurNbFaceNodes; aFaceNodeIter++) {
        MeshElemEntityId aNodeIndex1 = myAdaptor->FaceNodeIndex(*myElem, myCurFace, aFaceNodeIter);
        MeshElemEntityId aNodeIndex2 = myAdaptor->FaceNodeIndex(*myElem,
                                                                myCurFace,
                                                                (aFaceNodeIter + 1) % myCurNbFaceNodes);

        Graphic3d_Vec3 aVec1 = Graphic3d_Vec3::Cross(myNodePntVec[aNodeIndex1], myCurFaceVec);
        aVec1.Normalize();
        Graphic3d_Vec3 aVec2 = Graphic3d_Vec3::Cross(myNodePntVec[aNodeIndex2], myCurFaceVec);
        aVec2.Normalize();

        myCurAlpha2Angles[aFaceNodeIter] = std::asin(Min((aVec1 - aVec2).Modulus() * 0.5f, 1.0f)) * 2.0f
                                         * myCurFaceCoeff;
        if (Graphic3d_Vec3::Cross(aVec2, aVec1).Dot(myCurFaceVec) < 0.0f) {
            myCurAlpha2Angles[aFaceNodeIter] *= -1.0f;
        }

        Graphic3d_Vec3 aDiff            = myNodePntVec[aNodeIndex1] - myCurFaceVec;
        myCurThetaAngles[aFaceNodeIter] = std::asin(Min(aDiff.Modulus() * 0.5f, 1.0f)) * 2.0f;

        if (myCurThetaAngles[aFaceNodeIter] < ShortRealEpsilon()) {
            myWeights[aNodeIndex1] += myCurFaceVecLength / myNodePntVecLength[aNodeIndex1];
            myWeightsSum += myCurFaceVecLength / myNodePntVecLength[aNodeIndex1];
            return false;
        }
    }

    return true;
}

// ================================================================
// Function : ComputePlanePointWeights
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::ComputePlanePointWeights()
{
    float aFaceSum = 0.0f;

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myWeights[aNodeIter] = 0.0f;
    }

    float aPrevDot = 0.0f;
    for (MeshElemEntityId aFaceNodeIter = 0; aFaceNodeIter < myCurNbFaceNodes; aFaceNodeIter++) {
        MeshElemEntityId aNodeIndex1 = myAdaptor->FaceNodeIndex(*myElem, myCurFace, aFaceNodeIter);
        MeshElemEntityId aNodeIndex2 = myAdaptor->FaceNodeIndex(*myElem,
                                                                myCurFace,
                                                                (aFaceNodeIter + 1) % myCurNbFaceNodes);
        myWeights[aNodeIndex1] = (std::tan(myCurAlpha1Angles[(aFaceNodeIter + myCurNbFaceNodes - 1) % myCurNbFaceNodes]
                                           * 0.5f)
                                  + std::tan(myCurAlpha1Angles[aFaceNodeIter] * 0.5f))
                               / myNodePntVecLength[aNodeIndex1];
        aFaceSum += myWeights[aNodeIndex1];

        float aDot = Graphic3d_Vec3::Cross(myNodePntVec[aNodeIndex1], myNodePntVec[aNodeIndex2]).Dot(myCurFaceVec);
        if (aDot * aPrevDot < 0.0f) {
            return false;
        }
        aPrevDot = aDot;
    }

    if (Abs(aFaceSum) < ShortRealEpsilon() && myNbFaces > 1) {
        return false;
    }

    for (MeshElemEntityId aNodeIter = 0; aNodeIter < myNbNodes; aNodeIter++) {
        myWeights[aNodeIter] /= aFaceSum;
        myWeights[aNodeIter] = Max(0.0f, Min(1.0f, myWeights[aNodeIter]));
    }
    return true;
}

// ================================================================
// Function : InversedNormPointPlaneDistance
// ================================================================
template<class Element_T>
float MeshTools::PointElementChecker<Element_T>::InversedNormPointPlaneDistance(const Graphic3d_Vec3& theStartPnt,
                                                                                const Graphic3d_Vec3& theDirPnt,
                                                                                const Graphic3d_Vec3& thePlnPnt,
                                                                                const Graphic3d_Vec3& thePlnNorm)
{
    float aStartProj = (theStartPnt - thePlnPnt).Dot(thePlnNorm);
    float aDirProj   = (theStartPnt - theDirPnt).Dot(thePlnNorm);
    return Abs(aStartProj) > ShortRealSmall() ? 1.0f - aDirProj / aStartProj : ShortRealLast();
}

// ================================================================
// Function : CheckPntTrgIntersection
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::CheckPntTrgIntersection(const Graphic3d_Vec3& theDir,
                                                                        const Graphic3d_Vec3& theTrgEdge1,
                                                                        const Graphic3d_Vec3& theTrgEdge2)
{
    float aD00   = theTrgEdge1.Dot(theTrgEdge1);
    float aD01   = theTrgEdge1.Dot(theTrgEdge2);
    float aD11   = theTrgEdge2.Dot(theTrgEdge2);
    float aD20   = theDir.Dot(theTrgEdge1);
    float aD21   = theDir.Dot(theTrgEdge2);
    float aDenom = aD00 * aD11 - aD01 * aD01;

    if (aDenom < ShortRealSmall()) {
        return false;
    }

    float aCoord1 = (aD11 * aD20 - aD01 * aD21) / aDenom;
    float aCoord2 = (aD00 * aD21 - aD01 * aD20) / aDenom;
    return aCoord1 > -1e-3f && aCoord2 > -1e-3f && aCoord1 + aCoord2 < 1.0f + 1e-3f;
}

// ================================================================
// Function : CheckLineTrgIntersection
// ================================================================
template<class Element_T>
bool MeshTools::PointElementChecker<Element_T>::CheckLineTrgIntersection(const Graphic3d_Vec3& theStartPnt,
                                                                         const Graphic3d_Vec3& theEndPnt,
                                                                         const Graphic3d_Vec3& theTrgPnt1,
                                                                         const Graphic3d_Vec3& theTrgPnt2,
                                                                         const Graphic3d_Vec3& theTrgPnt3,
                                                                         Graphic3d_Vec3&       theNorm)
{
    Graphic3d_Vec3 aDir1       = theTrgPnt2 - theTrgPnt1;
    float          aDir1Length = aDir1.Modulus();
    if (aDir1Length < ShortRealEpsilon()) {
        return false;
    }
    Graphic3d_Vec3 aDir2       = theTrgPnt3 - theTrgPnt1;
    float          aDir2Length = aDir2.Modulus();
    if (aDir2Length < ShortRealEpsilon()) {
        return false;
    }

    theNorm           = Graphic3d_Vec3::Cross(aDir1 / aDir1Length, aDir2 / aDir2Length);
    float aNormLength = theNorm.Modulus();
    if (aNormLength < ShortRealEpsilon()) {
        return false;
    }
    theNorm /= aNormLength;

    if (theNorm.Dot(theTrgPnt1 - theStartPnt) < 0.0f)
        theNorm *= -1.0f;

    float aDot = (theEndPnt - theStartPnt).Dot(theNorm);
    if (Abs(aDot) < ShortRealEpsilon()) {
        return false;
    }
    float aDist = (theTrgPnt1 - theStartPnt).Dot(theNorm) / aDot;
    if (aDist < 1.0f - ShortRealEpsilon()) {
        return false;
    }

    Graphic3d_Vec3 aPntOnFace = theStartPnt + (theEndPnt - theStartPnt) * aDist;

    Graphic3d_Vec3 aDir = aPntOnFace - theTrgPnt1;

    return CheckPntTrgIntersection(aDir, aDir1, aDir2);
}

template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElement0d>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementTriangle>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementTriangle2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementQuad>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementQuad2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPolygon>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementTetrahedron>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementTetrahedron2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPyramid>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPyramid2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPrism>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPrism2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementHexahedron>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementHexahedron2nd>;
template RWCae_ExportTemplate class MeshTools::PointElementChecker<MeshElementPolyhedron>;

}

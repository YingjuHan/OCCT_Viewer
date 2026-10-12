#include <RWVtk/VtkUnstructuredDataReader.h>
#include <set>
#include <cmath>

#include <Message.hxx>
#include <Message_Messenger.hxx>

#include <regex>

namespace {
//! Check properties equality by internal fields.
static bool CheckPropsEquality(const RWVtk::VtkBaseXMLReader::DataArray&           theMappedPropData,
                               const RWVtk::VtkUnstructuredDataReader::MappedProp& theMappedProp,

                               RWCae::MeshPropertyBase::MappingType theMappingType,

                               const Handle(RWCae::MeshNVecProperty)& thePropToCheck)
{
    if (theMappedPropData.Name != thePropToCheck->Name() || theMappedProp.Name != thePropToCheck->Description()
        || theMappingType != thePropToCheck->MapType()
        || (int)theMappedProp.Components.size() != thePropToCheck->NbComponents()
        || theMappedProp.Type != thePropToCheck->Type()) {
        return false;
    }

    for (int aCompIndex = 0; aCompIndex < (int)theMappedProp.Components.size(); aCompIndex++) {
        if (theMappedPropData.Components[theMappedProp.Components[aCompIndex]]
            != thePropToCheck->GetComponent(aCompIndex)->Name()) {
            return false;
        }
    }

    return true;
}

}

namespace RWVtk {

// ================================================================
// function : VtkUnstructuredDataReader
// ================================================================
VtkUnstructuredDataReader::VtkUnstructuredDataReader()
    : VtkBaseXMLReader(),
      myMeshData(new RWCae::MeshData()),
      myStoredElements(new std::vector<std::vector<int>>())
{
    //
}

// ================================================================
// function : ~VtkUnstructuredDataReader
// ================================================================
VtkUnstructuredDataReader::~VtkUnstructuredDataReader()
{
    //
}

// ================================================================
// function : SetLoadedMeshData
// ================================================================
void VtkUnstructuredDataReader::SetLoadedMeshData(
    const Handle(RWCae::MeshData)&                      theMeshData,
    const std::shared_ptr<std::vector<std::vector<int>>>& theStoredElements)
{
    myMeshData           = theMeshData;
    myStoredElements     = theStoredElements;
    myToReadMeshGeometry = false;
}

// =======================================================================
// function : perform
// =======================================================================
bool VtkUnstructuredDataReader::perform()
{
    myCurPiece = 0;
    myNodesOffset = 0;
    myTimeStep = myRequestedTimeStep;
    myTimeStamp = myHasExplicitTimeStamp ? myRequestedTimeStamp : (double)std::max(myTimeStep, 0);
    if (myToReadMeshGeometry) {
        myMeshData = new RWCae::MeshData();
    }
    LDOM_Element aFieldDataElement = getFirstElement(myDataElement, "FieldData");
    if (!aFieldDataElement.isNull()) {
        std::vector<LDOM_Element> aDataArrayElements = getChildrenElementsByTagName(aFieldDataElement, "DataArray");
        for (const LDOM_Element& aDataArrayElement : aDataArrayElements) {
            DataArray aDataArray = readDataArray(aDataArrayElement);
            if (aDataArray.Data.IsNull()) { return false; }
            if (aDataArray.Name == "TimeValue") {
                if (aDataArray.Data->NbElements() != 1 || aDataArray.Components.size() != 1
                    || !std::isfinite(aDataArray.Data->GetDouble(0, 0))) {
                    return fail("TimeValue requires one finite scalar tuple");
                }
                if (!myHasExplicitTimeStamp) { myTimeStamp = aDataArray.Data->GetDouble(0, 0); }
                // Time is not an integer index (fractional/negative/large times are legal).
                if (myTimeStep == -1) { myTimeStep = 0; }
            } else if (aDataArray.Name != "TimeValue") {
                warn(std::string("Dataset FieldData '") + aDataArray.Name.ToCString()
                    + "' is validated but not exposed as a nodal/elemental field");
            }
        }
    }

    std::vector<LDOM_Element> aPieceElements = getChildrenElementsByTagName(myDataElement, "Piece");
    if (aPieceElements.empty()) { return fail("VTK dataset has no Piece"); }

    if (myToReadMeshGeometry) {
        myStoredElements->resize(aPieceElements.size());
    } else if (!myStoredElements || myStoredElements->size() != aPieceElements.size()) {
        return fail("VTK time step Piece layout differs from the original mesh");
    }

    for (const LDOM_Element& aPieceElement : aPieceElements) {
        for (LDOM_Node node = aPieceElement.getFirstChild(); !node.isNull(); node = node.getNextSibling()) {
            if (node.getNodeType() != LDOM_Node::ELEMENT_NODE) { continue; }
            const std::string tag = TCollection_AsciiString(node.getNodeName()).ToCString();
            const std::set<std::string> supported = {"Points", "Cells", "PointData", "CellData", "Verts", "Lines", "Polys", "Strips"};
            if (!supported.count(tag)) { return fail("Unsupported VTK Piece section: " + tag); }
        }
        for (const char* association : {"PointData", "CellData"}) {
            std::set<std::string> names;
            const auto section = getFirstElement(aPieceElement, association);
            if (section.isNull()) { continue; }
            for (const auto& element : getChildrenElementsByTagName(section, "DataArray")) {
                const std::string name = TCollection_AsciiString(getXmlAttribute(element, "Name", "name")).ToCString();
                if (name.empty() || !names.insert(name).second) {
                    return fail(std::string("Missing/duplicate VTK field name in ") + association + ": " + name);
                }
            }
        }
        if (!readMeshInfo(aPieceElement)) {
            return false;
        }

        if (myToReadMeshGeometry && !readMeshGeometry(aPieceElement)) {
            return false;
        }

        if (!readMeshProperties(aPieceElement)) {
            return false;
        }

        myNodesOffset = myMeshData->NbNodes();

        myCurPiece++;
    }

    myReport.points = myMeshData->NbNodes();
    myReport.cells = myMeshData->NbIndexedElems();
    myReport.fields = myMeshData->Properties().Size();
    return myReport.errors.empty();
}

// =======================================================================
// function : readMeshInfo
// =======================================================================
bool VtkUnstructuredDataReader::readMeshInfo(const LDOM_Element& theElem)
{
    myNbNodes = readCount(theElem,"NumberOfPoints","numberOfPoints");
    VtkResourceBudget::Consume(myBudget->Nodes,myNbNodes,std::min(myLimits.MaxNodes,(uint64_t)std::numeric_limits<int>::max()),"nodes");
    if (myNbNodes <= 0) {
        return fail(std::string("Invalid number of nodes '") + std::to_string(myNbNodes) + "'");
    }

    myNbProcessed = 0;

    return true;
}

// =======================================================================
// function : readMeshGeometry
// =======================================================================
bool VtkUnstructuredDataReader::readMeshGeometry(const LDOM_Element& theElem)
{
    LDOM_Element aPoints = getFirstElement(theElem, "Points");
    if (aPoints.isNull()) {
        return fail("'Points' element not found");
    }

    if (!parsePoints(aPoints)) {
        return false;
    }

    return true;
}

// =======================================================================
// function : readMeshProperties
// =======================================================================
bool VtkUnstructuredDataReader::readMeshProperties(const LDOM_Element& theElem)
{
    LDOM_Element aPointsProps = getFirstElement(theElem, "PointData");
    if (!aPointsProps.isNull()) {
        if (!parseProperties(aPointsProps, true)) {
            return false;
        }
    }

    LDOM_Element aCellsProps = getFirstElement(theElem, "CellData");
    if (!aCellsProps.isNull()) {
        if (!parseProperties(aCellsProps, false)) {
            return false;
        }
    }

    return true;
}

// =======================================================================
// function : parsePoints
// =======================================================================
bool VtkUnstructuredDataReader::parsePoints(const LDOM_Element& theElem)
{
    LDOM_Element aPointsDataArray = getFirstElement(theElem, "DataArray");
    if (aPointsDataArray.isNull()) {
        return fail("'Points/DataArray' element not found");
    }

    constexpr int aNbPntComp = 3;

    DataArray aDataArray = readDataArray(aPointsDataArray);
    if (aDataArray.Components.size() != aNbPntComp) {
        return fail(std::string("Invalid number of point components '") + std::to_string(aDataArray.Components.size()) + "'");
    }

    if (aDataArray.Data.IsNull()) {
        return fail("Failed to read points data");
    }

    if (aDataArray.Data->NbElements() != myNbNodes) { return fail("Point array length does not match NumberOfPoints"); }

    constexpr int aComp1 = 0;
    constexpr int aComp2 = 1;
    constexpr int aComp3 = 2;

    for (int i = 0; i < myNbNodes; i++) {
        Graphic3d_Vec3 aNode(aDataArray.Data->GetFloat(i, aComp1),
                             aDataArray.Data->GetFloat(i, aComp2),
                             aDataArray.Data->GetFloat(i, aComp3));
        if (!std::isfinite(aNode.x()) || !std::isfinite(aNode.y()) || !std::isfinite(aNode.z())) {
            return fail("VTK point coordinates must be finite and fit the mesh coordinate type");
        }
        myMeshData->AddNodePosition(aNode);
    }

    return true;
}

// =======================================================================
// function : parseElements
// =======================================================================
void VtkUnstructuredDataReader::parseElements(const LDOM_Element& theElem, MeshElements& theMeshElems)
{
    theMeshElems.Connectivity.Reset();
    theMeshElems.Offsets.Reset();
    theMeshElems.Types.Reset();
    theMeshElems.Faces.Reset();
    theMeshElems.FaceOffsets.Reset();

    std::vector<LDOM_Element> aDataArrays = getChildrenElementsByTagName(theElem, "DataArray");
    if (aDataArrays.empty()) {
        return;
    }
    std::set<std::string> names;
    for (const LDOM_Element& anElement : aDataArrays) {
        TCollection_AsciiString aName = getXmlAttribute(anElement, "Name", "name");
        if (!names.insert(aName.ToCString()).second) { fail("Duplicate VTK topology array name"); return; }
        const TCollection_AsciiString type = getXmlAttribute(anElement, "type", "Type");
        if ((aName == "types" && type != "UInt8") || (aName != "types" && type != "Int32" && type != "Int64")) {
            fail("Invalid VTK topology array type"); return;
        }
        if (aName == "connectivity") {
            theMeshElems.Connectivity = readDataArray(anElement);
        } else if (aName == "offsets") {
            theMeshElems.Offsets = readDataArray(anElement);
        } else if (aName == "types") {
            theMeshElems.Types = readDataArray(anElement);
        } else if (aName == "faces") {
            theMeshElems.Faces = readDataArray(anElement);
        } else if (aName == "faceoffsets") {
            theMeshElems.FaceOffsets = readDataArray(anElement);
        } else {
            fail(std::string("Unsupported VTK topology array: ") + aName.ToCString()); return;
        }
    }
}

// =======================================================================
// function : parseProperties
// =======================================================================
bool VtkUnstructuredDataReader::parseProperties(const LDOM_Element& theElem, bool theIsNodal)
{
    std::vector<LDOM_Element> aDataArrays = getChildrenElementsByTagName(theElem, "DataArray");
    if (aDataArrays.empty()) {
        return true;
    }

    static constexpr unsigned int aVectorNbComponents = 3;
    static constexpr unsigned int aCompIdx0           = 0;
    static constexpr unsigned int aCompIdx1           = 1;
    static constexpr unsigned int aCompIdx2           = 2;

    for (const LDOM_Element& anElement : aDataArrays) {
        DataArray aDataArray = readDataArray(anElement);
        if (aDataArray.Data.IsNull()) {
            return fail(std::string("Cannot read property '") + aDataArray.Name.ToCString() + "'");
        }

        const size_t expected = theIsNodal ? static_cast<size_t>(myNbNodes) : (*myStoredElements)[myCurPiece].size();
        if (static_cast<size_t>(aDataArray.Data->NbElements()) != expected) {
            return fail(std::string("Tuple count mismatch in ") + (theIsNodal ? "PointData '" : "CellData '")
                        + aDataArray.Name.ToCString() + "'");
        }
        if (aDataArray.Type == "Int64" || aDataArray.Type == "UInt64") {
            for (int n=0;n<aDataArray.Data->NbElements();++n) {
                for (int c=0;c<(int)aDataArray.Components.size();++c) {
                    if (std::abs(aDataArray.Data->GetDouble(n,c))>9007199254740991.0) {return fail("VTK integer field exceeds exact double range");}
                }
            }
        }

        if (aDataArray.Components.size() == aVectorNbComponents) {
            if (aDataArray.Components[aCompIdx0].IsEmpty()) { aDataArray.Components[aCompIdx0] = "X"; }
            if (aDataArray.Components[aCompIdx1].IsEmpty()) { aDataArray.Components[aCompIdx1] = "Y"; }
            if (aDataArray.Components[aCompIdx2].IsEmpty()) { aDataArray.Components[aCompIdx2] = "Z"; }
        }

        RWCae::MeshNVecPropertyType aDefaultType = RWCae::MeshNVecPropertyType::ScalarSet;
        const TCollection_AsciiString aVectors = getXmlAttribute(theElem, "Vectors", "vectors");
        const TCollection_AsciiString aNormals = getXmlAttribute(theElem, "Normals", "normals");
        const TCollection_AsciiString aTensors = getXmlAttribute(theElem, "Tensors", "tensors");
        if ((!aVectors.IsEmpty() && aDataArray.Name == aVectors) || (!aNormals.IsEmpty() && aDataArray.Name == aNormals)) {
            if (aDataArray.Components.size() != 3) { return fail("VTK vector/normal attribute must have three components"); }
            aDefaultType = RWCae::MeshNVecPropertyType::Vector;
        } else if (!aTensors.IsEmpty() && aDataArray.Name == aTensors) {
            if (aDataArray.Components.size() != 9) { return fail("VTK tensor attribute must have nine components"); }
            aDefaultType = RWCae::MeshNVecPropertyType::Tensor;
        }
        std::vector<MappedProp> aMappedProps = extractMappedProps(aDataArray, theIsNodal, aDefaultType);

        std::vector<Handle(RWCae::MeshNVecProperty)> aProperties(aMappedProps.size());
        RWCae::MeshPropertyBase::MappingType         aMappingType = theIsNodal
                                                                        ? RWCae::MeshPropertyBase::MappingType::Nodal
                                                                        : RWCae::MeshPropertyBase::MappingType::Elemental;
        parseMappedProps(aDataArray, aMappedProps, aMappingType, aProperties);

        for (int i = 0; i < (int)aProperties.size(); i++) {
            const Handle(RWCae::MeshNVecProperty)& aProperty = aProperties[i];
            for (int aCompIter = 0; aCompIter < aProperty->NbComponents(); aCompIter++) {
                RWCae::MeshScalarProperty* aComp = dynamic_cast<RWCae::MeshScalarProperty*>(
                    aProperty->GetComponent(aCompIter).get());

                storePropValues(aComp, aDataArray.Data, aMappedProps[i].Components[aCompIter]);
            }

            if (aProperty->IsVector()) {
                myBudget->Array((uint64_t)aProperty->GetComponent(0)->NbValues()
                    * (aProperty->GetComponent(0)->ElemType() == RWCae::MeshPropertyElemType::Float ? sizeof(float) : sizeof(double)));
            }
            aProperty->UpdateComponents();
        }
    }

    return true;
}

// =======================================================================
// function : parseMappedProps
// =======================================================================
void VtkUnstructuredDataReader::parseMappedProps(
    const VtkBaseXMLReader::DataArray&                        theProp,
    const std::vector<VtkUnstructuredDataReader::MappedProp>& theMappedProps,
    RWCae::MeshPropertyBase::MappingType                    theMappingType,
    std::vector<Handle(RWCae::MeshNVecProperty)>&           theProperties)
{
    for (int i = 0; i < (int)theMappedProps.size(); i++) {
        const MappedProp&                  aMappedProp = theMappedProps[i];
        Handle(RWCae::MeshNVecProperty)& aProp       = theProperties[i];
        TCollection_AsciiString            anId        = aMappedProp.Id;

        for (const Handle(RWCae::MeshNVecProperty)& aStoredProperty : myMeshData->Properties()) {
            if (!CheckPropsEquality(theProp, aMappedProp, theMappingType, aStoredProperty)) {
                continue;
            }

            if (myTimeStep == aStoredProperty->TimeStep()) {
                aProp = aStoredProperty;
                break;
            }

            // If the property ID coming from the mapping is empty, re-use the ID from another time step
            if (anId.IsEmpty()) {
                anId = aStoredProperty->Id();
            }
        }

        if (!aProp.IsNull()) {
            continue;
        }

        aProp = new RWCae::MeshNVecProperty(theProp.Name,
                                              static_cast<int>(aMappedProp.Components.size()),
                                              theMappingType,
                                              aMappedProp.Type);
        aProp->SetDescription(aMappedProp.Name);
        aProp->Init<RWCae::MeshScalarProperty>();
        if (!anId.IsEmpty()) {
            aProp->SetId(anId);
        }
        aProp->SetTime(myTimeStamp, myTimeStep);
        aProp->SetTimeStepDescription(aMappedProp.TimeDesc);
        aProp->SetTimeDisplayUnits(aMappedProp.TimeUnits);
        myMeshData->ChangeProperties().Append(aProp);

        RWCae::MeshPropertyElemType anElemType = theProp.Type == "Float32" ? RWCae::MeshPropertyElemType::Float
                                                                             : RWCae::MeshPropertyElemType::Double;

        for (int aCompIter = 0; aCompIter < aProp->NbComponents(); aCompIter++) {
            aProp->GetComponent(aCompIter)->SetName(theProp.Components[aMappedProp.Components[aCompIter]]);
            aProp->GetComponent(aCompIter)->SetDisplayUnits(aMappedProp.DisplayUnits[aCompIter]);
            Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetComponent(aCompIter))->Init(0, anElemType);
        }

        if (aMappedProp.Type == RWCae::MeshNVecPropertyType::Vector) {
            aProp->GetRawComponent(0)->SetDisplayUnits(aMappedProp.DisplayUnits[0]);
            Handle(RWCae::MeshScalarProperty)::DownCast(aProp->GetRawComponent(0))->Init(0, anElemType);
        }
    }
}

// =======================================================================
// function : extractMappedProps
// =======================================================================
std::vector<VtkUnstructuredDataReader::MappedProp> VtkUnstructuredDataReader::extractMappedProps(
    const VtkBaseXMLReader::DataArray& theProp,
    bool                               theIsNodal,
    RWCae::MeshNVecPropertyType theDefaultType)
{
    std::vector<MappedProp> aRes;

    bool isMapped = false;

    TCollection_AsciiString aTimeUnits;
    TCollection_AsciiString aTimeDesc;
    for (const RWCae::MeshFieldMappingInfo& anInfo : myMappingInfo) {
        if (anInfo.FieldMask == "TIME_STEP_ID") {
            aTimeDesc = anInfo.FieldDescription.c_str();
        } else if (anInfo.FieldMask == "TIME_STEP_VALUE" && anInfo.ComponentUnits.size() == 1) {
            aTimeUnits = anInfo.ComponentUnits[0].c_str();
        }
    }

    for (const RWCae::MeshFieldMappingInfo& anInfo : myMappingInfo) {
        std::regex aPropRegex(anInfo.FieldMask);
        if (!std::regex_match(theProp.Name.ToCString(), aPropRegex)) {
            continue;
        }

        // Filter out fields with non-matching calculation point
        if (anInfo.CalcPoint != RWCae::MeshMappingCalcPoint::Unknown
            && anInfo.CalcPoint
                   != (theIsNodal ? RWCae::MeshMappingCalcPoint::Nodes
                                  : RWCae::MeshMappingCalcPoint::ElementCentroids)) {
            continue;
        }

        aRes.push_back(MappedProp());
        MappedProp& aMapperProp = aRes.back();

        if (!extractMappedProp(theProp, anInfo, aMapperProp)) {
            warn(std::string("Failed to parse mapping for property '") + anInfo.FieldMask + "'");
            aRes.erase(aRes.end() - 1);
            continue;
        }

        aMapperProp.TimeDesc  = aTimeDesc;
        aMapperProp.TimeUnits = aTimeUnits;

        isMapped = true;
    }

    if (!isMapped) {
        aRes.push_back(MappedProp());
        aRes[0].Name = theProp.Name;
        aRes[0].Components.resize(theProp.Components.size());
        for (int i = 0; i < (int)aRes[0].Components.size(); i++) {
            aRes[0].Components[i] = i;
        }
        aRes[0].DisplayUnits.resize(theProp.Components.size(), "");
        aRes[0].Type = theDefaultType;
    }

    return aRes;
}

// =======================================================================
// function : extractMappedProp
// =======================================================================
bool VtkUnstructuredDataReader::extractMappedProp(const VtkBaseXMLReader::DataArray&     theProp,
                                                  const RWCae::MeshFieldMappingInfo&   theInfo,
                                                  VtkUnstructuredDataReader::MappedProp& theMappedProp)
{
    for (int aMappedCompIdx = 0; aMappedCompIdx < (int)theInfo.ComponentMasks.size(); aMappedCompIdx++) {
        std::regex aCompMaskRegex(theInfo.ComponentMasks[aMappedCompIdx]);
        bool       hasFound = false;
        for (int aBaseCompIdx = 0; aBaseCompIdx < (int)theProp.Components.size(); aBaseCompIdx++) {
            if (!std::regex_match(theProp.Components[aBaseCompIdx].ToCString(), aCompMaskRegex)) {
                continue;
            }

            theMappedProp.Components.push_back(aBaseCompIdx);
            theMappedProp.DisplayUnits.push_back(theInfo.ComponentUnits[aMappedCompIdx].c_str());
            hasFound = true;
            break;
        }

        if (!hasFound) {
            warn(std::string("Cannot find component '") + theInfo.ComponentMasks[aMappedCompIdx] + "' for property '" + theInfo.FieldMask + "'");
            continue;
        }
    }

    bool isOk = true;

    static constexpr unsigned int aVectorNbComponents  = 3;
    static constexpr unsigned int aTensor6NbComponents = 6;
    static constexpr unsigned int aTensor9NbComponents = 9;

    theMappedProp.Id   = theInfo.FieldId.c_str();
    theMappedProp.Name = theInfo.FieldDescription.c_str();
    switch (theInfo.ValueType) {
        case RWCae::MeshMappingFieldType::Scalar:
            theMappedProp.Type = RWCae::MeshNVecPropertyType::ScalarSet;
            isOk               = theMappedProp.Components.size() == 1;
            break;
        case RWCae::MeshMappingFieldType::Set: theMappedProp.Type = RWCae::MeshNVecPropertyType::ScalarSet; break;
        case RWCae::MeshMappingFieldType::Vector:
            theMappedProp.Type = RWCae::MeshNVecPropertyType::Vector;
            isOk               = theMappedProp.Components.size() == aVectorNbComponents;
            break;
        case RWCae::MeshMappingFieldType::Tensor6:
            theMappedProp.Type = RWCae::MeshNVecPropertyType::Tensor;
            isOk               = theMappedProp.Components.size() == aTensor6NbComponents;
            break;
        case RWCae::MeshMappingFieldType::Tensor9:
            theMappedProp.Type = RWCae::MeshNVecPropertyType::Tensor;
            isOk               = theMappedProp.Components.size() == aTensor9NbComponents;
            break;
        case RWCae::MeshMappingFieldType::Vector4:
        case RWCae::MeshMappingFieldType::Unknown: isOk = false; break;
    }

    return isOk;
}

// =======================================================================
// function : storePropValues
// =======================================================================
void VtkUnstructuredDataReader::storePropValues(RWCae::MeshScalarProperty*    theComp,
                                                const Handle(VtkAbstractArray)& theData,
                                                int                             theCompIndex)
{
    const int aNbValues = theData->NbElements();
    int       isNodal   = theComp->IsNodal();
    uint64_t generated = 0;
    for (int i = 0; i < aNbValues; ++i) { generated += isNodal ? 1 : (*myStoredElements)[myCurPiece][i]; }
    const uint64_t bytes = theComp->ElemType() == RWCae::MeshPropertyElemType::Float ? sizeof(float) : sizeof(double);
    if (((uint64_t)theComp->NbValues()+generated)*bytes > myLimits.MaxArrayBytes) { throw std::runtime_error("VTK resource limit: output field bytes"); }
    myBudget->Array(generated*bytes);
    for (int aValueIter = 0; aValueIter < aNbValues; aValueIter++) {
        int aNbGenElems = isNodal ? 1 : (*myStoredElements)[myCurPiece][aValueIter];
        for (int aGenElemIter = 0; aGenElemIter < aNbGenElems; aGenElemIter++) {
            theComp->AppendValue(theData->GetDouble(aValueIter, theCompIndex));
        }
    }
}

// =======================================================================
// function : addTriangle
// =======================================================================
void VtkUnstructuredDataReader::addTriangle(const Handle(VtkAbstractArray)& theConnectivityArray,
                                            int                             theCellBegin,
                                            int                             theElemIndex,
                                            bool                            theIs2nd)
{
    if (!theIs2nd) {
        addElemTriangle(theConnectivityArray->GetInt(theCellBegin, 0) + myNodesOffset,
                        theConnectivityArray->GetInt(theCellBegin + 1, 0) + myNodesOffset,
                        theConnectivityArray->GetInt(theCellBegin + 2, 0) + myNodesOffset,
                        theElemIndex);
    } else {
        RWCae::MeshElementTriangle2nd anElem;
        for (RWCae::MeshElemEntityId i = 0; i < RWCae::MeshElementTriangle2nd::NbFixedNodes; i++) {
            anElem[i] = theConnectivityArray->GetInt(theCellBegin + i, 0) + myNodesOffset;
        }
        myMeshData->AddElement(anElem);
        (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
    }
}

// =======================================================================
// function : addElemTriangle
// =======================================================================
void VtkUnstructuredDataReader::addElemTriangle(int theVert1, int theVert2, int theVert3, int theElemIndex)
{
    RWCae::MeshElementTriangle anElem;
    anElem[0] = theVert1;
    anElem[1] = theVert2;
    anElem[2] = theVert3;
    myMeshData->AddElement(anElem);
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addQuad
// =======================================================================
void VtkUnstructuredDataReader::addQuad(const Handle(VtkAbstractArray)& theConnectivityArray,
                                        int                             theCellBegin,
                                        int                             theElemIndex,
                                        bool                            theIs2nd)
{
    if (!theIs2nd) {
        addElemQuad(theConnectivityArray->GetInt(theCellBegin, 0) + myNodesOffset,
                    theConnectivityArray->GetInt(theCellBegin + 1, 0) + myNodesOffset,
                    theConnectivityArray->GetInt(theCellBegin + 2, 0) + myNodesOffset,
                    theConnectivityArray->GetInt(theCellBegin + 3, 0) + myNodesOffset,
                    theElemIndex);
    } else {
        RWCae::MeshElementQuad2nd anElem;
        for (RWCae::MeshElemEntityId i = 0; i < RWCae::MeshElementQuad2nd::NbFixedNodes; i++) {
            anElem[i] = theConnectivityArray->GetInt(theCellBegin + i, 0) + myNodesOffset;
        }
        myMeshData->AddElement(anElem);
        (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
    }
}

// =======================================================================
// function : addElemQuad
// =======================================================================
void VtkUnstructuredDataReader::addElemQuad(int theVert1, int theVert2, int theVert3, int theVert4, int theElemIndex)
{
    RWCae::MeshElementQuad anElem;
    anElem[0] = theVert1;
    anElem[1] = theVert2;
    anElem[2] = theVert3;
    anElem[3] = theVert4;
    myMeshData->AddElement(anElem);
    (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
}

// =======================================================================
// function : addPolygon
// =======================================================================
void VtkUnstructuredDataReader::addPolygon(const Handle(VtkAbstractArray)& theConnectivityArray,
                                           int                             theCellBegin,
                                           int                             theCellEnd,
                                           int                             theElemIndex)
{
    int aNbNodes = theCellEnd - theCellBegin;

    if (aNbNodes == 3) {
        addTriangle(theConnectivityArray, theCellBegin, theElemIndex, false);
    } else if (aNbNodes == 4) {
        addQuad(theConnectivityArray, theCellBegin, theElemIndex, false);
    } else {
        RWCae::MeshElementPolygon anElem(aNbNodes);
        for (int i = 0; i < aNbNodes; i++) {
            anElem[i] = theConnectivityArray->GetInt(theCellBegin + i, 0) + myNodesOffset;
        }
        myMeshData->AddElement(anElem);
        (*myStoredElements)[myCurPiece][myNbProcessed + theElemIndex]++;
    }
}

}


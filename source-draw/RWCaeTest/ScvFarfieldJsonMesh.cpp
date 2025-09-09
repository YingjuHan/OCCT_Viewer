
#include <RWCaeTest/ScvFarfieldJsonMesh.h>

#include <RWCaeTest/DrawableMeshData.h>

#include <ViewCae/InteractiveCaeMesh.h>

#include <Standard_WarningsDisable.hxx> // NOLINT
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <Standard_WarningsRestore.hxx> // NOLINT

#include <OSD_FileSystem.hxx>
#include <OSD_Path.hxx>
#include <RWGltf_GltfJsonParser.hxx>
#include <ViewerTest.hxx>

#include <unordered_set>

//! Extracts a member of a RapidJSON object.
static const RWGltf_JsonValue* findObjectMember(const RWGltf_JsonValue& theObject, const char* theName)
{
    if (!theObject.IsObject()) {
        return nullptr;
    }

    rapidjson::Document::ConstMemberIterator anIter = theObject.FindMember(theName);
    return anIter != theObject.MemberEnd() ? &anIter->value : nullptr;
}

namespace RWCaeTest {

// =======================================================================
// function : parseFile
// =======================================================================
bool ScvFarfieldJsonMesh::parseFile(std::vector<FarfieldTriplet>& theResult, PropInfo& thePropInfo) const
{
    // Open JSON file.
    const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
    std::shared_ptr<std::istream> aStream     = aFileSystem->OpenIStream(m_fileName.ToCString(),
                                                                     std::ios::in | std::ios::binary);
    if (aStream.get() == nullptr) {
        Message::SendFail() << "Failed to open file '" << m_fileName << "'";
        return false;
    }

    // Parse JSON file.
    rapidjson::Document       aValuesDoc;
    rapidjson::IStreamWrapper aFileStream(*aStream);
    rapidjson::ParseResult    aRes =
        aValuesDoc.ParseStream<rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseFullPrecisionFlag,
                               rapidjson::UTF8<>,
                               rapidjson::IStreamWrapper>(aFileStream);
    if (aRes.IsError()) {
        if (aRes.Code() == rapidjson::kParseErrorDocumentEmpty) {
            Message::SendFail() << "Error: Stream '" << m_fileName << "' is not a JSON document";
            return false;
        }

        TCollection_AsciiString anErrDesc(RWGltf_GltfJsonParser::FormatParseError(aRes.Code()));
        Message::SendFail() << "Error: Stream '" << m_fileName << "' defines invalid JSON document!\n"
                            << anErrDesc << " [at offset " << (int)aRes.Offset() << "].";
        return false;
    }

    // INPUT VALIDATION
    // Check for a table in RapidJSON document
    if (!aValuesDoc.IsObject()) {
        Message::SendFail() << "Error: Toplevel value is not an object.";
        return false;
    }

    const RWGltf_JsonValue* aTableArrayPtr = findObjectMember(aValuesDoc, "table");
    if (aTableArrayPtr == nullptr || !aTableArrayPtr->IsArray()) {
        Message::SendFail() << "Error: 'table' should be an array.";
        return false;
    } else if (aTableArrayPtr->Size() < 3) {
        Message::SendFail() << "Error: Table of values must contain at least 3 columns.";
        return false;
    }

    // Check for presence of necessary columns in the document
    const RWGltf_JsonValue& aTableArray = *aTableArrayPtr;

    // Columns of datapoints in spherical coordinate system.
    // Theta and Phi are angles, Abs value is a radius.
    const RWGltf_JsonValue* aThetaData = nullptr;
    const RWGltf_JsonValue* aPhiData   = nullptr;
    const RWGltf_JsonValue* anAbsData  = nullptr;
    // Property values always start from column 2 (with columns 0 and 1 containing angles).
    // We need to generate property IDs based on the column index starting from "farfield_1".
    // Hence, we calculate the property index as (the column number - PROP_COLUMN_OFFSET).
    static constexpr int PROP_COLUMN_OFFSET = 1;

    for (int anIter = 0; anIter < (int)aTableArrayPtr->Size(); ++anIter) {
        const RWGltf_JsonValue& anIterObj = aTableArray[anIter];
        if (!anIterObj.IsObject()) {
            Message::SendFail() << "Error: Table column " << anIter + 1 << " is not an object.";
            return false;
        }

        const RWGltf_JsonValue* aNameObj = findObjectMember(anIterObj, "name");
        const RWGltf_JsonValue* aUnitObj = findObjectMember(anIterObj, "unit");
        const RWGltf_JsonValue* aDataObj = findObjectMember(anIterObj, "data");
        if (aNameObj == nullptr || !aNameObj->IsString()) {
            Message::SendFail() << "Error: Table column " << anIter + 1
                                << " does not contain 'name' object or it is not a string.";
            return false;
        } else if (aUnitObj == nullptr || !aUnitObj->IsString()) {
            Message::SendFail() << "Error: Table column " << anIter + 1
                                << " does not contain 'unit' object or it is not a string.";
            return false;
        } else if (aDataObj == nullptr || !aDataObj->IsArray()) {
            Message::SendFail() << "Error: Table column " << anIter + 1
                                << " does not contain 'data' object or it is not an array.";
            return false;
        }

        const TCollection_AsciiString aName(aNameObj->GetString());
        TCollection_AsciiString       anLCName(aName);
        anLCName.LowerCase();
        if (anLCName == "theta") {
            if (aThetaData != nullptr) {
                Message::SendFail() << "Error: duplicate theta column.";
                return false;
            }
            aThetaData = aDataObj;
        } else if (anLCName == "phi") {
            if (aPhiData != nullptr) {
                Message::SendFail() << "Error: duplicate phi column.";
                return false;
            }
            aPhiData = aDataObj;
        } else if (aName == m_colName) {
            if (anAbsData != nullptr) {
                Message::SendFail() << "Error: duplicate absolute value column.";
                return false;
            }
            anAbsData             = aDataObj;
            thePropInfo.PropIndex = anIter - PROP_COLUMN_OFFSET;
            thePropInfo.PropUnits = aUnitObj->GetString();
            // Remember to shift the value range for "dB scale" quantities to obtain
            // non-negative rho values when computing Cartesian mesh node coordinates.
            thePropInfo.RangeShift = (thePropInfo.PropUnits.Search("dB") != -1);
        }
    }

    if (aThetaData == nullptr) {
        Message::SendFail() << "Error: 'theta' column is absent in file.";
        return false;
    } else if (aPhiData == nullptr) {
        Message::SendFail() << "Error: 'phi' column is absent in file.";
        return false;
    } else if (anAbsData == nullptr) {
        Message::SendFail() << "Error: '" << m_colName << "' column is absent in file.";
        return false;
    }

    // Check that column sizes of all columns match
    const unsigned aSize = aThetaData->Size();
    if (aPhiData->Size() != aSize || anAbsData->Size() != aSize) {
        Message::SendFail() << "Error: the sizes of columns don't match.";
        return false;
    }

    // Parse all numbers and load entire table into memory
    theResult.reserve(aSize);
    for (unsigned anIter = 0; anIter < aSize; anIter++) {
        if (!(*aThetaData)[anIter].IsDouble() || !(*aPhiData)[anIter].IsDouble() || !(*anAbsData)[anIter].IsDouble()) {
            Message::SendFail() << "Error: Values in table columns must be numbers.";
            return false;
        }

        // Calculated values can be negative.
        // We don't normalize them here because we need to fill the property with the original values.
        const double aParsedAbs = (*anAbsData)[anIter].GetDouble();
        thePropInfo.PropRange.Add(aParsedAbs);

        theResult.push_back({aParsedAbs, (*aThetaData)[anIter].GetDouble(), (*aPhiData)[anIter].GetDouble()});
    }

    // Ensure that datapoints form a regular grid
    std::unordered_set<double> aThetaUniqueValues;
    std::unordered_set<double> aPhiUniqueValues;
    for (const auto& aTri : theResult) {
        aThetaUniqueValues.insert(aTri.Theta);
        aPhiUniqueValues.insert(aTri.Phi);
    }

    // The number of total values must equal to multiplication of the number of unique values.
    if (aSize != aThetaUniqueValues.size() * aPhiUniqueValues.size()) {
        Message::SendFail() << "Error: The angle values in table do not form a regular grid. "
                               "The number of unique theta angles times number of unique phi values "
                               "must equal the count of datapoints.";
        Message::SendFail() << "Actual data:\n"
                            << "  nb data values = '" << aSize << "',\n  nb theta angles = '"
                            << aThetaUniqueValues.size() << "',\n  nb phi angles = '" << aPhiUniqueValues.size() << "'";
        return false;
    }

    // Grid 2D array stride.
    thePropInfo.Stride = aPhiUniqueValues.size();

    // The goal now is to achieve an order of values that corresponds
    // to scanning of primitives in rows that are parallel to the horizon (which is the XOY plane).

    // Perform stable sort by phi
    std::stable_sort(theResult.begin(),
                     theResult.end(),
                     [](const FarfieldTriplet& theLhs, const FarfieldTriplet& theRhs) {
                         return theLhs.Phi < theRhs.Phi;
                     });

    // Perform stable sort by theta
    std::stable_sort(theResult.begin(),
                     theResult.end(),
                     [](const FarfieldTriplet& theLhs, const FarfieldTriplet& theRhs) {
                         return theLhs.Theta < theRhs.Theta;
                     });
    return true;
}

// =======================================================================
// function : createMeshData
// =======================================================================
void ScvFarfieldJsonMesh::createMeshData(Handle(RWCae::MeshData)&          theResult,
                                         const std::vector<FarfieldTriplet>& theParsedValues,
                                         const PropInfo&                     thePropInfo) const
{
    // Parsing and preparation of data is finished, now onto creating a mesh between the datapoints.
    theResult = new RWCae::MeshData();

    // Auxiliary functions.
    const auto getTripletIndex =
        [thePropInfo](uint32_t theThetaIndex, uint32_t thePhiIndex, bool theToWrap = false) -> uint32_t {
        if (theToWrap) {
            // Phi is periodic.
            // Wrap around the phi index to 0 if it is near the stride value.
            // It is assumed that thePhiIndex is no greater than aStride - 1.
            thePhiIndex %= thePropInfo.Stride - 1;
        }
        return static_cast<uint32_t>(theThetaIndex * thePropInfo.Stride + thePhiIndex);
    };

    const auto getTriplet = [&theParsedValues, &getTripletIndex](size_t theThetaIndex,
                                                                 size_t thePhiIndex) -> const FarfieldTriplet& {
        return theParsedValues.at(
            getTripletIndex(static_cast<uint32_t>(theThetaIndex), static_cast<uint32_t>(thePhiIndex), false));
    };

    const auto calculateDatapointPosition = [this, &getTriplet, thePropInfo](const FarfieldTriplet& theTri) -> gp_XYZ {
        if (is2D()) {
            // Display presentation as plane with the theta axis flipped.
            return gp_XYZ{theTri.Phi, M_PI - theTri.Theta, 0.0};
        }

        // Mapping of input values to obtain non-negative rho values and control the 3D mesh size.
        Graphic3d_Vec2d aMinMax(0.0);
        thePropInfo.PropRange.GetBounds(aMinMax[0], aMinMax[1]);
        const double aScale = m_norm / (thePropInfo.RangeShift ? thePropInfo.PropRange.Delta() : aMinMax[1]);
        const double aRho   = (thePropInfo.RangeShift ? theTri.Abs - aMinMax[0] : theTri.Abs) * aScale;

        // Perform conversion from spherical coordinate system to Cartesian coordinate system
        return gp_XYZ{aRho * std::sin(theTri.Theta) * std::cos(theTri.Phi),
                      aRho * std::sin(theTri.Theta) * std::sin(theTri.Phi),
                      aRho * std::cos(theTri.Theta)};
    };

    // Initialize mesh property.
    const size_t aSize = theParsedValues.size();

    Handle(RWCae::MeshNVecProperty) aProp =
        new RWCae::MeshNVecProperty(m_colName,
                                      3,
                                      RWCae::MeshPropertyBase::MappingType::Nodal,
                                      RWCae::MeshNVecPropertyType::ScalarSet);
    aProp->SetId(TCollection_AsciiString("farfield_") + thePropInfo.PropIndex);
    aProp->Init<RWCae::MeshScalarProperty>();
    for (int aCompIter = 0; aCompIter < aProp->NbComponents(); ++aCompIter) {
        Handle(RWCae::MeshScalarProperty) aComp = Handle(RWCae::MeshScalarProperty)::DownCast(
            aProp->GetComponent(aCompIter));
        aComp->Init(static_cast<int>(aSize), RWCae::MeshPropertyElemType::Double);
        TCollection_AsciiString aName(aCompIter == 0 ? m_colName : (aCompIter == 1 ? "Theta" : "Phi"));
        aComp->SetName(aName);
        aComp->SetDescription(aName);
        aComp->SetDisplayUnits(aCompIter == 0 ? thePropInfo.PropUnits : "deg");
        aComp->SetEnabled(aCompIter == 0);
    }

    // Set time step info
    aProp->SetTime(std::numeric_limits<double>::quiet_NaN(), m_timeStep);
    aProp->SetTimeStepDescription(m_timeDescr);

    // Fill MeshData with nodes.
    for (size_t anIter = 0; anIter < aSize; ++anIter) {
        // Reinterpret dataset as a 2D array.
        const uint32_t aThetaIndex = static_cast<uint32_t>(anIter / thePropInfo.Stride);
        const uint32_t aPhiIndex   = static_cast<uint32_t>(anIter % thePropInfo.Stride);

        const FarfieldTriplet aTri = getTriplet(aThetaIndex, aPhiIndex);

        const gp_XYZ aNode = calculateDatapointPosition(aTri);
        theResult->AddNodePosition(aNode);

        RWCae::MeshPropertyEntityInfo anEntInfo(static_cast<int>(anIter));
        aProp->SetRawVector(anEntInfo, &aTri.Abs);
    }

    // Fill element buffer with quads.
    for (size_t anIter = 0; anIter < aSize; ++anIter) {
        // Reinterpret dataset as a 2D array.
        const uint32_t aThetaIndex = static_cast<uint32_t>(anIter / thePropInfo.Stride);
        const uint32_t aPhiIndex   = static_cast<uint32_t>(anIter % thePropInfo.Stride);
        if (aPhiIndex == thePropInfo.Stride - 1)
            continue; // prevent OOB access
        else if (aThetaIndex == (aSize / thePropInfo.Stride - 1))
            break;

        Standard_ASSERT_RAISE(getTripletIndex(aThetaIndex + 1, aPhiIndex + 1) < aSize,
                              "ScvFarfieldJsonMesh: Invalid angle indices.");

        RWCae::MeshEntityId aMeshElemNodeIndices[4]{getTripletIndex(aThetaIndex, aPhiIndex, !is2D()),
                                                      getTripletIndex(aThetaIndex + 1, aPhiIndex, !is2D()),
                                                      getTripletIndex(aThetaIndex + 1, aPhiIndex + 1, !is2D()),
                                                      getTripletIndex(aThetaIndex, aPhiIndex + 1, !is2D())};

        RWCae::MeshElementQuad aQuad(aMeshElemNodeIndices);
        theResult->AddElement(aQuad);
    }

    // Add the property
    aProp->UpdateComponents();
    theResult->ChangeProperties().Append(aProp);
}

// =======================================================================
// function : ScvFarfieldJsonMesh
// =======================================================================
ScvFarfieldJsonMesh::ScvFarfieldJsonMesh()
{
    AddVoidKey(this, "-2d") = [](ScvFarfieldJsonMesh* theCmd, int&) -> bool {
        theCmd->m_prsType = FarfieldPrsType::Type2D;
        return true;
    };
    AddVoidKey(this, "-3d") = [](ScvFarfieldJsonMesh* theCmd, int&) -> bool {
        theCmd->m_prsType = FarfieldPrsType::Type3D;
        return true;
    };
    AddKey(this, double(), "-norm") = [](ScvFarfieldJsonMesh* theCmd, int& /* theArgIter */, double theVal) -> bool {
        if (theVal <= 0) {
            theCmd->Interpretor() << "Syntax error: negative norm";
            return false;
        }
        theCmd->m_norm = theVal;
        return true;
    };
    AddVoidKey(this, "-time") = [](ScvFarfieldJsonMesh* theCmd, int& theArgIter) -> bool {
        return theCmd->parseTime(theArgIter, theCmd->m_timeStep, theCmd->m_timeDescr);
    };
}

// =======================================================================
// function : ParseArgument
// =======================================================================
bool ScvFarfieldJsonMesh::ParseArgument(int& theArgIter)
{
    if (MeshIOCommand::ParseArgument(theArgIter)) {
        return true;
    }

    const char* anArg = Argument(theArgIter);
    if (m_prsName.IsEmpty() && anArg[0] != '\0') {
        m_prsName = anArg;
    } else if (m_fileName.IsEmpty() && anArg[0] != '\0') {
        m_fileName = anArg;
    } else if (m_colName.IsEmpty() && anArg[0] != '\0') {
        m_colName = anArg;
    } else {
        return false;
    }
    return true;
}

// =======================================================================
// function : ParseAllArguments
// =======================================================================
bool ScvFarfieldJsonMesh::ParseAllArguments()
{
    if (!RWCaeTest::MeshIOCommand::ParseAllArguments()) {
        return false;
    }
    if (m_colName.IsEmpty()) {
        *myDI << "Syntax error: wrong number of arguments";
        return false;
    }
    return true;
}

// =======================================================================
// function : parseTime
// =======================================================================
bool ScvFarfieldJsonMesh::parseTime(int& theArgIter, int& theTimeStep, TCollection_AsciiString& theDescription)
{
    if (theArgIter + 1 >= NbArguments()) {
        *myDI << "Syntax error: incorrect time step option";
        return false;
    }

    TCollection_AsciiString anOption(Argument(++theArgIter));
    anOption.LowerCase();
    if (anOption == "no") {
        return true;
    }

    int anIndex = -1;
    if (anOption == "filename") {
        // extract time step index and description from JSON file name
        TCollection_AsciiString aFileFolder;
        TCollection_AsciiString aFileName;
        OSD_Path::FolderAndFileFromPath(m_fileName, aFileFolder, aFileName);

        // expected structure of far field JSON file name:
        // prefix_<time_step_index>_<time_step_description>_suffix.json
        TCollection_AsciiString anIndexStr   = aFileName.Token("_", 2);
        TCollection_AsciiString aDescription = aFileName.Token("_", 3);
        if (!Draw::ParseInteger(anIndexStr.ToCString(), anIndex)) {
            *myDI << "Syntax error: time step index missing in input file name '" << m_fileName << "'";
            return false;
        }

        theTimeStep    = anIndex;
        theDescription = aDescription;
    } else {
        // obtain time step index and description from command line arguments
        if (theArgIter + 1 >= NbArguments() || !Draw::ParseInteger(anOption.ToCString(), anIndex)) {
            *myDI << "Syntax error: time step index or description missing";
            return false;
        }

        theTimeStep    = anIndex;
        theDescription = TCollection_AsciiString(Argument(++theArgIter));
    }

    return true;
}

// =======================================================================
// function : Perform
// =======================================================================
bool ScvFarfieldJsonMesh::Perform()
{
    std::vector<FarfieldTriplet> aParsedValues;
    PropInfo                     aPropInfo;

    // Parse file with data.
    if (!parseFile(aParsedValues, aPropInfo)) {
        return false;
    }

    // Remove previous object with the same name.
    RemoveObject(m_prsName);

    // Put parsed data into mesh data.
    Handle(RWCae::MeshData) aMeshData;
    createMeshData(aMeshData, aParsedValues, aPropInfo);

    // Set Id in MeshData with draw name
    aMeshData->SetId(m_prsName);

    // Print name of draw object
    *myDI << aMeshData->Id() << " ";

    // Create DrawableMeshData using mesh data, set to a Draw variable
    Handle(RWCaeTest::DrawableMeshData) aDrawable = new RWCaeTest::DrawableMeshData(aMeshData);
    Draw::Set(m_prsName.ToCString(), aDrawable);

    // Create a Mesh Owner using MeshData
    // Create a CAE Mesh interactive object using the Mesh Owner.
    Handle(ViewCae::InteractiveCaeMeshOwner) anOwner  = new ViewCae::InteractiveCaeMeshOwner(aMeshData);
    Handle(ViewCae::InteractiveCaeMesh)      aMeshPrs = new ViewCae::InteractiveCaeMesh(anOwner);

    // Set callback for color mapper change on the mesh IO
    const TCollection_AsciiString anId = aMeshPrs->MeshOwner()->Id();
    aMeshPrs->SetOnColorMapperChangeCallback(
        [anId](const RWCae::MeshPropertyComponent& theProp, const Handle(ViewCae::MeshColorMapper)& theMapper) {
            (void)anId;
            (void)theProp;
            (void)theMapper;
        });

    m_context->Display(aMeshPrs, 0, -1, false, PrsMgr_DisplayStatus_Erased);

    // Create mesh presentation and call ProcessRedraw callback
    aMeshPrs->CreatePresentation(ViewCae::MeshPresentationType::Scalar);
    aMeshPrs->ProcessRedraw(m_context, ViewerTest::CurrentView(), AIS_RedrawProgress_BeforeUpdate);

    RegisterObject(m_prsName, aMeshPrs);
    return true;
}

}
